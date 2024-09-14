
#include "symbolbackend.h"
#include "gdbcontainer.h"
#include "gdbmi.h"
#include "symbolpanel.h"
#include <QDebug>
#include <QMessageBox>
#include <dwarf.h>
#include <libdwarf.h>

SymbolBackend::SymbolBackend(QObject *parent) : QObject(parent) {
    m_symbolFileFullPath = "";
    m_gdbProperlySet = false;
    m_waitTimer.setSingleShot(true);

    // Initialize libdwarf data
    m_dwarfDbg = nullptr;

    // It will pop out automatically when construted, close it immediately
    m_progressDialog.close();

    // Prepare internal types
    createInternalTypes();
}

SymbolBackend::~SymbolBackend() {
    if (m_dwarfDbg != nullptr) {
        dwarf_finish(m_dwarfDbg);
        m_dwarfDbg = nullptr;
    }
}

QString SymbolBackend::errorString(SymbolBackend::Error error) {
    switch (error) {
        case Error::NoError: return tr("No error");
        case Error::DwarfApiFailure: return tr("A libdwarf API call has failed");
        case Error::ExplainTypeFailed:
            return tr("Action failed because an internal call to explainType was unsuccessful");
        case Error::DwarfFailedToGetAddressSize: return tr("DWARF: Failed to get machine address size.");
        case Error::DwarfReferredDieNotFound: return tr("DWARF: Referred DIE is not found.");
        case Error::DwarfDieTypeInvalid: return tr("DWARF: Referred DIE has invalid TAG type.");
        case Error::DwarfDieFormatInvalid: return tr("DWARF: Referred DIE data format is corrupted.");
        case Error::DwarfAttrNotConstant: return tr("DWARF: Attibute is not resolvable to a constant expression.");
        default: return tr("Unknown error");
    }
}

Result<void, SymbolBackend::Error> SymbolBackend::switchSymbolFile(QString symbolFileFullPath) {
    m_symbolFileFullPath = symbolFileFullPath;

    // Prepare a progress dialog
    m_progressDialog.setLabelText(tr("Loading symbol file..."));
    m_progressDialog.setWindowModality(Qt::ApplicationModal);
    m_progressDialog.show();

    Dwarf_Error dwErr;
    int dwRet;
    if (m_dwarfDbg) {
        // Clean up CU DIEs cache
        for (auto it = m_cus.begin(); it != m_cus.end(); it++) {
            for (auto jt = it->TopDies.begin(); jt != it->TopDies.end(); jt++) {
                dwarf_dealloc_die(*jt);
            }
            dwarf_dealloc_die(it->CuDie);
        }
        m_cus.clear();
        m_typeMap.clear();
        m_cuOffsetMap.clear();
        m_qualifiedSourceFiles.clear();
        createInternalTypes();

        // Discard previous file
        dwRet = dwarf_finish(m_dwarfDbg);
    }
    dwRet = dwarf_init_path(symbolFileFullPath.toLocal8Bit().data(), NULL, 0, DW_GROUPNUMBER_ANY, NULL, NULL,
                            &m_dwarfDbg, &dwErr);
    if (dwRet != DW_DLV_OK) {
        return Err(Error::DwarfApiFailure);
    }

    // Fetch machine native word size, for example, Cortex-M MCU's are 32 bit
    if (dwarf_get_address_size(m_dwarfDbg, &m_machineWordSize, &m_err) != DW_DLV_OK) {
        return Err(Error::DwarfFailedToGetAddressSize);
    }

    m_progressDialog.setLabelText(tr("Refreshing symbols..."));
    QCoreApplication::processEvents();

    // A temporary storage for all type DIEs
    QVector<DieRef> m_typeDies;

    // Try iterate through all CUs in dwarf
    Dwarf_Unsigned abbrev_offset = 0;
    Dwarf_Half address_size = 0;
    Dwarf_Half version_stamp = 0;
    Dwarf_Half offset_size = 0;
    Dwarf_Half extension_size = 0;
    Dwarf_Sig8 signature;
    Dwarf_Unsigned typeoffset = 0;
    Dwarf_Unsigned next_cu_header = 0;
    Dwarf_Half header_cu_type = 0;
    dwRet = DW_DLV_OK;

    // Iterate until all CU are drained
    forever {
        Dwarf_Die noDie = 0;
        Dwarf_Die cuDie = 0;
        Dwarf_Unsigned cuHeaderLength = 0;

        memset(&signature, 0, sizeof(signature));
        dwRet = dwarf_next_cu_header_d(m_dwarfDbg, true, &cuHeaderLength, &version_stamp, &abbrev_offset, &address_size,
                                       &offset_size, &extension_size, &signature, &typeoffset, &next_cu_header,
                                       &header_cu_type, &m_err);
        if (dwRet == DW_DLV_ERROR) {
            return Err(Error::DwarfApiFailure);
        }
        if (dwRet == DW_DLV_NO_ENTRY) {
            break;
        }
        dwRet = dwarf_siblingof_b(m_dwarfDbg, noDie, true, &cuDie, &m_err);
        if (dwRet != DW_DLV_OK) {
            return Err(Error::DwarfApiFailure);
        }

        // Retrieve file name. If file name cannot be retrieved, discard this CU
        DwarfAttrList attrList(m_dwarfDbg, cuDie);
        auto nameAttr = attrList(DW_AT_name);
        if (!nameAttr) {
            dwarf_dealloc_die(cuDie);
            continue;
        }
        char *nameStr;
        SourceFile srcFile;
        dwRet = dwarf_formstring(nameAttr, &nameStr, &m_err);
        if (!nameStr || dwRet != DW_DLV_OK) {
            dwarf_dealloc_die(cuDie);
            continue;
        }
        srcFile.path = QString(nameStr);
        srcFile.cuIndex = m_cus.size();
        // qDebug() << "CU file name" << nameStr;

        // Cache CU data, all children etc
        DwarfCuData cuData;

        cuData.CuDie = cuDie;       // CU DIE
        cuData.Name = srcFile.path; // File name
        // dwRet = dwarf_dieoffset(cuDie, &cuData.CuDieOff, &m_err); // CU Offset
        auto cuOffsetResult = DwarfCuDataOffsetFromDie(cuDie, &m_err);
        if (cuOffsetResult.isErr()) {
            dwarf_dealloc_die(cuDie);
            continue;
        }
        cuData.CuDieOff = cuOffsetResult.unwrap();
        // Comp dir
        auto compDirAttr = attrList(DW_AT_comp_dir);
        if (compDirAttr) {
            char *compDirStr;
            dwRet = dwarf_formstring(compDirAttr, &compDirStr, &m_err);
            if (nameStr && dwRet == DW_DLV_OK) {
                cuData.CompileDir = QString(compDirStr);
            }
        }
        // Producer
        auto producerAttr = attrList(DW_AT_producer);
        if (producerAttr) {
            char *producerStr;
            dwRet = dwarf_formstring(producerAttr, &producerStr, &m_err);
            if (producerStr && dwRet == DW_DLV_OK) {
                cuData.Producer = QString(producerStr);
            }
        }
        // CU top level DIEs
        Dwarf_Die firstTop = 0;
        dwRet = dwarf_child(cuDie, &firstTop, &m_err);
        if (!firstTop || dwRet != DW_DLV_OK) {
            dwarf_dealloc_die(cuDie);
            continue;
        }
        Dwarf_Die currentTop = firstTop;
        forever {
            Dwarf_Off dieOffset;
            if (dwarf_die_CU_offset(currentTop, &dieOffset, &m_err) != DW_DLV_OK) {
                goto SkipDie;
            }
            cuData.TopDies[dieOffset] = currentTop;
            // qDebug() << "  Children DIE @" << QString::number(dieOffset, 16);
        SkipDie:
            // Go to next sibling
            dwRet = dwarf_siblingof_b(m_dwarfDbg, currentTop, true, &currentTop, &m_err);
            if (dwRet == DW_DLV_NO_ENTRY) {
                break; // All done
            }

            Q_ASSERT(dwRet != DW_DLV_ERROR);
        }
        // Add into internal CU cache
        m_cus.append(cuData);
        m_cuOffsetMap[cuData.CuDieOff] = m_cus.size() - 1;
        // All DIEs, depth first search
        int depth = 0, cuIdx = m_cus.size() - 1;
        std::function<void(Dwarf_Die)> recurseGetDies = [&](Dwarf_Die rootDie) {
            Dwarf_Die childDie = 0;
            Dwarf_Die currentDie = rootDie;
            Dwarf_Off offset;

            // Cache current DIE
            int dwRet = dwarf_die_CU_offset(rootDie, &offset, &m_err);
            Q_ASSERT(dwRet == DW_DLV_OK);
            // cuData.Dies[offset] = rootDie;
            addDie(cuIdx, offset, rootDie);

            // Loop on siblings
            forever {
                Dwarf_Die siblingDie = 0;

                // See if current node has a child
                dwRet = dwarf_child(currentDie, &childDie, &m_err);
                Q_ASSERT(dwRet != DW_DLV_ERROR);
                if (dwRet == DW_DLV_OK) {
                    // If it does, try recursively on child
                    recurseGetDies(childDie);
                    firstTop = 0;
                }

                // After looking into child, try get sibling of current node
                dwRet = dwarf_siblingof_b(m_dwarfDbg, currentDie, true, &siblingDie, &m_err);
                Q_ASSERT(dwRet != DW_DLV_ERROR);
                if (dwRet == DW_DLV_NO_ENTRY) {
                    break;
                }
                currentDie = siblingDie;

                // Cache current DIE
                int dwRet = dwarf_die_CU_offset(currentDie, &offset, &m_err);
                Q_ASSERT(dwRet == DW_DLV_OK);
                // cuData.Dies[offset] = currentDie;
                addDie(cuIdx, offset, currentDie);
            }
        };

        recurseGetDies(firstTop);

        // Add as a valid CU for returning
        if (isCuQualifiedSourceFile(cuData)) {
            m_qualifiedSourceFiles.append(srcFile);
        }
    }

    m_progressDialog.setLabelText(tr("Generating type database..."));
    m_progressDialog.setMaximum(m_resolutionTypeDies.count());

    foreach (auto die, m_resolutionTypeDies) {
        auto tryDerefResult = derefTypeDie(die);
        if (tryDerefResult.isErr()) {
            qDebug() << "DIE pre-resolution failed for" << die;
        }
    }

    m_progressDialog.close();

    return Ok();
}

Result<QList<SymbolBackend::SourceFile>, SymbolBackend::Error> SymbolBackend::getSourceFileList() {
    return Ok(m_qualifiedSourceFiles);
}

Result<QList<SymbolBackend::VariableNode>, SymbolBackend::Error>
    SymbolBackend::getVariableOfSourceFile(uint32_t cuIndex) {
    if (cuIndex >= m_cus.size()) {
        return Err(Error::InvalidParameter);
    }
    QList<VariableNode> ret;
    auto &cu = m_cus[cuIndex];
    auto &topDies = cu.TopDies;

    // Find all variable DIEs
    for (auto it = topDies.constBegin(); it != topDies.constEnd(); it++) {
        Dwarf_Half tag;
        if (dwarf_tag(it.value(), &tag, &m_err) != DW_DLV_OK) {
            return Err(Error::DwarfApiFailure);
        }
        if (tag != DW_TAG_variable) {
            continue;
        }

        Dwarf_Die actualVariableDie = it.value();
        DwarfAttrList attrs(m_dwarfDbg, it.value());

        // Take address
        Dwarf_Attribute addrAttr = attrs(DW_AT_location);
        if (!addrAttr) {
            // If a variable has no address, discard it
            continue;
        }
        Dwarf_Unsigned exprLen;
        Dwarf_Ptr exprPtr;
        Dwarf_Half addrAttrForm;
        if (dwarf_whatform(addrAttr, &addrAttrForm, &m_err) != DW_DLV_OK) {
            return Err(Error::DwarfApiFailure);
        }
        // if (dwarf_formexprloc(addrAttr, &exprLen, &exprPtr, &m_err) != DW_DLV_OK) {
        //     Dwarf_Block *addrBlock;
        //     if (dwarf_formblock(addrAttr, &addrBlock, &m_err) != DW_DLV_OK) {
        //         return Err(Error::DwarfApiFailure);
        //     }
        //     exprPtr = 0;
        //     // HACK: Arm compiler uses expressions in block form instead of exprloc for variable addresses
        //     if (reinterpret_cast<uint8_t *>(addrBlock->bl_data)[0] == DW_OP_addr) {
        //         memcpy(&exprPtr, reinterpret_cast<uint8_t *>(addrBlock->bl_data) + 1, addrBlock->bl_len - 1);
        //     } else {
        //         return Err(Error::DwarfApiFailure);
        //     }
        // }
        auto addrAttrResult = DwarfFormConstant(addrAttr);
        if (addrAttrResult.isErr()) {
            qCritical() << "Variable DIE " << DieRef(cuIndex, it.key())
                        << "Failed to get address:" << errorString(addrAttrResult.unwrapErr());
            return Err(Error::DwarfDieFormatInvalid);
        }
        exprPtr = (Dwarf_Ptr *) addrAttrResult.unwrap().u;

        // If it is a reference, go to the destination
        // This is found in root items when a global is defined in a namespace
        Dwarf_Half form;
        Dwarf_Attribute specificationAttr;
        if ((specificationAttr = attrs(DW_AT_specification)) && !attrs.has(DW_AT_name)) {
            Dwarf_Bool isInfo;
            if (auto derefResult = anyDeref(specificationAttr, nullptr, &m_err); derefResult.isErr()) {
                Dwarf_Off off;
                dwarf_die_CU_offset(it.value(), &off, &m_err);
                qDebug() << "Go to referenced variable: var DIE specification invalid:" << QString::number(off, 16);
                return Err(Error::DwarfApiFailure);
            } else {
                auto deref = derefResult.unwrap();
                actualVariableDie = m_cus[deref.first].die(deref.second);
                attrs = std::move(DwarfAttrList(m_dwarfDbg, actualVariableDie));
            }
        }

        VariableNode node;
        node.address = exprPtr;

        // Get variable display name
        char *dispName;
        Dwarf_Attribute dispNameAttr;
        if (!(dispNameAttr = attrs(DW_AT_name))) {
            return Err(Error::DwarfApiFailure);
        }
        if (dwarf_formstring(dispNameAttr, &dispName, &m_err) != DW_DLV_OK) {
            return Err(Error::DwarfApiFailure);
        }
        node.displayName = QString(dispName);

        // Get a DIE offset reference to type specifier.
        // DW_AT_type can refer to a DIE in current CU (DW_FORM_ref_n) or in other CUs(DW_FORM_ref_addr)
        // So when we're looking for type info we need to search for other CUs when current CU doesn't have it
        Dwarf_Off typeSpec;
        Dwarf_Bool isInfo;
        Dwarf_Attribute typeSpecAttr;
        if (!(typeSpecAttr = attrs(DW_AT_type))) {
            return Err(Error::DwarfApiFailure);
        }
#if 0
        if (DwarfFormRefEx(typeSpecAttr, &typeSpec, &isInfo, &m_err) != DW_DLV_OK || !isInfo || !cu.hasDie(typeSpec)) {
            Dwarf_Off off;
            dwarf_dieoffset(it.value(), &off, &m_err);
            qDebug() << "Get typespec: CU has no DIE" << QString::number(typeSpec, 16)
                     << ", var DIE:" << QString::number(trueOffset ? trueOffset : off, 16);
            return Err(Error::LibdwarfApiFailure);
        }
#endif
        auto derefResult = anyDeref(typeSpecAttr, &isInfo, &m_err);
        if (derefResult.isErr()) {
            return Err(Error::DwarfApiFailure);
        }
        // node.typeSpec = typeSpec;
        node.typeSpec = derefResult.unwrap();

        // TODO: displayable type name
        auto typeObjResult = derefTypeDie(derefResult.unwrap());
        if (typeObjResult.isErr()) {
            node.displayTypeName = tr("<Error Type>");
            node.expandable = false;
            node.iconType = VariableIconType::Unknown;
        } else {
            auto typeObj = typeObjResult.unwrap();
            node.typeObj = typeObj;
            writeTypeInfoToVariableNode(node, typeObj);
        }

        ret.append(node);
    }

    return Ok(ret);
}

Result<SymbolBackend::ExpandNodeResult, SymbolBackend::Error>
    SymbolBackend::getVariableChildren(QString varName, uint32_t cuIndex, IType::p typeObj) {
    QList<VariableNode> ret;
    auto result = typeObj->getChildren();
    if (result.isErr()) {
        return Err(Error::InvalidParameter);
    }
    ExpandNodeResult expandResult;
    foreach (auto &i, result.unwrap()) {
        // I've decided that we leave anonymous substructures out (only synthesized members are left)
        if (i.flags & TypeChildInfo::AnonymousSubstructure) {
            continue;
        }

        VariableNode node;
        writeTypeInfoToVariableNode(node, i.type);
        // node.address = // FIXME:

        node.displayName = i.name;
        node.typeObj = i.type;
        expandResult.subNodeDetails.append(node);
    }
    // return Err(Error::NoError);
    return Ok(expandResult);
}

/***************************************** INTERNAL UTILS *****************************************/

void SymbolBackend::addDie(int cu, Dwarf_Off cuOffset, Dwarf_Die die) {
    Dwarf_Half tag;

    if (dwarf_tag(die, &tag, &m_err) == DW_DLV_OK) {
        switch (tag) {
            case DW_TAG_base_type:
            case DW_TAG_array_type:
            // case DW_TAG_subrange_type:
            case DW_TAG_enumeration_type:
            case DW_TAG_class_type:
            case DW_TAG_structure_type:
            case DW_TAG_union_type:
            case DW_TAG_string_type:
            case DW_TAG_atomic_type:
            case DW_TAG_volatile_type:
            case DW_TAG_const_type:
            case DW_TAG_restrict_type: m_resolutionTypeDies.append({cu, cuOffset}); break;
            case DW_TAG_namespace: m_resolutionNamespaceDies.append({cu, cuOffset}); break;
            default: break;
        }
    }

    m_cus[cu].Dies[cuOffset] = die;
}

void SymbolBackend::createInternalTypes() {
#define PS_CREATE_PRIMITIVE_TYPE(TYPE)                                                                                 \
    m_typeMap[DieRef{static_cast<int>(ReservedCu::InternalPrimitiveTypes),                                             \
                     static_cast<Dwarf_Off>(IType::Kind::TYPE)}] = std::make_shared<TypePrimitive>(IType::Kind::TYPE)

    PS_CREATE_PRIMITIVE_TYPE(Sint8);
    PS_CREATE_PRIMITIVE_TYPE(Sint16);
    PS_CREATE_PRIMITIVE_TYPE(Sint32);
    PS_CREATE_PRIMITIVE_TYPE(Sint64);
    PS_CREATE_PRIMITIVE_TYPE(Uint8);
    PS_CREATE_PRIMITIVE_TYPE(Uint16);
    PS_CREATE_PRIMITIVE_TYPE(Uint32);
    PS_CREATE_PRIMITIVE_TYPE(Uint64);
    PS_CREATE_PRIMITIVE_TYPE(Float32);
    PS_CREATE_PRIMITIVE_TYPE(Float64);

#undef PS_CREATE_PRIMITIVE_TYPE

    m_typeMap[DieRef{static_cast<int>(ReservedCu::InternalUnsupportedTypes), 0}] = std::make_shared<TypeUnsupported>();
}

IType::p SymbolBackend::getPrimitive(IType::Kind kind) {
    DieRef ref{static_cast<int>(ReservedCu::InternalPrimitiveTypes), static_cast<Dwarf_Off>(kind)};

    if (m_typeMap.contains(ref)) {
        return m_typeMap.value(ref);
    }

    return getUnsupported();
}

IType::p SymbolBackend::getUnsupported() {
    return m_typeMap[DieRef{static_cast<int>(ReservedCu::InternalUnsupportedTypes), 0}];
}

bool SymbolBackend::isCuQualifiedSourceFile(DwarfCuData &cu) {
    auto &topDies = cu.TopDies, &allDies = cu.Dies;

    // Find valid variable DIEs, if there is, return true
    for (auto it = topDies.constBegin(); it != topDies.constEnd(); it++) {
        Dwarf_Half tag;
        if (dwarf_tag(it.value(), &tag, &m_err) != DW_DLV_OK) {
            return false;
        }
        if (tag != DW_TAG_variable) {
            continue;
        }


        Dwarf_Die actualVariableDie = it.value();
        DwarfAttrList attrs(m_dwarfDbg, it.value());

        // Take address
        Dwarf_Attribute addrAttr = attrs(DW_AT_location);
        if (!addrAttr) {
            // If a variable has no address, discard it
            continue;
        }

        return true;
    }

    return false;
}

Result<QPair<int, Dwarf_Off>, std::nullptr_t> SymbolBackend::dieOffsetGlobalToCuBased(Dwarf_Off globalOffset) {
    auto cuIdxIt = m_cuOffsetMap.lowerBound(globalOffset);
    if (cuIdxIt.key() > globalOffset) {
        --cuIdxIt;
        Q_ASSERT(cuIdxIt.key() < globalOffset);
        Q_ASSERT(m_cus.size() > cuIdxIt.value());
        auto &targetCu = m_cus[cuIdxIt.value()];
        Q_ASSERT(targetCu.hasDie(globalOffset - targetCu.CuDieOff));
    }

    return Ok(QPair{cuIdxIt.value(), globalOffset - m_cus[cuIdxIt.value()].CuDieOff});
}

Result<QPair<int, Dwarf_Off>, int> SymbolBackend::anyDeref(Dwarf_Attribute dw_attr, Dwarf_Bool *dw_is_info,
                                                           Dwarf_Error *dw_err) {
    //
    Dwarf_Half form;
    Dwarf_Off offset;
    Dwarf_Bool isInfo;
    int result;

    result = dwarf_whatform(dw_attr, &form, dw_err);
    if (result != DW_DLV_OK) {
        return Err(result);
    }

    result = dwarf_global_formref_b(dw_attr, &offset, &isInfo, dw_err);
    if (result != DW_DLV_OK) {
        return Err(result);
    }

    if (dw_is_info != nullptr) {
        *dw_is_info = isInfo;
    }

    return Ok(dieOffsetGlobalToCuBased(offset).unwrap());
}

Result<IType::p, SymbolBackend::Error> SymbolBackend::derefTypeDie(SymbolBackend::DieRef typeDie) {
    // If it's already resolved, return it
    if (m_typeMap.contains(typeDie)) {
        return Ok(m_typeMap.value(typeDie, m_errorType));
    }

    // If not, try resolving
    return resolveTypeDie(typeDie);
}

Result<IType::p, SymbolBackend::Error> SymbolBackend::resolveTypeDie(SymbolBackend::DieRef typeDie) {
    if (m_cus.size() <= typeDie.cuIndex || !m_cus[typeDie.cuIndex].hasDie(typeDie.dieOffset)) {
        qCritical() << "resolveTypeDie: CU:Offset" << typeDie << "not found";
        return Err(Error::DwarfReferredDieNotFound);
    }

    Dwarf_Die die = m_cus[typeDie.cuIndex].die(typeDie.dieOffset);
    Dwarf_Half dieType;
    int dwRet;

    dwRet = dwarf_tag(die, &dieType, &m_err);
    if (dwRet != DW_DLV_OK) {
        return Err(Error::DwarfApiFailure);
    }

    IType::p ret;
    switch (dieType) {
        case DW_TAG_base_type: {
            // Do not create new TypePrimitive's here, take a predefined
            DwarfAttrList attr(m_dwarfDbg, die);
            Dwarf_Attribute encoding = attr(DW_AT_encoding);
            Dwarf_Attribute byteSize = attr(DW_AT_byte_size);
            if (!encoding || !byteSize) {
                qCritical() << "DW_TAG_base_type" << typeDie << "encoding" << encoding << "byteSize" << byteSize;
                return Err(Error::DwarfDieFormatInvalid);
            }
            // Select appropriate type for returning
            auto encodingResult = DwarfFormInt(encoding);
            auto byteSizeResult = DwarfFormInt(byteSize);
            if (encodingResult.isErr() || byteSizeResult.isErr()) {
                qWarning() << "DW_TAG_base_type" << typeDie << "attribute form failed";
                ret = getUnsupported();
            }
            auto encodingX = encodingResult.unwrap();
            auto byteSizeX = byteSizeResult.unwrap();
            if ((encodingX.u != 2 && (encodingX.u < 4 || encodingX.u > 8)) ||
                (qPopulationCount(byteSizeX.u) != 1 || byteSizeX.u > 8)) {
                qWarning() << "DW_TAG_base_type" << typeDie << "encoding =" << encodingX.u
                           << "byteSize =" << byteSizeX.u << "is unsupported!";
                ret = getUnsupported();
            }
            switch (encodingX.u) {
                case 4: // Floating point
                    switch (byteSizeX.u) {
                        case 4: ret = getPrimitive(IType::Kind::Float32); break;
                        case 8: ret = getPrimitive(IType::Kind::Float64); break;
                        default:
                            qWarning() << "DW_TAG_base_type" << typeDie << "Float byteSize =" << byteSizeX.u
                                       << "is unsupported!";
                            ret = getUnsupported();
                    }
                    break;
                case 2: // Boolean // FIXME: really?
                case 5: // Signed integer
                case 6: // Signed char
                    switch (byteSizeX.u) {
                        case 1: ret = getPrimitive(IType::Kind::Sint8); break;
                        case 2: ret = getPrimitive(IType::Kind::Sint16); break;
                        case 4: ret = getPrimitive(IType::Kind::Sint32); break;
                        case 8: ret = getPrimitive(IType::Kind::Sint64); break;
                        default: Q_UNREACHABLE();
                    }
                    break;
                case 7: // Unsigned integer
                case 8: // Unsigned char
                    switch (byteSizeX.u) {
                        case 1: ret = getPrimitive(IType::Kind::Uint8); break;
                        case 2: ret = getPrimitive(IType::Kind::Uint16); break;
                        case 4: ret = getPrimitive(IType::Kind::Uint32); break;
                        case 8: ret = getPrimitive(IType::Kind::Uint64); break;
                        default: Q_UNREACHABLE();
                    }
                    break;
            }
            break;
        }
        case DW_TAG_array_type: {
            // Ensure referred base type is resolved.
            DwarfAttrList attr(m_dwarfDbg, die);
            if (!attr.has(DW_AT_type)) {
                qCritical() << "DW_AT_array_type" << typeDie << "cannot find base type";
                return Err(Error::DwarfDieFormatInvalid);
            }
            auto derefBaseTypeResult = anyDeref(attr(DW_AT_type), nullptr, &m_err);
            if (derefBaseTypeResult.isErr()) {
                qCritical() << "DW_AT_array_type" << typeDie << "cannot deref base type DIE";
                return Err(Error::DwarfDieFormatInvalid);
            }
            auto baseTypeResult = resolveTypeDie(derefBaseTypeResult.unwrap());
            if (baseTypeResult.isErr()) {
                qCritical() << "DW_AT_array_type" << typeDie
                            << "base type resolution failed:" << errorString(baseTypeResult.unwrapErr());
                return baseTypeResult;
            }
            // Get subranges inside. If there isn't more than 1 subranges then we don't create a vector for it
            Dwarf_Die firstSubrange, subrange;
            if (dwarf_child(die, &firstSubrange, &m_err) != DW_DLV_OK) {
                qCritical() << "DW_TAG_array_type" << typeDie << "Cannot get subrange";
                return Err(Error::DwarfDieFormatInvalid);
            }
            if (dwarf_siblingof_c(firstSubrange, &subrange, &m_err) == DW_DLV_NO_ENTRY) {
                // Only one subrange. Directly create TypeModified
                DwarfAttrList attr(m_dwarfDbg, firstSubrange);
                if (!attr.has(DW_AT_upper_bound)) {
                    qCritical() << "DW_TAG_array_type" << typeDie << "Cannot get subrange upperbound";
                    return Err(Error::DwarfDieFormatInvalid);
                }
                auto upperRangeResult = DwarfFormInt(attr(DW_AT_upper_bound));
                if (upperRangeResult.isErr()) {
                    qCritical() << "DW_TAG_array_type" << typeDie << "Cannot form subrange upperbound";
                    return Err(Error::DwarfDieFormatInvalid);
                }
                auto upperRange = upperRangeResult.unwrap();
                // Keil generates ubound=0 for flexible array (a[]), GCC generates ubound=-1.
                // We unify it to be zero here (this will also be the convention in internal type representation)
                if (upperRange.s <= 0) {
                    upperRange.s = 0;
                }
                ret = std::make_shared<TypeModified>(baseTypeResult.unwrap(), TypeModified::Modifier::Array,
                                                     upperRange.s);
                break;
            } else {
                // Has multiple subranges. In this case, we create TypeModified for all the subrange DIEs except for the
                // first subrange (these TypeModified's will be stored in type map at the corresponding subrange DIEs'
                // offsets), the first subrange will not have a type object, and will be instead allocated to the array
                // DIE.
                QVector<Dwarf_Die> subranges{firstSubrange, subrange};
                while (dwarf_siblingof_c(subrange, &subrange, &m_err) != DW_DLV_NO_ENTRY) {
                    subranges.append(subrange);
                }
                auto lastLayerBaseType = baseTypeResult.unwrap();
                for (int i = subranges.size() - 1; i >= 0; i--) {
                    DwarfAttrList attr(m_dwarfDbg, subranges[i]);
                    if (!attr.has(DW_AT_upper_bound)) {
                        qCritical() << "DW_TAG_array_type" << typeDie << "Cannot get subrange upperbound";
                        return Err(Error::DwarfDieFormatInvalid);
                    }
                    auto upperRangeResult = DwarfFormInt(attr(DW_AT_upper_bound));
                    if (upperRangeResult.isErr()) {
                        qCritical() << "DW_TAG_array_type" << typeDie << "Cannot form subrange upperbound";
                        return Err(Error::DwarfDieFormatInvalid);
                    }
                    auto upperRange = upperRangeResult.unwrap();
                    // Keil generates ubound=0 for flexible array (a[]), GCC generates ubound=-1.
                    // We unify it to be zero here (this will also be the convention in internal type representation)
                    if (upperRange.s <= 0) {
                        upperRange.s = 0;
                    }
                    // Not ret yet, just a place to store the intermediate type
                    ret =
                        std::make_shared<TypeModified>(lastLayerBaseType, TypeModified::Modifier::Array, upperRange.s);
                    // If this is not last layer, update lastLayerBaseType and allocate to subrange DIE
                    if (i != 0) {
                        Dwarf_Off globOffset, localOffset;
                        int dwRet = dwarf_die_offsets(subranges[i], &globOffset, &localOffset, &m_err);
                        if (dwRet != DW_DLV_OK) {
                            qWarning() << "DW_TAG_array_type" << typeDie << "subrange" << i << "can't get offset";
                        } else {
                            auto cuLocalOffset = dieOffsetGlobalToCuBased(globOffset);
                            if (cuLocalOffset.isOk()) {
                                // Assign intermediate type object to subrange DIE
                                m_typeMap[cuLocalOffset.unwrap()] = ret;
                            }
                        }
                        // Prepare for next level
                        lastLayerBaseType = ret;
                    }
                    // If this is the last layer, type object is not assigned to subrange DIE, and the loop exits here.
                    // ret will be assigned to array DIE and be returned.
                }
            }
            break;
        }
        case DW_TAG_subrange_type: break; // TODO: not useful anymore
        case DW_TAG_pointer_type: {
            DwarfAttrList attr(m_dwarfDbg, die);
            if (!attr.has(DW_AT_type)) {
                // This is normal for void*
                ret = std::make_shared<TypeModified>(getUnsupported(), TypeModified::Modifier::Pointer,
                                                     m_machineWordSize);
            } else if (auto derefTypeResult = anyDeref(attr(DW_AT_type), nullptr, &m_err); derefTypeResult.isErr()) {
                qCritical() << "DW_TAG_pointer_type" << typeDie << "Referred type can't be dereferenced";
                return Err(Error::DwarfDieFormatInvalid);
            } else if (auto forwardResult = derefTypeDie(derefTypeResult.unwrap()); forwardResult.isErr()) {
                qCritical() << "DW_TAG_pointer_type" << typeDie
                            << "type resolution failed:" << errorString(forwardResult.unwrapErr());
                return Err(Error::DwarfDieFormatInvalid);
            } else {
                ret = std::make_shared<TypeModified>(forwardResult.unwrap(), TypeModified::Modifier::Pointer,
                                                     m_machineWordSize);
            }
            break;
        }
        case DW_TAG_enumeration_type:
            ret = getUnsupported();
            break; // TODO:
        // Class/Structure/Union are very similar in their internals
        case DW_TAG_class_type:
        case DW_TAG_structure_type:
        case DW_TAG_union_type: {
            // Kind variable for the three
            IType::Kind kind;
            switch (dieType) {
                case DW_TAG_class_type: kind = IType::Kind::Class; break;
                case DW_TAG_structure_type: kind = IType::Kind::Structure; break;
                case DW_TAG_union_type: kind = IType::Kind::Union; break;
            }
            // Create a temporary type object
            // We FOR NOW put it into the type map to prevent infinite recursion
            DwarfAttrList attr(m_dwarfDbg, die);
            auto type = std::make_shared<TypeStructure>(kind);
            m_typeMap[typeDie] = type;
            // Resolve type name and byte size
            if (!attr.has(DW_AT_name)) {
                type->m_anonymous = true;
            } else {
                auto nameAttr = attr(DW_AT_name);
                char *nameStr;
                if (dwarf_formstring(nameAttr, &nameStr, &m_err) != DW_DLV_OK) {
                    qCritical() << "Structural type" << typeDie << "Cannot form name string";
                    return Err(Error::DwarfDieFormatInvalid);
                }
                if (strlen(nameStr) == 0) {
                    type->m_anonymous = true;
                } else {
                    type->m_typeName = nameStr;
                }
            }
            if (!attr.has(DW_AT_byte_size)) {
                qCritical() << "Structural type" << typeDie << "Does NOT contain byte_size attribute";
                return Err(Error::DwarfDieFormatInvalid);
            } else {
                auto byteSizeResult = DwarfFormInt(attr(DW_AT_byte_size));
                if (byteSizeResult.isErr()) {
                    qCritical() << "Structural type" << typeDie << "Cannot form byte_size";
                    return Err(Error::DwarfDieFormatInvalid);
                }
                type->m_byteSize = byteSizeResult.unwrap().u;
            }
            // Check if this entry is just a declaration
            if (attr.has(DW_AT_declaration)) {
                // FIXME: UNTESTED
                type->m_declaration = true;
                ret = type;
                break;
            }
            Dwarf_Die child;
            if (dwarf_child(die, &child, &m_err) != DW_DLV_OK) {
                qCritical() << "Structural type" << typeDie << "Has no child";
                return Err(Error::DwarfDieFormatInvalid);
            }
            // Iterate through kids
            do {
                Dwarf_Half childTag;
                // Check if we've found a nested type definition
                if (dwarf_tag(child, &childTag, &m_err) != DW_DLV_OK) {
                    qWarning() << "Structural type" << typeDie << "Child" << child << "Cannot get tag";
                    continue;
                }
                if (isANestableType(childTag)) {
                    // Resolve nested type, correct scope
                    DieRef nestedTypeRef{typeDie.cuIndex, 0};
                    if (dwarf_die_CU_offset(child, &nestedTypeRef.dieOffset, &m_err) != DW_DLV_OK) {
                        qWarning() << "Structural type" << typeDie << "Child" << child << "Cannot get CU local offset";
                        continue;
                    }
                    auto nestedTypeResult = derefTypeDie(nestedTypeRef);
                    if (nestedTypeResult.isErr()) {
                        qWarning() << "Structural type" << typeDie << "Child" << child << "Cannot be deref'd";
                        continue;
                    }
                    auto nestedType = std::dynamic_pointer_cast<TypeStructure>(nestedTypeResult.unwrap());
                    if (!nestedType) {
                        qWarning() << "Structural type" << typeDie << "Child" << nestedTypeRef
                                   << "is NOT TypeStructure";
                        continue;
                    }
                    // Correct children scope
                    nestedType->m_parentScope = type;
                    continue;
                }
                // Inheritance. We DO NOT support virtual inheritances (virtual inherited base class will require
                // reading the vtable to determine base class address, and is unviable for our use case.)
                if (childTag == DW_TAG_inheritance) {
                    DwarfAttrList attr(m_dwarfDbg, child);
                    // If it is a virtual inheritance, discard it
                    if (attr.has(DW_AT_virtuality) &&
                        DwarfFormInt(attr(DW_AT_virtuality)).unwrapOr(DwarfFormedInt{0}).u == 1) {
                        qWarning() << "Structural type" << typeDie << "Has a virtual inheritance and was discarded";
                        type->m_hasVirtualInheritance = true;
                        continue;
                    } else if (!attr.has(DW_AT_type) || !attr.has(DW_AT_data_member_location)) {
                        // Get the inherited base class
                        qWarning() << "Structural type" << typeDie << "Inheritance information incomplete";
                        continue;
                    } else if (auto typeDerefResult = anyDeref(attr(DW_AT_type), nullptr, &m_err);
                               typeDerefResult.isErr()) {
                        qWarning() << "Structural type" << typeDie << "Cannot get inherited base type";
                        continue;
                    } else if (auto typeResult = derefTypeDie(typeDerefResult.unwrap()); typeResult.isErr()) {
                        qWarning() << "Structural type" << typeDie << "Cannot deref inherited base type at"
                                   << typeDerefResult.unwrap();
                        continue;
                    } else if (auto baseChildrenResult = typeResult.unwrap()->getChildren();
                               baseChildrenResult.isErr()) {
                        qWarning() << "Structural type" << typeDie << "Cannot get inherited base type members";
                        continue;
                    } else if (auto baseOffsetResult = DwarfFormConstant(attr(DW_AT_data_member_location));
                               baseOffsetResult.isErr()) {
                        qWarning() << "Structural type" << typeDie << "Cannot get inherited base type offset:"
                                   << errorString(baseOffsetResult.unwrapErr());
                        continue;
                    } else {
                        auto baseOffset = baseOffsetResult.unwrap().s;
                        // Add base class members
                        foreach (auto i, baseChildrenResult.unwrap()) {
                            i.flags |= TypeChildInfo::FromInheritance;
                            propagateOffset(i.byteOffset, baseOffset);
                            type->addMember(i);
                        }
                        // Propagate virtual inheritance flag
                        type->m_hasVirtualInheritance =
                            (typeResult.unwrap()->flags() & IType::HasVirtualInheritance) != 0;
                        continue;
                    }
                }
                // Now we only care about members
                if (childTag != DW_TAG_member) {
                    continue;
                }
                DwarfAttrList attr(m_dwarfDbg, child);
                TypeChildInfo childInfo;
                childInfo.bitOffset = 0;
                childInfo.bitWidth = 0;
                childInfo.byteOffset = 0;
                childInfo.flags = 0;
                // Get offsets (for unions they are always zero)
                if (!attr.has(DW_AT_data_member_location) && dieType != DW_TAG_union_type) {
                    continue; // Probably is a static member, discard this member
                }
                if (dieType != DW_TAG_union_type) {
                    auto offsetResult = DwarfFormConstant(attr(DW_AT_data_member_location));
                    if (offsetResult.isErr()) {
                        qWarning() << "Structural type" << typeDie << "member" << child
                                   << "Cannot form offset:" << errorString(offsetResult.unwrapErr());
                        continue;
                    }
                    childInfo.byteOffset = offsetResult.unwrap().u;
                }
                // Get name
                if (!attr.has(DW_AT_name)) {
                    childInfo.flags |= TypeChildInfo::AnonymousSubstructure;
                } else {
                    auto nameAttr = attr(DW_AT_name);
                    char *nameStr;
                    if (dwarf_formstring(nameAttr, &nameStr, &m_err) != DW_DLV_OK) {
                        qCritical() << "Structural type" << typeDie << "member @"
                                    << childInfo.byteOffset.value_or(0xffffffff) << "Cannot form name string";
                        continue;
                    }
                    if (strlen(nameStr) == 0) {
                        childInfo.flags |= TypeChildInfo::AnonymousSubstructure;
                    } else {
                        childInfo.name = nameStr;
                    }
                }
                // Get type
                if (!attr.has(DW_AT_type)) {
                    qWarning() << "Structural type" << typeDie << "member @" << childInfo.byteOffset
                               << "Does not have type attribute. Defaults to TypeUnsupported.";
                    childInfo.type = getUnsupported();
                } else if (auto typeDerefResult = anyDeref(attr(DW_AT_type), nullptr, &m_err);
                           typeDerefResult.isErr()) {
                    qWarning() << "Structural type" << typeDie << "member @" << childInfo.byteOffset
                               << "Cannot deref member type. Defaults to TypeUnsupported.";
                    childInfo.type = getUnsupported();
                } else if (auto typeResult = derefTypeDie(typeDerefResult.unwrap()); typeResult.isErr()) {
                    qWarning() << "Structural type" << typeDie << "member @" << childInfo.byteOffset
                               << "Cannot resolve member type. Defaults to TypeUnsupported.";
                    childInfo.type = getUnsupported();
                } else {
                    childInfo.type = typeResult.unwrap();
                }
                // Check if it is a bitfield
                auto bitfieldCheck = (attr.has(DW_AT_byte_size) ? 1 : 0) + (attr.has(DW_AT_bit_size) ? 1 : 0) +
                                     (attr.has(DW_AT_bit_offset) ? 1 : 0);
                if (bitfieldCheck != 3 && bitfieldCheck != 0) {
                    qCritical() << "Structural type" << typeDie << "member @" << childInfo.byteOffset
                                << "Is a broken bitfield";
                    continue;
                }
                if (bitfieldCheck == 3) {
                    auto byteSizeResult = DwarfFormInt(attr(DW_AT_byte_size));
                    auto bitSizeResult = DwarfFormInt(attr(DW_AT_bit_size));
                    auto bitOffsetResult = DwarfFormInt(attr(DW_AT_bit_offset));
                    if (byteSizeResult.isErr() || bitSizeResult.isErr() || bitOffsetResult.isErr()) {
                        qCritical() << "Structural type" << typeDie << "member @" << childInfo.byteOffset
                                    << "One or more bitfield parameter cannot be formed";
                        continue;
                    }
                    Dwarf_Unsigned bitOffset =
                        byteSizeResult.unwrap().u * 8 - bitSizeResult.unwrap().u - bitOffsetResult.unwrap().u;
                    propagateOffset(childInfo.byteOffset, bitOffset / 8);
                    childInfo.bitOffset = static_cast<uint8_t>(bitOffset % 8);
                    childInfo.bitWidth = static_cast<uint8_t>(bitSizeResult.unwrap().u);
                }
                // Add to members
                type->addMember(childInfo);
                // For anonymous substructures, add their members too
                if (childInfo.flags & TypeChildInfo::AnonymousSubstructure) {
                    auto anonChildResult = childInfo.type->getChildren();
                    if (anonChildResult.isErr()) {
                        qCritical() << "Structural type" << typeDie << "member @" << childInfo.byteOffset
                                    << "Cannot access members of an anonymous substructure.";
                        continue;
                    }
                    foreach (auto i, anonChildResult.unwrap()) {
                        propagateOffset(i.byteOffset, childInfo.byteOffset);
                        i.flags |= TypeChildInfo::FromAnonymousSubstructure;
                        type->addMember(i);
                    }
                }
            } while (dwarf_siblingof_c(child, &child, &m_err) != DW_DLV_NO_ENTRY);
            // Sort members by offset
            std::sort(type->m_children.begin(), type->m_children.end(),
                      [](const TypeChildInfo &a, const TypeChildInfo &b) {
                          if (a.byteOffset != b.byteOffset) {
                              return a.byteOffset < b.byteOffset;
                          }
                          if (a.bitOffset != b.bitOffset) {
                              return a.bitOffset < b.bitOffset;
                          }
                          return a.name < b.name; // Compare names if byteOffset and bitOffset are equal
                      });

            // Initialize members map, from this step on, the member vector is essentially frozen
            for (int i = 0; i < type->m_children.size(); i++) {
                auto &child = type->m_children[i];
                if (!(child.flags & TypeChildInfo::AnonymousSubstructure) && !child.name.isEmpty()) {
                    type->m_childrenMap[child.name] = i;
                }
            }
            ret = type;
            break;
        }
        case DW_TAG_subroutine_type:
        case DW_TAG_string_type: ret = getUnsupported(); break;
        // Forwarding types
        case DW_TAG_typedef:
        case DW_TAG_atomic_type:
        case DW_TAG_volatile_type:
        case DW_TAG_const_type:
        case DW_TAG_restrict_type: {
            DwarfAttrList attr(m_dwarfDbg, die);
            if (!attr.has(DW_AT_type)) {
                qCritical() << "Forwarding type" << typeDie << "Has no referred type";
                return Err(Error::DwarfDieFormatInvalid);
            }
            auto derefTypeResult = anyDeref(attr(DW_AT_type), nullptr, &m_err);
            if (derefTypeResult.isErr()) {
                qCritical() << "Forwarding type" << typeDie << "Referred type can't be dereferenced";
                return Err(Error::DwarfDieFormatInvalid);
            }
            auto forwardResult = derefTypeDie(derefTypeResult.unwrap());
            if (forwardResult.isErr()) {
                qCritical() << "Forwarding type" << typeDie
                            << "Forward failed:" << errorString(forwardResult.unwrapErr());
                return Err(Error::DwarfDieFormatInvalid);
            }
            ret = forwardResult.unwrap();

            // We frequently use typedef to give an unnamed structure an alias.
            // We give a name to these unnamed structures if we've found them.
            if (dieType == DW_TAG_typedef && (ret->flags() & IType::Anonymous)) {
                if (auto structureType = std::dynamic_pointer_cast<TypeStructure>(ret); structureType) {
                    if (!attr.has(DW_AT_name)) {
                        qWarning() << "Forwarding type" << typeDie
                                   << "Trying to name an unnamed structure, "
                                      "but typedef doesn't have a name";
                    } else if (char *typedefNamePtr;
                               dwarf_formstring(attr(DW_AT_name), &typedefNamePtr, &m_err) != DW_DLV_OK ||
                               typedefNamePtr == nullptr || strlen(typedefNamePtr) == 0) {
                        qWarning() << "Forwarding type" << typeDie
                                   << "Trying to name an unnamed structure, but typedef name "
                                      "is either unable to be formed, nullptr, or empty";
                    } else {
                        structureType->m_typeName = typedefNamePtr;
                        structureType->m_namedByTypedef = true;
                        structureType->m_anonymous = false;
                    }
                }
            }
            break;
        }
        case DW_TAG_reference_type:
            ret = getUnsupported();
            break; // TODO:
        // WTF
        case DW_TAG_ptr_to_member_type:
        case DW_TAG_unspecified_type: ret = getUnsupported(); break;
        default: Q_UNREACHABLE();
    }
    Q_ASSERT(ret.get() != nullptr);
    m_typeMap[typeDie] = ret;
    return Ok(ret);
}

#if 0
bool SymbolBackend::resolveTypeDetails(uint32_t cuIndex, Dwarf_Off typeDie, TypeResolutionContext &ctx) {
    DwarfCuData::TypeDieDetails details;
    auto &cu = m_cus[cuIndex];

    // Deep in until we reach the elementary type
    Dwarf_Off currentDieOff = typeDie;
    Dwarf_Die die;
    int dwRet;

    // forever {
    if (!cu.hasDie(currentDieOff)) {
        return false;
    }
    die = cu.die(currentDieOff);

    Dwarf_Half dieTag;
    if (dwarf_tag(die, &dieTag, &m_err) != DW_DLV_OK) {
        qDebug() << "Cannot get TAG for specified DIE" << typeDie;
        return false;
    }
    details.dwarfTag = dieTag;
    switch (dieTag) {
        case DW_TAG_array_type: {
            // When resolving array type tags, resolve its children which represent array dimensions
            // Simply use the first child's resolution
            Dwarf_Die firstRange;
            Dwarf_Off firstRangeOffset;
            if (dwarf_child(die, &firstRange, &m_err) != DW_DLV_OK) {
                return false;
            }
            if (dwarf_die_CU_offset(firstRange, &firstRangeOffset, &m_err) != DW_DLV_OK) {
                return false;
            }
            // Because subrange objects may contain no specific type info, we need to feed the
            // following resolution routines with the actual type defined in array object
            Dwarf_Attribute typeAttr;
            Dwarf_Off typeDieOff;
            Dwarf_Bool isInfo;
            DwarfAttrList attrs(m_dwarfDbg, die);
            Q_ASSERT(attrs.has(DW_AT_type));
            if (!(typeAttr = attrs(DW_AT_type))) {
                return false;
            }
            auto typeResult = anyDeref(typeAttr, &typeDieOff, &isInfo, &m_err);
            if (typeResult.isErr()) {
                return false;
            }
            ctx.arrayElementTypeOff = typeResult.unwrap();
            auto result = resolveTypeDetails(cuIndex, firstRangeOffset, ctx);
            if (!result) {
                return false;
            }
            auto resultX = getTypeDetails(cuIndex, firstRangeOffset).unwrap();
            details.displayName = resultX.displayName.arg("");
            details.expandable = resultX.expandable;
            break;
        }
        case DW_TAG_subrange_type: {
            // Example: Two dimision array "int a[3][5]" has two subranges, with upperbound 2 and 4
            // When we iterate through the siblings, we can only go to next sibling
            // But for arrays, logically when we access a[1] we actually drop the first subrange,
            // and logically we should append the array dimension sizes from beginning to end of siblings,
            // but when we do recursive scans we can only go from back to beginning, we need to somehow make it possible
            // to insert dimensions between the deeper dimensions and the base type. Therefore we have to put a "%1" in
            // the middle as a placeholder for shallower dimensions to insert into, and others don't need this insertion
            // point will have to get rid of them themselves.

            // Every subrange has an upperbound. Take that first
            DwarfAttrList attrs(m_dwarfDbg, die);
            Dwarf_Attribute ubAttr;
            union {
                Dwarf_Signed s;
                Dwarf_Unsigned u;
            } upperBound;
            bool hasUpperBound = true;
            if (!(ubAttr = attrs(DW_AT_upper_bound))) {
                hasUpperBound = false;
            } else {
                Dwarf_Half uboundForm = 0;
                if (dwarf_whatform(ubAttr, &uboundForm, &m_err) != DW_DLV_OK) {
                    qDebug() << "Cannot get upperbound form";
                    return false;
                }
                if (uboundForm != DW_FORM_sdata && uboundForm != DW_FORM_udata && uboundForm != DW_FORM_data1 &&
                    uboundForm != DW_FORM_data2 && uboundForm != DW_FORM_data4 && uboundForm != DW_FORM_data8 &&
                    uboundForm != DW_FORM_data16) {
                    qDebug() << "Invalid upperbound form";
                    return false;
                }
                if ((uboundForm == DW_FORM_sdata) ? (dwarf_formsdata(ubAttr, &upperBound.s, &m_err) != DW_DLV_OK)
                                                  : (dwarf_formudata(ubAttr, &upperBound.u, &m_err) != DW_DLV_OK)) {
                    qDebug() << "Cannot form upperbound";
                    return false;
                }

                // GCC does this for the top level subrange that has no specified size
                if (uboundForm == DW_FORM_sdata && upperBound.s == -1) {
                    hasUpperBound = false;
                }
            }

            Dwarf_Die deeperDim;
            Dwarf_Off deeperDimOff;
            dwRet = dwarf_siblingof_b(m_dwarfDbg, die, true, &deeperDim, &m_err);
            if (dwRet == DW_DLV_OK) {
                // We got deeper dimension, take that
                if (dwarf_die_CU_offset(deeperDim, &deeperDimOff, &m_err) != DW_DLV_OK) {
                    qDebug() << "Cannot form deeper dimension subrange";
                    return false;
                }
                if (!resolveTypeDetails(cuIndex, deeperDimOff, ctx)) {
                    qDebug() << "getTypeDetails for deeper dimension subrange failed";
                    return false;
                }
                // Insert our dimension in the middle
                auto uboundStr = hasUpperBound ? QString::number(upperBound.s + 1) : "";
                details.displayName = getTypeDetails(cuIndex, deeperDimOff)
                                          .unwrap()
                                          .displayName.arg("%1" + QString("[%1]").arg(uboundStr));
                details.expandable = hasUpperBound; // When no clear upper bound specified, do not make expandable
            } else {
                // No deeper dimension available, take base type
                auto result = getTypeDetails(ctx.arrayElementTypeOff);
                if (result.isErr()) {
                    qDebug() << "getTypeDetails of array base type failed";
                    return false;
                }
                auto uboundStr = hasUpperBound ? QString::number(upperBound.s + 1) : "";
                details.displayName = result.unwrap().displayName + "%1" + QString("[%1]").arg(uboundStr);
                details.expandable = hasUpperBound;
            }
            details.arraySubrangeElementTypeDie = ctx.arrayElementTypeOff;
            break;
        }
        case DW_TAG_base_type: {
            // Fill the displayed type
            DwarfAttrList attrs(m_dwarfDbg, die);
            Dwarf_Attribute nameAttr;
            char *nameStr = NULL;
            if (!(nameAttr = attrs(DW_AT_name))) {
                return false;
            }
            if (dwarf_formstring(nameAttr, &nameStr, &m_err) != DW_DLV_OK || !nameStr) {
                return false;
            }
            details.displayName = QString(nameStr);
            details.expandable = false;
            break;
        }
        case DW_TAG_subroutine_type: {
            // TODO: Deal with function pointer
            details.displayName = tr("Function pointer");
            details.expandable = false;
            break;
        }
        case DW_TAG_enumeration_type: {
            // Enums are stored as basic types, take base type and prefix with "(enum) "
            DwarfAttrList attrs(m_dwarfDbg, die);
            Dwarf_Attribute baseTypeAttr;
            Dwarf_Off baseTypeOff;
            Dwarf_Bool isInfo;
            if (!(baseTypeAttr = attrs(DW_AT_type))) {
                return false;
            }
            if (dwarf_formref(baseTypeAttr, &baseTypeOff, &isInfo, &m_err) != DW_DLV_OK) {
                return false;
            }
            auto result = getTypeDetails(cuIndex, baseTypeOff);
            if (result.isErr()) {
                return false;
            }
            details.displayName = "(enum) " + result.unwrap().displayName;
            details.expandable = false;
            break;
        }
        case DW_TAG_pointer_type: {
            // Take base type and append "*"
            DwarfAttrList attrs(m_dwarfDbg, die);
            Dwarf_Attribute baseTypeAttr;
            Dwarf_Off baseTypeOff;
            Dwarf_Bool isInfo;
            if (!(baseTypeAttr = attrs(DW_AT_type))) {
                details.displayName = "void*";
                details.expandable = false;
                break;
            }
            if (dwarf_formref(baseTypeAttr, &baseTypeOff, &isInfo, &m_err) != DW_DLV_OK) {
                return false;
            }
            auto result = getTypeDetails(cuIndex, baseTypeOff);
            if (result.isErr()) {
                return false;
            }
            details.displayName = result.unwrap().displayName + "*";
            details.expandable = true;
            // FIXME: consider logic for expanding basic types / structs/unions
            break;
        }
        case DW_TAG_union_type:
        case DW_TAG_structure_type:
        case DW_TAG_class_type: {
            // Fill the displayed type
            DwarfAttrList attrs(m_dwarfDbg, die);
            Dwarf_Attribute nameAttr;
            char *nameStr = NULL;
            // Use real class/struct names for those who have it
            if ((nameAttr = attrs(DW_AT_name))) {
                if (dwarf_formstring(nameAttr, &nameStr, &m_err) != DW_DLV_OK || !nameStr) {
                    return false;
                }
                details.displayName = QString(nameStr);
            } else {
                // For anonymous struct/classes etc, use typedef type name
                details.displayName = ctx.typedefTypeName;
            }
            details.expandable = true;
            break;
        }
        case DW_TAG_typedef: {
            // Before passing through, get the typedef type name
            // Some inner types have no names, a candidate type name fron typedef is necessary
            DwarfAttrList attrs(m_dwarfDbg, die);
            Dwarf_Attribute nameAttr;
            char *nameStr = NULL;
            if (!(nameAttr = attrs(DW_AT_name))) {
                return false;
            }
            if (dwarf_formstring(nameAttr, &nameStr, &m_err) != DW_DLV_OK || !nameStr) {
                return false;
            }
            ctx.typedefTypeName = nameStr;

            // Pass this through to base type
            Dwarf_Attribute baseTypeAttr;
            Dwarf_Off baseTypeOff;
            Dwarf_Bool isInfo;
            if (!(baseTypeAttr = attrs(DW_AT_type))) {
                return false;
            }
            if (dwarf_formref(baseTypeAttr, &baseTypeOff, &isInfo, &m_err) != DW_DLV_OK) {
                return false;
            }
            auto ret = resolveTypeDetails(cuIndex, baseTypeOff, ctx);
            if (ret) {
                // Copy base type to current type DIE
                cu.CachedTypes[currentDieOff] = cu.CachedTypes[baseTypeOff];
            }
            return ret;
            break;
        }
        case DW_TAG_atomic_type:
        case DW_TAG_restrict_type:
        case DW_TAG_const_type:
        case DW_TAG_volatile_type: {
            // Meaningless to application, pass through
            DwarfAttrList attrs(m_dwarfDbg, die);
            Dwarf_Attribute baseTypeAttr;
            Dwarf_Off baseTypeOff;
            Dwarf_Bool isInfo;
            if (!(baseTypeAttr = attrs(DW_AT_type))) {
                return false;
            }
            auto typeResult = anyDeref(baseTypeAttr, &baseTypeOff, &isInfo, &m_err);
            if (typeResult.isErr()) {
                return false;
            }
            auto typeRef = typeResult.unwrap();
            auto ret = resolveTypeDetails(typeRef.first, typeRef.second, ctx);
            if (ret) {
                // Copy base type to current type DIE
                details = m_cus[typeRef.first].CachedTypes[typeRef.second];
            }
            break;
        }
    }
    // }
    cu.CachedTypes[currentDieOff] = details;
    return true;
}

Result<SymbolBackend::ExpandNodeResult, SymbolBackend::Error>
    SymbolBackend::tryExpandType(uint32_t cuIndex, Dwarf_Off typeDie, TypeResolutionContext ctx, QString varName) {
    //
    ExpandNodeResult ret;

    auto &cu = m_cus[cuIndex];
    if (cuIndex >= m_cus.size() || !cu.hasDie(typeDie)) {
        return Err(Error::InvalidParameter);
    }
    auto die = cu.die(typeDie);

    Dwarf_Half dieTag;
    if (dwarf_tag(die, &dieTag, &m_err) != DW_DLV_OK) {
        qDebug() << "Cannot get TAG for specified DIE" << typeDie;
        return Err(Error::DwarfApiFailure);
    }
    ret.tag = dieTag;

    switch (dieTag) {
        case DW_TAG_array_type: {
            // The real useful type info is the first subrange, return that instead
            // Get the subrange and passthrough
            Dwarf_Die childDie;
            Dwarf_Off childDieOffset;
            Dwarf_Half childDieTag;
            if (dwarf_child(die, &childDie, &m_err) != DW_DLV_OK ||
                dwarf_die_CU_offset(childDie, &childDieOffset, &m_err) != DW_DLV_OK ||
                dwarf_tag(childDie, &childDieTag, &m_err) != DW_DLV_OK || childDieTag != DW_TAG_subrange_type) {
                return Err(Error::DwarfApiFailure);
            }
            // Still needs to pass the element type to the inner layers, that's why we have ctx here
            // FIXME: we store element types in cache. Is this still needed?
            Dwarf_Attribute typeAttr;
            Dwarf_Off typeDieOff;
            Dwarf_Bool isInfo;
            DwarfAttrList attrs(m_dwarfDbg, die);
            Q_ASSERT(attrs.has(DW_AT_type));
            if (!(typeAttr = attrs(DW_AT_type))) {
                return Err(Error::DwarfApiFailure);
            }
            auto typeResult = anyDeref(typeAttr, &typeDieOff, &isInfo, &m_err);
            if (typeResult.isErr()) {
                return Err(Error::DwarfApiFailure);
            }
            ctx.arrayElementTypeOff = typeResult.unwrap();
            return tryExpandType(cuIndex, childDieOffset, ctx, varName);
            break;
        }
        case DW_TAG_subrange_type: {
            // Take children count. An expandable array/subrange must contain valid ubound, simply use unsigned api
            Dwarf_Unsigned upperbound;
            DwarfAttrList attrs(m_dwarfDbg, die);
            Dwarf_Attribute uboundAttr;
            if (!(uboundAttr = attrs(DW_AT_upper_bound)) ||
                dwarf_formudata(uboundAttr, &upperbound, &m_err) != DW_DLV_OK) {
                return Err(Error::DwarfApiFailure);
            }
            // Check if this is the deepest dimension
            int dwRet;
            Dwarf_Die deeperDim;
            Dwarf_Off deeperDimOff;
            dwRet = dwarf_siblingof_b(m_dwarfDbg, die, true, &deeperDim, &m_err);
            if (dwRet == DW_DLV_OK) {
                // Has deeper dimension, use its offset as child type
                if (dwarf_die_CU_offset(deeperDim, &deeperDimOff, &m_err)) {
                    return Err(Error::DwarfApiFailure);
                }
                // return Ok(std::tuple<uint32_t, Dwarf_Off>{upperbound, deeperDimOff});
                // Build return data
                auto subtypeDetails = getTypeDetails(cuIndex, deeperDimOff);
                if (subtypeDetails.isErr()) {
                    return Err(subtypeDetails.unwrapErr());
                }
                for (Dwarf_Unsigned i = 0; i < upperbound + 1; i++) {
                    VariableNode node;
                    node.displayName = QString("%1[%2]").arg(varName).arg(i);
                    node.displayTypeName = subtypeDetails.unwrap().displayName.arg("");
                    node.expandable = subtypeDetails.unwrap().expandable;
                    node.iconType = VariableIconType::Array;
                    node.typeSpec = DieRef(cuIndex, deeperDimOff);
                    // node.address = // TODO: Addressing scheme??
                    ret.subNodeDetails.append(node);
                }
                return Ok(ret);
            } else {
                // Has no deeper dimension, use element type
                if (!cu.CachedTypes.contains(typeDie)) {
                    return Err(Error::DwarfApiFailure);
                }
                // return Ok(
                //     std::tuple<uint32_t, Dwarf_Off>{upperbound,
                //     cu.CachedTypes[typeDie].arraySubrangeElementTypeDie});
                // Build return data
                auto subTypeDieOff = cu.CachedTypes[typeDie].arraySubrangeElementTypeDie;
                auto subtypeDetailsResult = getTypeDetails(subTypeDieOff.cuIndex, subTypeDieOff.dieOffset);
                if (subtypeDetailsResult.isErr()) {
                    return Err(subtypeDetailsResult.unwrapErr());
                }
                auto subtypeDetails = subtypeDetailsResult.unwrap();
                for (Dwarf_Unsigned i = 0; i < upperbound + 1; i++) {
                    VariableNode node;
                    node.displayName = QString("%1[%2]").arg(varName).arg(i);
                    node.displayTypeName = subtypeDetails.displayName;
                    node.expandable = subtypeDetails.expandable;
                    node.iconType = dwarfTagToIconType(subtypeDetails.dwarfTag);
                    node.typeSpec = subTypeDieOff;
                    // node.address = // TODO: Addressing scheme??
                    ret.subNodeDetails.append(node);
                }
                return Ok(ret);
            }
            break;
        }
        case DW_TAG_pointer_type: {
            // FIXME: return pointed object now, but in later days we will need to match GDB behavior
            // Take base type and append "*"
            DwarfAttrList attrs(m_dwarfDbg, die);
            Dwarf_Attribute baseTypeAttr;
            Dwarf_Off baseTypeOff;
            Dwarf_Bool isInfo;
            if (!(baseTypeAttr = attrs(DW_AT_type)) ||
                dwarf_formref(baseTypeAttr, &baseTypeOff, &isInfo, &m_err) != DW_DLV_OK) {
                return Err(Error::DwarfApiFailure);
            }
            auto typeResult = anyDeref(baseTypeAttr, &baseTypeOff, &isInfo, &m_err);
            if (typeResult.isErr()) {
                return Err(Error::DwarfApiFailure);
            }
            auto result = getTypeDetails(typeResult.unwrap());
            if (result.isErr()) {
                return Err(result.unwrapErr());
            }
            VariableNode node;
            // node.address =// TODO: Addressing scheme???
            node.displayName = QString("(*%1)").arg(varName);
            node.displayTypeName = result.unwrap().displayName;
            node.expandable = result.unwrap().expandable;
            node.iconType = dwarfTagToIconType(result.unwrap().dwarfTag);
            node.typeSpec = typeResult.unwrap();
            ret.subNodeDetails.append(node);
            return Ok(ret);
            // FIXME: consider logic for expanding basic types / structs/unions
            break;
        }
        case DW_TAG_union_type:
        case DW_TAG_structure_type:
        case DW_TAG_class_type: {
            // Take all members
            Dwarf_Die child = 0;
            if (dwarf_child(die, &child, &m_err) != DW_DLV_OK) {
                // Really...? A structure with nothing?
                return Ok(ret);
            }
            forever {
                Dwarf_Half tag;
                if (dwarf_tag(child, &tag, &m_err) != DW_DLV_OK) {
                    return Err(Error::DwarfApiFailure);
                }
                if (tag == DW_TAG_member) {
                    // Process this member
                    DwarfAttrList attrs(m_dwarfDbg, child);
                    Dwarf_Attribute nameAttr, typeAddr, locationAddr;
                    if (!(nameAttr = attrs(DW_AT_name)) || !(typeAddr = attrs(DW_AT_type)) ||
                        ((dieTag != DW_TAG_union_type) && !(locationAddr = attrs(DW_AT_data_member_location)))) {
                        // NOTE: A static member in a class CAN have no DW_AT_data_member_location and have a
                        // DW_AT_const_value instead
                        goto NextSibling;
                    }
                    char *nameStr;
                    Dwarf_Bool isInfo;
                    if (dwarf_formstring(nameAttr, &nameStr, &m_err) != DW_DLV_OK) {
                        return Err(Error::DwarfApiFailure);
                    }
                    Dwarf_Off typeGlobalOff;
                    auto typeResult = anyDeref(typeAddr, &typeGlobalOff, &isInfo, &m_err);
                    if (typeResult.isErr()) {
                        return Err(Error::DwarfApiFailure);
                    }
                    auto result = getTypeDetails(typeResult.unwrap());
                    if (result.isErr()) {
                        return Err(result.unwrapErr());
                    }
                    auto resultX = result.unwrap();
                    VariableNode node;
                    // node.address=// TODO: Addressing scheme?
                    node.displayName = QString(nameStr);
                    node.displayTypeName = resultX.displayName;
                    node.iconType = dwarfTagToIconType(resultX.dwarfTag);
                    node.typeSpec = typeResult.unwrap();
                    node.expandable = resultX.expandable;
                    ret.subNodeDetails.append(node);
                }
            NextSibling:
                // Get next sibling
                int dwRet = dwarf_siblingof_b(m_dwarfDbg, child, true, &child, &m_err);
                if (dwRet == DW_DLV_NO_ENTRY) {
                    break;
                } else if (dwRet != DW_DLV_OK) {
                    return Err(Error::DwarfApiFailure);
                }
            }
            return Ok(ret);
            break;
        }
        case DW_TAG_typedef:
        case DW_TAG_atomic_type:
        case DW_TAG_restrict_type:
        case DW_TAG_const_type:
        case DW_TAG_volatile_type: {
            // Meaningless to application, pass through
            DwarfAttrList attrs(m_dwarfDbg, die);
            Dwarf_Attribute baseTypeAttr;
            Dwarf_Off baseTypeOff;
            Dwarf_Bool isInfo;
            if (!(baseTypeAttr = attrs(DW_AT_type)) ||
                dwarf_formref(baseTypeAttr, &baseTypeOff, &isInfo, &m_err) != DW_DLV_OK) {
                return Err(Error::DwarfApiFailure);
            }
            return tryExpandType(cuIndex, baseTypeOff, ctx, varName);
        }
    }
    return Err(Error::InvalidParameter);
}
#endif

SymbolBackend::VariableIconType SymbolBackend::dwarfTagToIconType(Dwarf_Half tag) {
    switch (tag) {
        case DW_TAG_structure_type:
        case DW_TAG_union_type:
        case DW_TAG_class_type: return VariableIconType::Structure; break;
        case DW_TAG_enumeration_type:
        case DW_TAG_base_type: return VariableIconType::Integer; break;
        case DW_TAG_array_type:
        case DW_TAG_subrange_type: return VariableIconType::Array; break;
        case DW_TAG_pointer_type: return VariableIconType::Pointer; break;
        default: return VariableIconType::Unknown; break;
    }
}

void SymbolBackend::writeTypeInfoToVariableNode(VariableNode &node, IType::p typeObj) {
    node.displayTypeName = typeObj->displayName();
    node.expandable = typeObj->expandable();
    // Give it an icon
    if (typeObj->flags() & IType::Flags::Modified) {
        // Modified type special processing
        auto modified = std::dynamic_pointer_cast<TypeModified>(typeObj);
        if (!modified) {
            qWarning() << "Type object" << typeObj.get() << "has Modified flag but can't be cast to TypeModified";
            node.iconType = VariableIconType::Unknown;
        } else {
            switch (modified->modifier()) {
                case TypeModified::Modifier::Array: node.iconType = VariableIconType::Array; break;
                case TypeModified::Modifier::Pointer: node.iconType = VariableIconType::Pointer; break;
            }
        }
    } else {
        switch (typeObj->kind()) {
            case IType::Kind::Unsupported: node.iconType = VariableIconType::Unknown; break;
            case IType::Kind::Uint8:
            case IType::Kind::Sint8:
            case IType::Kind::Uint16:
            case IType::Kind::Sint16:
            case IType::Kind::Uint32:
            case IType::Kind::Sint32:
            case IType::Kind::Uint64:
            case IType::Kind::Sint64: node.iconType = VariableIconType::Integer; break;
            case IType::Kind::Float32:
            case IType::Kind::Float64: node.iconType = VariableIconType::FloatingPoint; break;
            case IType::Kind::Structure:
            case IType::Kind::Union: node.iconType = VariableIconType::Structure; break;
        }
    }
}

bool SymbolBackend::isANestableType(Dwarf_Half tag) {
    switch (tag) {
        case DW_TAG_class_type:
        case DW_TAG_structure_type:
        case DW_TAG_union_type: return true;
        default: return false;
    }
}

void SymbolBackend::propagateOffset(std::optional<TypeChildInfo::offset_t> &base,
                                    std::optional<TypeChildInfo::offset_t> offset) {
    if (offset.has_value()) {
        // Propagate offset value
        base.value() += offset.value();
    } else {
        // Propagate undeterminable state
        base.reset();
    }
}

QDebug operator<<(QDebug debug, const SymbolBackend::DieRef &c) {
    QDebugStateSaver saver(debug);
    return debug.nospace() << "DieRef(" << c.cuIndex << "," << c.dieOffset << ")";
}

QDebug operator<<(QDebug debug, const std::optional<TypeChildInfo::offset_t> &c) {
    QDebugStateSaver saver(debug);
    if (c.has_value()) {
        return debug.nospace().noquote() << "ChildOffset(0x" << QString::number(c.value(), 16) << ")";
    } else {
        return debug.nospace() << "ChildOffset(Undetermined)";
    }
}

/***************************************** DWARF HELPERS *****************************************/

Result<Dwarf_Off, int> SymbolBackend::DwarfCuDataOffsetFromDie(Dwarf_Die die, Dwarf_Error *error) {
    Dwarf_Off cu_offset, cu_length;

    int result = dwarf_die_CU_offset_range(die, &cu_offset, &cu_length, error);
    if (result != DW_DLV_OK) {
        return Err(result);
    }

    return Ok(cu_offset);
}

Result<SymbolBackend::DwarfFormedInt, int> SymbolBackend::DwarfFormInt(Dwarf_Attribute attr) {
    Dwarf_Half form = 0;
    DwarfFormedInt formedInt;
    int ret;
    if ((ret = dwarf_whatform(attr, &form, &m_err)) != DW_DLV_OK) {
        qDebug() << "Cannot get form";
        return Err(ret);
    }
    if (form != DW_FORM_sdata && form != DW_FORM_udata && form != DW_FORM_data1 && form != DW_FORM_data2 &&
        form != DW_FORM_data4 && form != DW_FORM_data8 && form != DW_FORM_data16) {
        qDebug() << "Invalid form";
        return Err(0);
    }
    if ((form == DW_FORM_sdata) ? ((ret = dwarf_formsdata(attr, &formedInt.s, &m_err)) != DW_DLV_OK)
                                : ((ret = dwarf_formudata(attr, &formedInt.u, &m_err)) != DW_DLV_OK)) {
        qDebug() << "Cannot form data";
        return Err(ret);
    }

    return Ok(formedInt);
}

Result<SymbolBackend::DwarfFormedInt, SymbolBackend::Error> SymbolBackend::DwarfFormConstant(Dwarf_Attribute attr) {
    Dwarf_Half form = 0;
    DwarfFormedInt formedInt;
    int dwRet;
    formedInt.u = 0;

    if ((dwRet = dwarf_whatform(attr, &form, &m_err)) != DW_DLV_OK) {
        qDebug() << "Cannot get form";
        return Err(Error::DwarfApiFailure);
    }
    if (form == DW_FORM_block || form == DW_FORM_exprloc) {
        // https://www.prevanders.net/libdwarfdoc/group__example__loclistcv5.html
        Dwarf_Unsigned lcount = 0;
        Dwarf_Loc_Head_c loclist_head = 0;
        if (dwarf_get_loclist_c(attr, &loclist_head, &lcount, &m_err) != DW_DLV_OK) {
            dwarf_dealloc_loc_head_c(loclist_head);
            return Err(Error::DwarfApiFailure);
        }
        for (Dwarf_Unsigned i = 0; i < lcount; ++i) {
            Dwarf_Small loclist_lkind = 0;
            Dwarf_Small lle_value = 0;
            Dwarf_Unsigned rawval1 = 0;
            Dwarf_Unsigned rawval2 = 0;
            Dwarf_Bool debug_addr_unavailable = false;
            Dwarf_Addr lopc = 0;
            Dwarf_Addr hipc = 0;
            Dwarf_Unsigned loclist_expr_op_count = 0;
            Dwarf_Locdesc_c locdesc_entry = 0;
            Dwarf_Unsigned expression_offset = 0;
            Dwarf_Unsigned locdesc_offset = 0;

            dwRet = dwarf_get_locdesc_entry_d(loclist_head, i, &lle_value, &rawval1, &rawval2, &debug_addr_unavailable,
                                              &lopc, &hipc, &loclist_expr_op_count, &locdesc_entry, &loclist_lkind,
                                              &expression_offset, &locdesc_offset, &m_err);
            if (dwRet == DW_DLV_OK) {
                Dwarf_Unsigned j = 0;
                int opres = 0;
                Dwarf_Small op = 0;

                for (j = 0; j < loclist_expr_op_count; ++j) {
                    Dwarf_Unsigned opd1 = 0;
                    Dwarf_Unsigned opd2 = 0;
                    Dwarf_Unsigned opd3 = 0;
                    Dwarf_Unsigned offsetforbranch = 0;

                    opres = dwarf_get_location_op_value_c(locdesc_entry, j, &op, &opd1, &opd2, &opd3, &offsetforbranch,
                                                          &m_err);
                    if (opres == DW_DLV_OK) {
                        switch (op) {
                            case DW_OP_plus_uconst: formedInt.u += opd1; break;
                            case DW_OP_addr: formedInt.u = opd1; break;
                            default: Q_ASSERT_X(false, __FUNCTION__, "Unexpected DW_OP");
                        }
                    } else {
                        dwarf_dealloc_loc_head_c(loclist_head);
                        /*Something is wrong. */
                        return Err(Error::DwarfApiFailure);
                    }
                }
            } else {
                /* Something is wrong. Do something. */
                dwarf_dealloc_loc_head_c(loclist_head);
                return Err(Error::DwarfApiFailure);
            }
        }
    } else {
        auto formIntResult = DwarfFormInt(attr);
        if (formIntResult.isErr()) {
            return Err(Error::DwarfApiFailure);
        } else {
            return Ok(formIntResult.unwrap());
        }
    }
    return Ok(formedInt);
}

/***************************************** INTERNAL SLOTS *****************************************/
