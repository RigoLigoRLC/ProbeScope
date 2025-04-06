

#include "expressionevaluator/bytecode.h"
#include <QDebug>
#include <QMetaEnum>
#include <set>
#include <utility>


namespace ExpressionEvaluator {
// =================================================== NOTICE ==========================================================
// When adding an instruction that needs an immediate, you must add it to the sets here as well. And specifically for
// integer immediates, you must add it into respective immediate category (I16, U16, ...etc) and the IntImmSet big set
// together.
static const std::set<uint8_t> IntImmSet{LoadU16, LoadI16,   AddI16,  MulI16,  OffsetI16, LoadU32, LoadI32,  AddI32,
                                         MulI32,  OffsetI32, LoadU64, LoadI64, AddI64,    MulI64,  OffsetI64};
static const std::set<uint8_t> U16ImmSet{LoadU16}, I16ImmSet{LoadI16, AddI16, MulI16, OffsetI16}, U32ImmSet{LoadU32},
    I32ImmSet{LoadI32, AddI32, MulI32, OffsetI32}, U64ImmSet{LoadU64}, I64ImmSet{LoadI64, AddI64, MulI64, OffsetI64};
static const std::set<uint8_t> StrImmSet{LoadBase, BaseLoadScope, BaseMember, TypeLoadScope, TypeLoadType};



bool Bytecode::pushInstruction(Opcode opcode, std::optional<QVariant> immediate) {
    if (!checkIfRequiredImmediateValid(opcode, immediate)) {
        return false;
    }
    if (immediate.has_value()) {
        // We only fit uint16_t::max number of constants
        // FIXME: if it's inlineable we should allow that?
        if (constants.size() > std::numeric_limits<uint16_t>::max()) {
            return false;
        }
        while (instructions.size() % 4 > 1) {
            instructions.push_back(Opcode::Nop);
        }

        // Meta opcodes for integer operations will be handled in handleIntegerImmediates
        if (opcode < MaxOpcodes) {
            instructions.push_back(static_cast<uint8_t>(opcode));
        }

        uint16_t immIndex;
        if (immediate->type() == QVariant::String) {
            immIndex = constants.size();
            constants.push_back(immediate.value());
        } else {
            immIndex = handleIntegerImmediates(opcode, immediate.value());
        }
        instructions.push_back(QByteArray(reinterpret_cast<char *>(&immIndex), 2));
        return true;
    } else {
        instructions.push_back(static_cast<uint8_t>(opcode));
        return true;
    }
}

QString Bytecode::disassemble(bool integerInHex) {
    auto metaEnum = QMetaEnum::fromType<Opcode>();
    QString ret;
    uint16_t immIdx;
    // TODO: rewrite using Bytecode::execute

    auto getImmIndex = [](decltype(instructions.cbegin()) &it) -> auto {
        uint16_t ret = *it;
        ++it;
        ret |= (uint16_t(*it) << 8);
        return ret;
    };

    auto getConstant = [this](uint16_t immIndex) -> QVariant {
        if (constants.size() <= immIndex) {
            return QStringLiteral("<ImmIndexOverflow %1>").arg(immIndex);
        }
        return constants[immIndex];
    };

    for (auto it = instructions.cbegin(); it != instructions.cend(); ++it) {
        uint8_t insn = *it;
        const int base = integerInHex ? 16 : 10;
        auto opName = metaEnum.valueToKey(insn);
        if (opName == nullptr) {
            ret += QStringLiteral("<UnknownInsn %1>\n").arg(uint(*it), 2, 16, QChar('0'));
            continue;
        }
        ret += opName;
        if (IntImmSet.contains(insn)) {
            // Do integer immediate specific processing
            immIdx = getImmIndex(++it);
            if (integerInHex) {
                ret += " 0x";
            } else {
                ret + ' ';
            }
            if (U16ImmSet.contains(insn)) {
                ret += QString::number(immIdx, base);
            } else if (U32ImmSet.contains(insn)) {
                ret += QString::number(getConstant(immIdx).toULongLong(), base);
            } else if (U64ImmSet.contains(insn)) {
                ret += QString::number(getConstant(immIdx).toULongLong(), base);
            } else if (I16ImmSet.contains(insn)) {
                int16_t signedImmIdx;
                memcpy(&signedImmIdx, &immIdx, 2);
                ret += QString::number(signedImmIdx, base);
            } else if (I32ImmSet.contains(insn)) {
                ret += QString::number(getConstant(immIdx).toLongLong(), base);
            } else if (I64ImmSet.contains(insn)) {
                ret += QString::number(getConstant(immIdx).toLongLong(), base);
            } else {
                Q_UNREACHABLE();
            }
        } else if (StrImmSet.contains(insn)) {
            // String immediate specific processing
            immIdx = getImmIndex(++it);
            ret += " \"";
            ret += getConstant(immIdx).toString();
            ret += '"';
        }
        ret += '\n';
    }

    return ret;
}

void Bytecode::execute(ExecutionState &state, std::function<bool(ExecutionState &, Opcode, ImmType)> runner) {
    auto getImmIndex = [this](size_t &offset) -> auto {
        uint16_t ret = instructions.at(offset);
        ++offset;
        ret |= (instructions.at(offset) << 8);
        return ret;
    };

    auto getConstant = [this](uint16_t immIndex) {
        Q_ASSERT(constants.size() > immIndex);
        return constants[immIndex];
    };

    if (state.PC > instructions.size()) {
        qCritical() << "Bytecode execution PC overflow: insn count" << instructions.size() << "PC=" << state.PC;
        qCritical() << disassemble();
        state.PC = 0;
        return;
    }

    // Try execute all instructions if the executor is satisfied
    for (auto &PC = state.PC; PC < instructions.size(); ++PC) {
        // Fetch instruction and immediate
        uint8_t insn = instructions.at(PC);
        ImmType imm{std::nullopt};
        // Resolve immediate
        uint16_t immIdx;
        union {
            int64_t i;
            uint64_t u;
        } SignExtender;
        if (IntImmSet.contains(insn)) {
            // Do integer immediate specific processing
            immIdx = getImmIndex(++PC);
            if (U16ImmSet.contains(insn)) {
                imm = immIdx;
            } else if (U32ImmSet.contains(insn)) {
                imm = getConstant(immIdx).toULongLong();
            } else if (U64ImmSet.contains(insn)) {
                imm = getConstant(immIdx).toULongLong();
            } else if (I16ImmSet.contains(insn)) {
                int16_t signedImmIdx;
                memcpy(&signedImmIdx, &immIdx, 2);
                SignExtender.i = signedImmIdx;
                imm = SignExtender.u;
            } else if (I32ImmSet.contains(insn)) {
                SignExtender.i = getConstant(immIdx).toLongLong();
                imm = SignExtender.u;
            } else if (I64ImmSet.contains(insn)) {
                SignExtender.i = getConstant(immIdx).toLongLong();
                imm = SignExtender.u;
            } else {
                Q_UNREACHABLE();
            }
        } else if (StrImmSet.contains(insn)) {
            // String immediate specific processing
            immIdx = getImmIndex(++PC);
            imm = getConstant(immIdx).toString();
        }

        // Give it to execution engine and see if it wants to continue
        if (!runner(state, Opcode(insn), imm)) {
            // FIXME: halt reason? Finished? Error? MemAccess?
            ++PC;
            break;
        }
    }
}

/***************************************** INTERNAL UTILS *****************************************/

bool Bytecode::checkIfRequiredImmediateValid(Opcode opcode, const std::optional<QVariant> &immediate) {
    switch (opcode) {
        case MetaLoadInt:
        case MetaAddInt:
        case MetaMulInt:
        case MetaOffsetInt:
            if (immediate.has_value() && immediate->type() != QVariant::String &&
                (immediate->canConvert(QMetaType::ULongLong) || immediate->canConvert(QMetaType::LongLong))) {
                return true;
            }
            qCritical() << "Opcode" << opcode << "one INT IMM required, check failed";
            return false;
        case LoadBase:
        case BaseLoadScope:
        case BaseMember:
        case TypeLoadScope:
        case TypeLoadType:
            if (immediate.has_value() && (immediate->type() == QVariant::String)) {
                return true;
            }
            qCritical() << "Opcode" << opcode << "one STR IMM required, check failed";
            return false;
        default: return !immediate.has_value();
    }
}

uint16_t Bytecode::handleIntegerImmediates(Opcode opcode, const QVariant immediate) {
    switch (opcode) {
        case MetaLoadInt: return handleIntegerImmediatesWithUnsignedRange(opcode, immediate);
        case MetaAddInt:
        case MetaMulInt:
        case MetaOffsetInt: return handleIntegerImmediatesWithoutUnsignedRange(opcode, immediate);
        default: Q_UNREACHABLE();
    }
}

uint16_t Bytecode::handleIntegerImmediatesWithUnsignedRange(Opcode opcode, const QVariant &immediate) {
    static const std::map<Opcode, Opcode> MappingI16{
        {MetaLoadInt, LoadI16}
    };
    static const std::map<Opcode, Opcode> MappingU16{
        {MetaLoadInt, LoadU16}
    };
    static const std::map<Opcode, Opcode> MappingI32{
        {MetaLoadInt, LoadI32}
    };
    static const std::map<Opcode, Opcode> MappingU32{
        {MetaLoadInt, LoadU32}
    };
    static const std::map<Opcode, Opcode> MappingI64{
        {MetaLoadInt, LoadI64}
    };
    static const std::map<Opcode, Opcode> MappingU64{
        {MetaLoadInt, LoadU64}
    };
    auto realImm = getIntegerImmediateFromQVariant(immediate); // FIXME: QVariant is stupid, replace with stdvariant
    uint16_t ret;

    Q_ASSERT(MappingI16.contains(opcode));

    if (std::holds_alternative<uint64_t>(realImm)) {
        // Unsigned
        auto imm = std::get<uint64_t>(realImm);
        if (std::in_range<uint16_t>(imm)) {
            ret = uint16_t(imm);
            instructions.push_back(uint8_t(MappingU16.at(opcode)));
        } else if (std::in_range<uint32_t>(imm)) {
            ret = uint16_t(constants.size());
            constants.push_back(immediate);
            instructions.push_back(uint8_t(MappingU32.at(opcode)));
        } else {
            ret = uint16_t(constants.size());
            constants.push_back(immediate);
            instructions.push_back(uint8_t(MappingU64.at(opcode)));
        }
    } else {
        // Signed
        auto imm = std::get<int64_t>(realImm);
        if (std::in_range<int16_t>(imm)) {
            ret = int16_t(imm);
            instructions.push_back(uint8_t(MappingI16.at(opcode)));
        } else if (std::in_range<int32_t>(imm)) {
            ret = uint16_t(constants.size());
            constants.push_back(immediate);
            instructions.push_back(uint8_t(MappingI32.at(opcode)));
        } else {
            ret = uint16_t(constants.size());
            constants.push_back(immediate);
            instructions.push_back(uint8_t(MappingI64.at(opcode)));
        }
    }

    return ret;
}

uint16_t Bytecode::handleIntegerImmediatesWithoutUnsignedRange(Opcode opcode, const QVariant &immediate) {
    static const std::map<Opcode, Opcode> MappingI16{
        {MetaAddInt,    AddI16   },
        {MetaMulInt,    MulI16   },
        {MetaOffsetInt, OffsetI16},
    };
    static const std::map<Opcode, Opcode> MappingI32{
        {MetaAddInt,    AddI32   },
        {MetaMulInt,    MulI32   },
        {MetaOffsetInt, OffsetI32},
    };
    static const std::map<Opcode, Opcode> MappingI64{
        {MetaAddInt,    AddI64   },
        {MetaMulInt,    MulI64   },
        {MetaOffsetInt, OffsetI64},
    };
    auto realImm = getIntegerImmediateFromQVariant(immediate);
    uint16_t ret;

    // If we got an U64 and it can't fit into I64, it will fail
    Q_ASSERT(std::holds_alternative<int64_t>(realImm) || (std::in_range<int64_t>(std::get<uint64_t>(realImm))));
    Q_ASSERT(MappingI16.contains(opcode));

    int64_t imm =
        std::holds_alternative<int64_t>(realImm) ? std::get<int64_t>(realImm) : int64_t(std::get<uint64_t>(realImm));

    if (std::in_range<int16_t>(imm)) {
        ret = int16_t(imm);
        instructions.push_back(uint8_t(MappingI16.at(opcode)));
    } else if (std::in_range<int32_t>(imm)) {
        ret = uint16_t(constants.size());
        constants.push_back(immediate);
        instructions.push_back(uint8_t(MappingI32.at(opcode)));
    } else {
        ret = uint16_t(constants.size());
        constants.push_back(immediate);
        instructions.push_back(uint8_t(MappingI64.at(opcode)));
    }

    return ret;
}

std::variant<uint64_t, int64_t> Bytecode::getIntegerImmediateFromQVariant(const QVariant &immediate) {
    switch (static_cast<QMetaType::Type>(immediate.type())) {
        case QMetaType::UChar:
        case QMetaType::UShort:
        case QMetaType::UInt:
        case QMetaType::ULong:
        case QMetaType::ULongLong: return uint64_t(immediate.toULongLong());
        case QMetaType::SChar:
        case QMetaType::Short:
        case QMetaType::Int:
        case QMetaType::Long:
        case QMetaType::LongLong: return int64_t(immediate.toLongLong());
        default: Q_UNREACHABLE();
    }
}
} // namespace ExpressionEvaluator
