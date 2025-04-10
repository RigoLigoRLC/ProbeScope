
#include "expressionevaluator/optimizer.h"

namespace ExpressionEvaluator {

Result<Bytecode, QString> StaticOptimize(Bytecode &bytecode, SymbolBackend *symbolBackend) {
    QString err;
    Bytecode ret;
    ExecutionState es;

    bool definingBase = false, definingType = false;
    bytecode.execute(es, [&](ExecutionState &es, Opcode op, Bytecode::ImmType imm) -> bool {
        switch (op) {
            // Base defining
            case BaseResetScope: es.regBaseScope = symbolBackend->getRootScope(); break;
            case BaseLoadScope: es.regBaseScope = es.regBaseScope->getSubScope(std::get<QString>(imm)); break;
            case LoadBase: {
                uint64_t address;
                auto base = es.regBaseScope->getVariable(std::get<QString>(imm));
                es.regBaseType = base->type;
                address = base->offset;
                ret.pushInstruction(MetaLoadInt, address);
                break;
            }
            // Type defining
            case TypeResetScope: es.regTypeScope = symbolBackend->getRootScope(); break;
            case TypeLoadScope: es.regTypeScope = es.regTypeScope->getSubScope(std::get<QString>(imm)); break;
            case TypeLoadType: es.regType = es.regTypeScope->getType(std::get<QString>(imm)); break;
            // Type casting
            case BaseCast: es.regBaseType = es.regType; break;
            // Get member of base, which is essentially adding offset and changing type
            case BaseMember: {
                auto childInfoResult = es.regBaseType->getChild(std::get<QString>(imm));
                if (childInfoResult.isErr()) {
                    err = QObject::tr("%1 does not have a child named %2")
                              .arg(es.regBaseType->fullyQualifiedName(), std::get<QString>(imm));
                    return false;
                }
                auto childInfo = childInfoResult.unwrap();
                if (childInfo.flags & TypeChildInfo::Bitfield) {
                    // Process bitfield
                    ret.pushInstruction(MetaAddInt, childInfo.byteOffset);
                    // Determine how many bytes to read at least
                    auto bytesToRead = (childInfo.bitOffset + childInfo.bitWidth + 7) / 8;
                    Q_ASSERT(bytesToRead <= 8);
                    ret.pushInstruction(
                        (bytesToRead < 8 ? bytesToRead < 4 ? bytesToRead < 2 ? Deref8 : Deref16 : Deref32 : Deref64),
                        {});
                    ret.pushInstruction(LogicalShiftRight, childInfo.bitOffset);
                    ret.pushInstruction(IType::isSignedInteger(childInfo.type) ? MaskBitsSignExtend
                                                                               : MaskBitsZeroExtend,
                                        childInfo.bitWidth);
                    es.regBaseType = childInfo.type;
                    es.regFlags |= ExecutionState::MemberEvaluated;
                } else {
                    // Normal member access
                    ret.pushInstruction(MetaAddInt, childInfo.byteOffset);
                    es.regBaseType = childInfo.type;
                }
                break;
            }
            case BaseEval: {
                if (es.regFlags.testFlag(ExecutionState::MemberEvaluated)) {
                    break;
                }
                switch (es.regBaseType->kind()) {
                    case IType::Kind::Uint8:
                    case IType::Kind::Sint8: ret.pushInstruction(Deref8, {}); break;
                    case IType::Kind::Uint16:
                    case IType::Kind::Sint16: ret.pushInstruction(Deref16, {}); break;
                    case IType::Kind::Uint32:
                    case IType::Kind::Sint32:
                    case IType::Kind::Float32: ret.pushInstruction(Deref32, {}); break;
                    case IType::Kind::Uint64:
                    case IType::Kind::Sint64:
                    case IType::Kind::Float64: ret.pushInstruction(Deref64, {}); break;
                    case IType::Kind::Unsupported:
                    case IType::Kind::Structure:
                    case IType::Kind::Union:
                    case IType::Kind::Enumeration:
                        err = QObject::tr("Got a non-numeric type on evaluation");
                        return false;
                }
                es.regFlags |= ExecutionState::MemberEvaluated;
                break;
            }
            case ReturnAsBase: {
                if (!es.regFlags.testFlag(ExecutionState::MemberEvaluated)) {
                    err = "Internal error: Member not evaluated before return";
                    return false;
                }
                switch (es.regBaseType->kind()) {
                    case IType::Kind::Uint8: ret.pushInstruction(ReturnU8, {}); break;
                    case IType::Kind::Sint8: ret.pushInstruction(ReturnI8, {}); break;
                    case IType::Kind::Uint16: ret.pushInstruction(ReturnU16, {}); break;
                    case IType::Kind::Sint16: ret.pushInstruction(ReturnI16, {}); break;
                    case IType::Kind::Uint32: ret.pushInstruction(ReturnU32, {}); break;
                    case IType::Kind::Sint32: ret.pushInstruction(ReturnI32, {}); break;
                    case IType::Kind::Float32: ret.pushInstruction(ReturnF32, {}); break;
                    case IType::Kind::Uint64: ret.pushInstruction(ReturnU64, {}); break;
                    case IType::Kind::Sint64: ret.pushInstruction(ReturnI64, {}); break;
                    case IType::Kind::Float64: ret.pushInstruction(ReturnF64, {}); break;
                    case IType::Kind::Unsupported:
                    case IType::Kind::Structure:
                    case IType::Kind::Union:
                    case IType::Kind::Enumeration: err = QObject::tr("Got a non-numeric type on return"); return false;
                }
                break;
            }
            default: {
                if (std::holds_alternative<QString>(imm)) {
                    ret.pushInstruction(op, std::get<QString>(imm));
                } else if (std::holds_alternative<uint64_t>(imm)) {
                    ret.pushInstruction(op, std::get<uint64_t>(imm));
                } else {
                    ret.pushInstruction(op, {});
                }
            }
        }
        return true;
    });

    if (es.PC != bytecode.instructions.size() && !err.isNull()) {
        return Err(err);
    }

    return Ok(ret);
}

} // namespace ExpressionEvaluator
