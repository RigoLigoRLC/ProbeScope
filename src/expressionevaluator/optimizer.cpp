
#include "expressionevaluator/optimizer.h"

namespace ExpressionEvaluator {

Result<Bytecode, QString> ConstantFolding(Bytecode &bytecode) {
    Bytecode ret;
    ExecutionState es;

    union {
        uint64_t u;
        int64_t i;
    } tmp1, tmp2;
    bytecode.execute(es, [&](ExecutionState &es, Opcode op, Bytecode::ImmType imm) -> Bytecode::ExecutionResult {
        switch (op) {
            case Nop: return Bytecode::Continue;
            case LoadI16:
            case LoadU16:
            case LoadI32:
            case LoadU32:
            case LoadI64:
            case LoadU64: es.stack.push_back(std::get<uint64_t>(imm)); return Bytecode::Continue;
            case Add: {
                uint64_t a = es.stack.takeLast(), b = es.stack.takeLast();
                es.stack.push_back(b + a);
                return Bytecode::Continue;
            }
            case AddI16:
            case AddI32:
            case AddI64: es.stack.back() += std::get<uint64_t>(imm); return Bytecode::Continue;
            case Mul: {
                tmp1.u = es.stack.takeLast();
                tmp2.u = es.stack.takeLast();
                es.stack.push_back(tmp1.i * tmp2.i);
                return Bytecode::Continue;
            }
            case MulI16:
            case MulI32:
            case MulI64: {
                tmp1.u = es.stack.back();
                tmp2.u = std::get<uint64_t>(imm);
                es.stack.back() = tmp1.i * tmp2.i;
                return Bytecode::Continue;
            }
            default: {
                // When we've met a non-constant-manipulator instruction we should have at most one entry on the stack.
                Q_ASSERT(es.stack.size() <= 1);
                if (es.stack.size()) {
                    ret.pushInstruction(MetaLoadInt, es.stack.takeLast());
                }
                // TODO: this instruction forward code is stupid, refactor it as something simpler
                if (std::holds_alternative<QString>(imm)) {
                    ret.forwardInstruction(op, std::get<QString>(imm));
                } else if (std::holds_alternative<uint64_t>(imm)) {
                    ret.forwardInstruction(op, std::get<uint64_t>(imm));
                } else {
                    ret.forwardInstruction(op, {});
                }
                return Bytecode::Continue;
            }
        }
    });

    return Ok(ret);
}

Result<Bytecode, QString> StaticOptimize(Bytecode &bytecode, SymbolBackend *symbolBackend) {
    QString err;
    Bytecode ret;
    ExecutionState es;

    bool definingBase = false, definingType = false;

    // This "awaitingImm" is used, because in this static optimization pass, Add/Mul/Offset insns that pops two values
    // off the stack and does some operation, should be converted into the Add16/Add32/Add64/etc variants that only
    // modifies the address on the top of the stack, the other operand is in the immediate of such insn. But because we
    // process the instruction flow linearly, meaning we can't look back at "what the last few instructions have done"
    // and our bytecode container doesn't support popping insn either, meaning we must somehow save the second operand
    // and wait until the Add/Mul/Offset insn comes. So we came up with this variable that does exactly this.
    Option<uint64_t> awaitingImm;

    bytecode.execute(es, [&](ExecutionState &es, Opcode op, Bytecode::ImmType imm) -> Bytecode::ExecutionResult {
        switch (op) {
            // Base defining
            case BaseResetScope: es.regBaseScope = symbolBackend->getRootScope(); break;
            case BaseLoadScope: es.regBaseScope = es.regBaseScope->getSubScope(std::get<QString>(imm)); break;
            case LoadBase: {
                uint64_t address;
                auto base = es.regBaseScope->getVariable(std::get<QString>(imm));
                if (!base) {
                    err = QObject::tr("Variable \"%1\" does not exist.").arg(std::get<QString>(imm));
                    return Bytecode::ErrorBreak;
                }
                es.regBaseType = base->type;
                address = base->offset;
                ret.pushInstruction(MetaLoadInt, address);
                break;
            }
            case BaseDeref: {
                // We attempt to acquire a Modification type (array or pointer). Failing is fatal in this case.
                auto modifiedType = std::dynamic_pointer_cast<TypeModified>(es.regBaseType);
                if (!modifiedType) {
                    err = QObject::tr("The type being dereferenced is not pointer or array type");
                    return Bytecode::ErrorBreak;
                }
                es.regBaseType = modifiedType->getOperated(IType::Operation::Deref).unwrap();
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
                    return Bytecode::ErrorBreak;
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
            // Offsetting. This is used on arrays and pointers, and is syntactically equivalent to adding integers onto
            // a pointer (offsets the address pointed to by N times the element size)
            case Offset: {
                // We attempt to acquire a Modification type (array or pointer). Failing is fatal in this case.
                auto modifiedType = std::dynamic_pointer_cast<TypeModified>(es.regBaseType);
                if (!modifiedType) {
                    err = QObject::tr("You may only add offset a pointer or an array");
                    return Bytecode::ErrorBreak;
                }

                // This getOperated operation doesn't seem to care the actual argument... should this get a FIXME?
                auto baseType = modifiedType->getOperated(IType::Operation::Deref).unwrap();

                // Multiply with the value already on the top of stack
                ret.pushInstruction(MetaMulInt, {baseType->getSizeof()});

                // Do the offset, let the constant folding part clean it up
                ret.pushInstruction(Add, {});
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
                        return Bytecode::ErrorBreak;
                }
                es.regFlags |= ExecutionState::MemberEvaluated;
                break;
            }
            case ReturnAsBase: {
                if (!es.regFlags.testFlag(ExecutionState::MemberEvaluated)) {
                    err = "Internal error: Member not evaluated before return";
                    return Bytecode::ErrorBreak;
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
                    case IType::Kind::Enumeration:
                        err = QObject::tr("Got a non-numeric type on return");
                        return Bytecode::ErrorBreak;
                }
                break;
            }
            default: {
                if (std::holds_alternative<QString>(imm)) {
                    ret.forwardInstruction(op, std::get<QString>(imm));
                } else if (std::holds_alternative<uint64_t>(imm)) {
                    ret.forwardInstruction(op, std::get<uint64_t>(imm));
                } else {
                    ret.forwardInstruction(op, {});
                }
            }
        }
        return Bytecode::Continue;
    });

    if (es.PC != bytecode.instructions.size() && !err.isNull()) {
        return Err(err);
    }

    qDebug() << "Static evaluation pass";
    qDebug().noquote() << ret.disassemble();

    auto constantFolded = ConstantFolding(ret);

    return Ok(constantFolded.unwrap());
}

} // namespace ExpressionEvaluator
