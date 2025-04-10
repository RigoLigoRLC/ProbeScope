
#pragma once

#include "typerepresentation.h"
#include <QFlags>
#include <QVector>
#include <cstdint>

namespace ExpressionEvaluator {

struct ExecutionState {
    enum FlagsEnum {
        BaseDefined = 0x01,
        InSingleEvalBlock = 0x02,
        SingleEvalBlockDefined = 0x04,
        PendingMemAccess = 0x08,
        MemberEvaluated = 0x10, ///< Member evaluation is performed because last member access got a bitfield.
    };
    Q_DECLARE_FLAGS(Flags, FlagsEnum);

    QVector<uint64_t> stack;
    size_t PC;
    IScope::p regBaseScope;
    IScope::p regTypeScope;
    IType::p regBaseType;
    IType::p regType;
    Flags regFlags;

    ExecutionState() : stack(8, 0), PC(0) {}

    void resetAll() {
        PC = 0;
        stack.clear();
        regFlags = Flags();
    }
};

} // namespace ExpressionEvaluator
