
#include "symbolbackend.h"
#include "gdbcontainer.h"
#include "gdbmi.h"
#include "symbolpanel.h"
#include <QDebug>
#include <QMessageBox>
#include <libdwarf/dwarf.h>
#include <libdwarf/libdwarf.h>



SymbolBackend::SymbolBackend(QString gdbExecutable, QObject *parent) : QObject(parent) {
    m_symbolFileFullPath = "";
    m_gdbProperlySet = false;
    m_waitTimer.setSingleShot(true);

    // Initialize libdwarf data
    m_dwarfDbg = nullptr;

    // It will pop out automatically when construted, close it immediately
    m_progressDialog.close();

    connect(&m_gdb, &GdbContainer::gdbExited, this, &SymbolBackend::gdbExited);
    connect(&m_gdb, &GdbContainer::gdbCommandResponse, this, &SymbolBackend::gdbResponse);

    // Wait GDB response related
    connect(&m_waitTimer, &QTimer::timeout, this, &SymbolBackend::gdbWaitTimedOut);

    setGdbExecutableLazy(gdbExecutable);
}

SymbolBackend::~SymbolBackend() {
    if (m_dwarfDbg != nullptr) {
        dwarf_finish(m_dwarfDbg);
        m_dwarfDbg = nullptr;
    }

    m_gdb.stopGdb();
}

QString SymbolBackend::errorString(SymbolBackend::Error error) {
    switch (error) {
        case Error::NoError:
            return tr("No error");
        case Error::GdbNotStarted:
            return tr("GDB not started");
        case Error::GdbResponseTimeout:
            return tr("GDB response timeout");
        case Error::GdbStartupFail:
            return tr("GDB startup failed");
        case Error::GdbCommandExecutionError:
            return tr("GDB command execution failure");
        case Error::UnexpectedGdbConsoleOutput:
            return tr("Unexpected GDB console output (nothing useful found)");
        case Error::UnsupportedCppIdentifier:
            return tr("Unsupported C++ identifier is used");
        case Error::LibdwarfApiFailure:
            return tr("A libdwarf API call has failed");
        case Error::ExplainTypeFailed:
            return tr("Action failed because an internal call to explainType was unsuccessful");
        default:
            return tr("Unknown error");
    }
}

bool SymbolBackend::setGdbExecutableLazy(QString gdbPath) {
    if (m_gdbProperlySet) {
        return false;
    }

    if (gdbPath.isEmpty()) {
        QMessageBox::warning(nullptr, tr("GDB executable path not set"),
                             tr("GDB executable path is not set. Please set it in settings, "
                                "and manually start GDB from menu after this."));
        return false;
    }

    m_gdb.setGdbExecutablePath(gdbPath);

    // See if it starts successfully
    Error startupResult = Error::NoError;
    connect(&m_gdb, &GdbContainer::gdbStarted, [&](bool successful) {
        if (successful) {
            startupResult = Error::NoError;
        } else {
            startupResult = Error::GdbStartupFail;
        }
    });

    // Starting a process is instantaneous on all platforms I know of so startupResult should be ready
    // right after startup call
    m_gdb.startGdb();

    // Disconnect gdbStarted signal - it's only used here but beware of this later
    disconnect(&m_gdb, SIGNAL(gdbStarted));

    if (startupResult != Error::NoError) {
        QMessageBox::critical(nullptr, tr("GDB startup failed"),
                              tr("GDB startup failed. Please set proper GDB executable path "
                                 "and manually start GDB from menu.\n\n"
                                 "Failed GDB executable path: %1\n")
                                  .arg(gdbPath));
        return false;
    } else {
        m_gdbProperlySet = true; // Now you should not tamper with GDB anymore
        return true;
    }
}

Result<void, SymbolBackend::Error> SymbolBackend::switchSymbolFile(QString symbolFileFullPath) {
    m_symbolFileFullPath = symbolFileFullPath;

    // Prepare a progress dialog
    m_progressDialog.setLabelText(tr("Loading symbol file..."));
    m_progressDialog.show();

    // // If we have a symbol file set, tell GDB to use it
    // auto result = retryableGdbCommand(QString("-file-exec-and-symbols \"%1\"").arg(m_symbolFileFullPath));
    // if (result.isErr()) {
    //     return Err(result.unwrapErr());
    // }

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
        m_qualifiedSourceFiles.clear();

        // Discard previous file
        dwRet = dwarf_finish(m_dwarfDbg);
    }
    dwRet = dwarf_init_path(symbolFileFullPath.toLocal8Bit().data(), NULL, 0, DW_GROUPNUMBER_ANY, NULL, NULL,
                            &m_dwarfDbg, &dwErr);
    if (dwRet != DW_DLV_OK) {
        return Err(Error::LibdwarfApiFailure);
    }


    // Refresh our symbol table, with a similar structure of GDB/MI's serialization format
    m_progressDialog.setLabelText(tr("Refreshing symbols..."));
    QCoreApplication::processEvents();

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
                                       &header_cu_type, NULL);
        if (dwRet == DW_DLV_ERROR) {
            return Err(Error::LibdwarfApiFailure);
        }
        if (dwRet == DW_DLV_NO_ENTRY) {
            break;
        }
        dwRet = dwarf_siblingof_b(m_dwarfDbg, noDie, true, &cuDie, NULL);
        if (dwRet != DW_DLV_OK) {
            return Err(Error::LibdwarfApiFailure);
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
        dwRet = dwarf_formstring(nameAttr, &nameStr, NULL);
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
        dwRet = dwarf_die_CU_offset(cuDie, &cuData.CuDieOff, NULL);
        if (dwRet != DW_DLV_OK) {
            dwarf_dealloc_die(cuDie);
            continue;
        }
        // Comp dir
        auto compDirAttr = attrList(DW_AT_comp_dir);
        if (compDirAttr) {
            char *compDirStr;
            dwRet = dwarf_formstring(compDirAttr, &compDirStr, NULL);
            if (nameStr && dwRet == DW_DLV_OK) {
                cuData.CompileDir = QString(compDirStr);
            }
        }
        // Producer
        auto producerAttr = attrList(DW_AT_producer);
        if (producerAttr) {
            char *producerStr;
            dwRet = dwarf_formstring(producerAttr, &producerStr, NULL);
            if (producerStr && dwRet == DW_DLV_OK) {
                cuData.Producer = QString(producerStr);
            }
        }
        // CU top level DIEs
        Dwarf_Die firstTop = 0;
        dwRet = dwarf_child(cuDie, &firstTop, NULL);
        if (!firstTop || dwRet != DW_DLV_OK) {
            dwarf_dealloc_die(cuDie);
            continue;
        }
        Dwarf_Die currentTop = firstTop;
        forever {
            Dwarf_Off dieOffset;
            if (dwarf_die_CU_offset(currentTop, &dieOffset, NULL) != DW_DLV_OK) {
                goto SkipDie;
            }
            cuData.TopDies[dieOffset /* + cuData.CuDieOff*/] = currentTop;
            // qDebug() << "  Children DIE @" << QString::number(dieOffset, 16);
        SkipDie:
            // Go to next sibling
            dwRet = dwarf_siblingof_b(m_dwarfDbg, currentTop, true, &currentTop, NULL);
            if (dwRet == DW_DLV_NO_ENTRY) {
                break; // All done
            }

            Q_ASSERT(dwRet != DW_DLV_ERROR);
        }
        // All DIEs, depth first search
        int depth = 0;
        std::function<void(Dwarf_Die)> recurseGetDies = [&](Dwarf_Die rootDie) {
            Dwarf_Die childDie = 0;
            Dwarf_Die currentDie = rootDie;
            Dwarf_Off offset;

            // Cache current DIE
            int dwRet = dwarf_die_CU_offset(rootDie, &offset, NULL);
            Q_ASSERT(dwRet == DW_DLV_OK);
            cuData.Dies[offset /* + cuData.CuDieOff*/] = rootDie;

            // Loop on siblings
            forever {
                Dwarf_Die siblingDie = 0;

                // See if current node has a child
                dwRet = dwarf_child(currentDie, &childDie, NULL);
                Q_ASSERT(dwRet != DW_DLV_ERROR);
                if (dwRet == DW_DLV_OK) {
                    // If it does, try recursively on child
                    recurseGetDies(childDie);
                    firstTop = 0;
                }

                // After looking into child, try get sibling of current node
                dwRet = dwarf_siblingof_b(m_dwarfDbg, currentDie, true, &siblingDie, NULL);
                Q_ASSERT(dwRet != DW_DLV_ERROR);
                if (dwRet == DW_DLV_NO_ENTRY) {
                    break;
                }
                currentDie = siblingDie;

                // Cache current DIE
                int dwRet = dwarf_die_CU_offset(currentDie, &offset, NULL);
                Q_ASSERT(dwRet == DW_DLV_OK);
                cuData.Dies[offset /* + cuData.CuDieOff*/] = currentDie;
            }

            // // Try go deeper
            // dwRet = dwarf_child(rootDie, &child, NULL);
            // if (dwRet == DW_DLV_OK && child) {
            //     recurseGetDies(child);
            // } else if (dwRet == DW_DLV_NO_ENTRY) {
            //     // No children, go for siblings
            //     forever {
            //         // Get next sibling
            //         dwRet = dwarf_siblingof_b(m_dwarfDbg, rootDie, true, &rootDie, NULL);
            //         if (dwRet == DW_DLV_NO_ENTRY) {
            //             // No more sibling in the chain, return from this level
            //             return;
            //         } else {
            //             // Process next one
            //             recurseGetDies(rootDie);
            //         }
            //     }
            // }
        };

        recurseGetDies(firstTop);

        // Add into internal CU cache
        m_cus.append(cuData);

        // Add as a valid CU for returning
        if (isCuQualifiedSourceFile(cuData)) {
            m_qualifiedSourceFiles.append(srcFile);
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
    auto &topDies = cu.TopDies, &allDies = cu.Dies;

    // Find all variable DIEs
    for (auto it = topDies.constBegin(); it != topDies.constEnd(); it++) {
        Dwarf_Half tag;
        if (dwarf_tag(it.value(), &tag, NULL) != DW_DLV_OK) {
            return Err(Error::LibdwarfApiFailure);
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
        if (dwarf_whatform(addrAttr, &addrAttrForm, NULL) != DW_DLV_OK) {
            return Err(Error::LibdwarfApiFailure);
        }
        // FIXME: Arm compiler uses block form instead of exprloc for variable addresses
        if (dwarf_formexprloc(addrAttr, &exprLen, &exprPtr, NULL) != DW_DLV_OK) {
            Dwarf_Block *addrBlock;
            if (dwarf_formblock(addrAttr, &addrBlock, NULL) != DW_DLV_OK) {
                return Err(Error::LibdwarfApiFailure);
            }
            // TODO: How to use?????
        }

        // If it is a reference, go to the destination
        // This is found in root items when a global is defined in a namespace
        Dwarf_Half form;
        Dwarf_Attribute specificationAttr;
        Dwarf_Off trueOffset = 0;
        if ((specificationAttr = attrs(DW_AT_specification)) && !attrs.has(DW_AT_name)) {
            Dwarf_Bool isInfo;
            if (dwarf_formref(specificationAttr, &trueOffset, &isInfo, NULL) != DW_DLV_OK || !isInfo ||
                !cu.hasDie(trueOffset)) {
                Dwarf_Off off;
                dwarf_die_CU_offset(it.value(), &off, NULL);
                qDebug() << "Go to referenced variable: CU has no DIE" << QString::number(trueOffset, 16)
                         << ", var DIE:" << QString::number(off, 16);
                return Err(Error::LibdwarfApiFailure);
            }
            actualVariableDie = allDies.value(trueOffset);
            attrs = std::move(DwarfAttrList(m_dwarfDbg, actualVariableDie));
        }

        VariableNode node;
        node.address = exprPtr;

        // Get variable display name
        char *dispName;
        Dwarf_Attribute dispNameAttr;
        if (!(dispNameAttr = attrs(DW_AT_name))) {
            return Err(Error::LibdwarfApiFailure);
        }
        if (dwarf_formstring(dispNameAttr, &dispName, NULL) != DW_DLV_OK) {
            return Err(Error::LibdwarfApiFailure);
        }
        node.displayName = QString(dispName);

        // Get a DIE offset reference to type specifier. DW_AT_type is always a ref to other DIEs in CU
        Dwarf_Off typeSpec;
        Dwarf_Bool isInfo;
        Dwarf_Attribute typeSpecAttr;
        if (!(typeSpecAttr = attrs(DW_AT_type))) {
            return Err(Error::LibdwarfApiFailure);
        }
        if (dwarf_formref(typeSpecAttr, &typeSpec, &isInfo, NULL) != DW_DLV_OK || !isInfo || !cu.hasDie(typeSpec)) {
            Dwarf_Off off;
            dwarf_dieoffset(it.value(), &off, NULL);
            qDebug() << "Get typespec: CU has no DIE" << QString::number(typeSpec, 16)
                     << ", var DIE:" << QString::number(trueOffset ? trueOffset : off, 16);
            return Err(Error::LibdwarfApiFailure);
        }
        node.typeSpec = typeSpec;

        // TODO: displayable type name
        auto typeDetailsResult = getTypeDetails(cuIndex, typeSpec);
        if (typeDetailsResult.isErr()) {
            node.displayTypeName = tr("<Error Type>");
            node.expandable = false;
            node.iconType = VariableIconType::Unknown;
        } else {
            auto typeDetails = typeDetailsResult.unwrap();
            node.displayTypeName = typeDetails.displayName;
            node.expandable = typeDetails.expandable;
            switch (typeDetails.dwarfTag) {
                case DW_TAG_structure_type:
                case DW_TAG_union_type:
                case DW_TAG_class_type:
                    node.iconType = VariableIconType::Structure;
                    break;
                case DW_TAG_enumeration_type:
                case DW_TAG_base_type:
                    node.iconType = VariableIconType::Integer;
                    break;
                case DW_TAG_array_type:
                case DW_TAG_subrange_type:
                    node.iconType = VariableIconType::Array;
                    break;
                case DW_TAG_pointer_type:
                    node.iconType = VariableIconType::Pointer;
                    break;
                default:
                    node.iconType = VariableIconType::Unknown;
                    break;
            }
        }

        ret.append(node);
    }

    return Ok(ret);
}

Result<QString, SymbolBackend::Error> SymbolBackend::explainType(QString typeName) {
    // This ability was not exposed to MI unfortunately, you must emulate console input
    // GAAAAHHHHHHH

    // C++ is known to cause issues, let's get rid of them
    if (typeName.contains("::")) {
        return Ok(QString("__PROBESCOPE_INVALID_TYPE_T"));
    }

    // WHY ON EARTH class KEYWORD IS PRINTED BUT NOT RECOGNIZED?
    typeName.remove("class ");

    auto result = retryableGdbCommand(QString("-interpreter-exec console \"whatis %1\"").arg(typeName));
    if (result.isErr()) {
        return Err(result.unwrapErr());
    }

    foreach (auto i, result.unwrap().outputs) {
        if (i.type != gdbmi::Response::ConsoleOutput::console) {
            continue;
        }

        auto explainedType = i.text;
        if (!explainedType.contains("~\"type = ")) {
            continue;
        }

        // Example: R"(~"type = struct __SPI_HandleTypeDef\n")"
        // R"(~"type = int *\n")"
        explainedType.remove("~\"type = "); // Should be at very beginning
        explainedType.remove("\\n\"");      // Should be at very last

        // Remove the goddam cv qualifiers for good
        explainedType.remove("volatile ");
        explainedType.remove("const ");

        // Now: R"(struct __SPI_HandleTypeDef)"
        // R"(int *)"
        return Ok(explainedType);
    }

    return Err(Error::UnexpectedGdbConsoleOutput);
}

Result<uint64_t, SymbolBackend::Error> SymbolBackend::getVariableChildrenCount(QString expr) {
    // Some C++ stuff is naughty, discard them
    if (expr.contains("::")) {
        return Err(Error::UnsupportedCppIdentifier);
    }

    auto createResult = retryableGdbCommand(QString("-var-create - @ \"%1\"").arg(expr));
    if (createResult.isErr()) {
        return Err(createResult.unwrapErr());
    }

    auto payload = createResult.unwrap().payload.toMap();
    uint64_t ret = payload["numchild"].toUInt();

    // Don't care if the delete fails
    auto deleteResult = retryableGdbCommand(QString("-var-delete \"%1\"").arg(payload["name"].toString()));
    return Ok(ret);
}

Result<SymbolBackend::ExpandNodeResult, SymbolBackend::Error>
    SymbolBackend::getVariableChildren(QString varName, uint32_t cuIndex, Dwarf_Off typeSpec) {
    // TODO:
    QList<VariableNode> ret;

    TypeResolutionContext ctx;
    auto result = tryExpandType(cuIndex, typeSpec, ctx, varName);
    if (result.isErr()) {
        return Err(result.unwrapErr());
    }

    // return Err(Error::NoError);
    return Ok(result.unwrap());
}

/***************************************** INTERNAL UTILS *****************************************/

void SymbolBackend::recoverGdbCrash() {
    m_gdb.startGdb();

    // If we have a symbol file set, tell GDB to use it
    waitGdbResponse(m_gdb.sendCommand(QString("-file-exec-and-symbols \"%1\"").arg(m_symbolFileFullPath)));
}

Result<int, SymbolBackend::Error> SymbolBackend::warnUnstartedGdb() {
    if (m_gdb.isGdbRunning()) {
        return Ok(0);
    }

    QMessageBox::warning(nullptr, tr("GDB not started"), tr("GDB is not started. Please start GDB from menu."));
    return Err(Error::GdbNotStarted);
}

Result<gdbmi::Response, SymbolBackend::Error> SymbolBackend::retryableGdbCommand(QString command, int timeout) {
    auto warnResult = warnUnstartedGdb();
    if (warnResult.isErr()) {
        return Err(warnResult.unwrapErr());
    }

    QApplication::setOverrideCursor(Qt::WaitCursor);
    forever {
        auto token = m_gdb.sendCommand(command);
        auto result = waitGdbResponse(token, timeout);

        // The GDB command somehow never returned
        if (result.isErr()) {
            auto intention = QMessageBox::critical(nullptr, tr("GDB command unsuccessful"),
                                                   tr("The underlying GDB command was unsuccessful, status: %1.\n"
                                                      "Do you want to retry?\n\n"
                                                      "Command: %2")
                                                       .arg(errorString(result.unwrapErr()), command),
                                                   QMessageBox::Yes | QMessageBox::No);

            if (intention == QMessageBox::Yes) {
                continue;
            } else {
                QApplication::restoreOverrideCursor();
                return result;
            }
        } else {
            // Do a simple check on the result class, and only pop a dialog when there is an error
            auto response = result.unwrap();

            if (response.message == "error") {
                QMessageBox::critical(nullptr, tr("GDB Command Error"),
                                      tr("GDB command error: %1\n\nCommand: %2")
                                          .arg(response.payload.toMap().value("msg").toString(), command));

                QApplication::restoreOverrideCursor();
                return Err(Error::GdbCommandExecutionError);
            }

            QApplication::restoreOverrideCursor();
            return result;
        }
    }
}

Result<gdbmi::Response, SymbolBackend::Error> SymbolBackend::waitGdbResponse(uint64_t token, int timeout) {
    m_expectedGdbResponseToken = token;
    m_waitTimer.start(timeout);
    auto eventLoopRet = SymbolBackend::Error(m_eventLoop.exec());
    // Event loop takes over, until timed out or GDB responded

    // Kill the timer, it's useless now
    m_waitTimer.stop();

    if (eventLoopRet != Error::NoError) {
        return Err(eventLoopRet);
    } else {
        return Ok(m_lastGdbResponse);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
///                     GDB GARBAGE ABOVE
/////////////////////////////////////////////////////////////////////////////////////////////////////


bool SymbolBackend::isCuQualifiedSourceFile(DwarfCuData &cu) {
    auto &topDies = cu.TopDies, &allDies = cu.Dies;

    // Find valid variable DIEs, if there is, return true
    for (auto it = topDies.constBegin(); it != topDies.constEnd(); it++) {
        Dwarf_Half tag;
        if (dwarf_tag(it.value(), &tag, NULL) != DW_DLV_OK) {
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

bool SymbolBackend::ensureTypeResolved(uint32_t cuIndex, Dwarf_Off typeDie) {
    auto &cu = m_cus[cuIndex];
    if (cu.CachedTypes.contains(typeDie)) {
        return true;
    } else {
        TypeResolutionContext ctx; ///< Leave as default; it's only for internal use
        if (resolveTypeDetails(cuIndex, typeDie, ctx)) {
            return true;
        } else {
            return false;
        }
    }
}

Result<SymbolBackend::DwarfCuData::TypeDieDetails, SymbolBackend::Error>
    SymbolBackend::getTypeDetails(uint32_t cuIndex, Dwarf_Off typeDie) {
    if (cuIndex >= m_cus.size()) {
        return Err(Error::InvalidParameter);
    }

    auto &cu = m_cus[cuIndex];
    if (ensureTypeResolved(cuIndex, typeDie)) {
        return Ok(cu.CachedTypes[typeDie]);
    } else {
        return Err(Error::LibdwarfApiFailure);
    }
}

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
    if (dwarf_tag(die, &dieTag, NULL) != DW_DLV_OK) {
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
            if (dwarf_child(die, &firstRange, NULL) != DW_DLV_OK) {
                return false;
            }
            if (dwarf_die_CU_offset(firstRange, &firstRangeOffset, NULL) != DW_DLV_OK) {
                return false;
            }
            // Because subrange objects may contain no specific type info, we need to feed the
            // following resolution routines with the actual type defined in array object
            Dwarf_Attribute typeAttr;
            Dwarf_Off typeDieOff;
            Dwarf_Bool isInfo;
            DwarfAttrList attrs(m_dwarfDbg, die);
            Q_ASSERT(attrs.has(DW_AT_type));
            if (!(typeAttr = attrs(DW_AT_type)) || dwarf_formref(typeAttr, &typeDieOff, &isInfo, NULL) != DW_DLV_OK) {
                return false;
            }
            ctx.arrayElementTypeOff = typeDieOff;
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
                if (dwarf_whatform(ubAttr, &uboundForm, NULL) != DW_DLV_OK) {
                    qDebug() << "Cannot get upperbound form";
                    return false;
                }
                if (uboundForm != DW_FORM_sdata && uboundForm != DW_FORM_data1 && uboundForm != DW_FORM_data2 &&
                    uboundForm != DW_FORM_data4 && uboundForm != DW_FORM_data8 && uboundForm != DW_FORM_data16) {
                    qDebug() << "Invalid upperbound form";
                    return false;
                }
                if ((uboundForm == DW_FORM_sdata) ? (dwarf_formsdata(ubAttr, &upperBound.s, NULL) != DW_DLV_OK)
                                                  : (dwarf_formudata(ubAttr, &upperBound.u, NULL) != DW_DLV_OK)) {
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
            dwRet = dwarf_siblingof_b(m_dwarfDbg, die, true, &deeperDim, NULL);
            if (dwRet == DW_DLV_OK) {
                // We got deeper dimension, take that
                if (dwarf_die_CU_offset(deeperDim, &deeperDimOff, NULL) != DW_DLV_OK) {
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
                auto result = getTypeDetails(cuIndex, ctx.arrayElementTypeOff);
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
            if (dwarf_formstring(nameAttr, &nameStr, NULL) != DW_DLV_OK || !nameStr) {
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
            if (dwarf_formref(baseTypeAttr, &baseTypeOff, &isInfo, NULL) != DW_DLV_OK) {
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
            if (dwarf_formref(baseTypeAttr, &baseTypeOff, &isInfo, NULL) != DW_DLV_OK) {
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
                if (dwarf_formstring(nameAttr, &nameStr, NULL) != DW_DLV_OK || !nameStr) {
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
            if (dwarf_formstring(nameAttr, &nameStr, NULL) != DW_DLV_OK || !nameStr) {
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
            if (dwarf_formref(baseTypeAttr, &baseTypeOff, &isInfo, NULL) != DW_DLV_OK) {
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
            if (dwarf_formref(baseTypeAttr, &baseTypeOff, &isInfo, NULL) != DW_DLV_OK) {
                return false;
            }
            auto ret = resolveTypeDetails(cuIndex, baseTypeOff, ctx);
            if (ret) {
                // Copy base type to current type DIE
                details = cu.CachedTypes[baseTypeOff];
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
    if (dwarf_tag(die, &dieTag, NULL) != DW_DLV_OK) {
        qDebug() << "Cannot get TAG for specified DIE" << typeDie;
        return Err(Error::LibdwarfApiFailure);
    }
    ret.tag = dieTag;

    switch (dieTag) {
        case DW_TAG_array_type: {
            // The real useful type info is the first subrange, return that instead
            // Get the subrange and passthrough
            Dwarf_Die childDie;
            Dwarf_Off childDieOffset;
            Dwarf_Half childDieTag;
            if (dwarf_child(die, &childDie, NULL) != DW_DLV_OK ||
                dwarf_die_CU_offset(childDie, &childDieOffset, NULL) != DW_DLV_OK ||
                dwarf_tag(childDie, &childDieTag, NULL) != DW_DLV_OK || childDieTag != DW_TAG_subrange_type) {
                return Err(Error::LibdwarfApiFailure);
            }
            // Still needs to pass the element type to the inner layers, that's why we have ctx here
            // FIXME: we store element types in cache. Is this still needed?
            Dwarf_Attribute typeAttr;
            Dwarf_Off typeDieOff;
            Dwarf_Bool isInfo;
            DwarfAttrList attrs(m_dwarfDbg, die);
            Q_ASSERT(attrs.has(DW_AT_type));
            if (!(typeAttr = attrs(DW_AT_type)) || dwarf_formref(typeAttr, &typeDieOff, &isInfo, NULL) != DW_DLV_OK) {
                return Err(Error::LibdwarfApiFailure);
            }
            ctx.arrayElementTypeOff = typeDieOff;
            return tryExpandType(cuIndex, childDieOffset, ctx, varName);
            break;
        }
        case DW_TAG_subrange_type: {
            // Take children count. An expandable array/subrange must contain valid ubound, simply use unsigned api
            Dwarf_Unsigned upperbound;
            DwarfAttrList attrs(m_dwarfDbg, die);
            Dwarf_Attribute uboundAttr;
            if (!(uboundAttr = attrs(DW_AT_upper_bound)) ||
                dwarf_formudata(uboundAttr, &upperbound, NULL) != DW_DLV_OK) {
                return Err(Error::LibdwarfApiFailure);
            }
            // Check if this is the deepest dimension
            int dwRet;
            Dwarf_Die deeperDim;
            Dwarf_Off deeperDimOff;
            dwRet = dwarf_siblingof_b(m_dwarfDbg, die, true, &deeperDim, NULL);
            if (dwRet == DW_DLV_OK) {
                // Has deeper dimension, use its offset as child type
                if (dwarf_die_CU_offset(deeperDim, &deeperDimOff, NULL)) {
                    return Err(Error::LibdwarfApiFailure);
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
                    node.typeSpec = deeperDimOff;
                    // node.address = // TODO: Addressing scheme??
                    ret.subNodeDetails.append(node);
                }
                return Ok(ret);
            } else {
                // Has no deeper dimension, use element type
                if (!cu.CachedTypes.contains(typeDie)) {
                    return Err(Error::LibdwarfApiFailure);
                }
                // return Ok(
                //     std::tuple<uint32_t, Dwarf_Off>{upperbound,
                //     cu.CachedTypes[typeDie].arraySubrangeElementTypeDie});
                // Build return data
                auto subTypeDieOff = cu.CachedTypes[typeDie].arraySubrangeElementTypeDie;
                auto subtypeDetailsResult = getTypeDetails(cuIndex, subTypeDieOff);
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
                dwarf_formref(baseTypeAttr, &baseTypeOff, &isInfo, NULL) != DW_DLV_OK) {
                return Err(Error::LibdwarfApiFailure);
            }
            auto result = getTypeDetails(cuIndex, baseTypeOff);
            if (result.isErr()) {
                return Err(result.unwrapErr());
            }
            VariableNode node;
            // node.address =// TODO: Addressing scheme???
            node.displayName = QString("(*%1)").arg(varName);
            node.displayTypeName = result.unwrap().displayName;
            node.expandable = result.unwrap().expandable;
            node.iconType = dwarfTagToIconType(result.unwrap().dwarfTag);
            node.typeSpec = baseTypeOff;
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
            if (dwarf_child(die, &child, NULL) != DW_DLV_OK) {
                // Really...? A structure with nothing?
                return Ok(ret);
            }
            forever {
                Dwarf_Half tag;
                if (dwarf_tag(child, &tag, NULL) != DW_DLV_OK) {
                    return Err(Error::LibdwarfApiFailure);
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
                    Dwarf_Off typeOff;
                    if (dwarf_formstring(nameAttr, &nameStr, NULL) != DW_DLV_OK ||
                        dwarf_formref(typeAddr, &typeOff, &isInfo, NULL) != DW_DLV_OK) {
                        return Err(Error::LibdwarfApiFailure);
                    }
                    auto result = getTypeDetails(cuIndex, typeOff);
                    if (result.isErr()) {
                        return Err(result.unwrapErr());
                    }
                    auto resultX = result.unwrap();
                    VariableNode node;
                    // node.address=// TODO: Addressing scheme?
                    node.displayName = QString(nameStr);
                    node.displayTypeName = resultX.displayName;
                    node.iconType = dwarfTagToIconType(resultX.dwarfTag);
                    node.typeSpec = typeOff;
                    node.expandable = resultX.expandable;
                    ret.subNodeDetails.append(node);
                }
            NextSibling:
                // Get next sibling
                int dwRet = dwarf_siblingof_b(m_dwarfDbg, child, true, &child, NULL);
                if (dwRet == DW_DLV_NO_ENTRY) {
                    break;
                } else if (dwRet != DW_DLV_OK) {
                    return Err(Error::LibdwarfApiFailure);
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
                dwarf_formref(baseTypeAttr, &baseTypeOff, &isInfo, NULL) != DW_DLV_OK) {
                return Err(Error::LibdwarfApiFailure);
            }
            return tryExpandType(cuIndex, baseTypeOff, ctx, varName);
        }
    }
    return Err(Error::InvalidParameter);
}


SymbolBackend::VariableIconType SymbolBackend::dwarfTagToIconType(Dwarf_Half tag) {
    switch (tag) {
        case DW_TAG_structure_type:
        case DW_TAG_union_type:
        case DW_TAG_class_type:
            return VariableIconType::Structure;
            break;
        case DW_TAG_enumeration_type:
        case DW_TAG_base_type:
            return VariableIconType::Integer;
            break;
        case DW_TAG_array_type:
        case DW_TAG_subrange_type:
            return VariableIconType::Array;
            break;
        case DW_TAG_pointer_type:
            return VariableIconType::Pointer;
            break;
        default:
            return VariableIconType::Unknown;
            break;
    }
}

/***************************************** INTERNAL SLOTS *****************************************/

void SymbolBackend::gdbExited(bool normalExit, int exitCode) {
    if (normalExit) {
        return;
    }

    // GDB crash
    auto intention = QMessageBox::critical(nullptr, tr("GDB crashed"),
                                           tr("GDB crashed (exit code %1). Do you want to restart it?").arg(exitCode),
                                           QMessageBox::Yes | QMessageBox::No);

    if (intention == QMessageBox::Yes) {
        recoverGdbCrash();
    }
}

void SymbolBackend::gdbResponse(uint64_t token, const gdbmi::Response response) {
    if (m_expectedGdbResponseToken == token) {
        m_lastGdbResponse = response;
        m_eventLoop.exit(int(Error::NoError));
    }
}

void SymbolBackend::gdbWaitTimedOut() {
    m_eventLoop.exit(int(Error::GdbResponseTimeout));
}
