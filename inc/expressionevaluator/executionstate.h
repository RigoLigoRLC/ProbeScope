
#pragma once

#include "typerepresentation.h"
#include <QFlags>
#include <QVector>
#include <cstdint>

namespace ExpressionEvaluator {

struct ExecutionState {
    enum FlagsEnum {

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
};

} // namespace ExpressionEvaluator
