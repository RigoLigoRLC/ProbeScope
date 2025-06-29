
#pragma once

#include "expressionevaluator/executionstate.h"
#include "opcodes.h"
#include <QByteArray>
#include <QVariant>
#include <QVector>
#include <functional>
#include <optional>
#include <variant>

namespace ExpressionEvaluator {

/**
 * @brief Expression bytecode currently only encodes instructions as byte stream, constants are kept as Qt types.
 * Instructions are 32-bit aligned. Normally, opcodes that don't require an immediate are 1-byte long. Opcodes that
 * require an immediate takes 3 bytes, they should sit within 32-bit boundaries, and first byte is opcode and last two
 * bytes are 16-bit little-endian encoded "index". This "index" can either be a 16-bit immediate itself or an index into
 * the associated constant table. Currently
 *
 */
struct Bytecode {
    QByteArray instructions;
    QVector<QVariant> constants;

    using ImmType = std::variant<std::nullopt_t, uint64_t, QString>;

    enum ExecutionResult { Completed, Continue, MemAccess, BeginErrors, ErrorBreak, InvalidPC };

    bool pushInstruction(Opcode opcode, std::optional<QVariant> immediate);
    bool forwardInstruction(Opcode opcode, std::optional<QVariant> immediate);
    QString disassemble(bool integerInHex = true);
    ExecutionResult execute(ExecutionState &state,
                            std::function<ExecutionResult(ExecutionState &, Opcode, ImmType)> runner);
    static ExecutionResult genericComputationExecutor(ExecutionState &es, Opcode op, ImmType imm);

private:
    static bool checkIfRequiredImmediateValid(Opcode opcode, const std::optional<QVariant> &immediate);

    uint16_t handleIntegerImmediates(Opcode opcode, const QVariant immediate);
    uint16_t handleIntegerImmediatesWithUnsignedRange(Opcode opcode, const QVariant &immediate);
    uint16_t handleIntegerImmediatesWithoutUnsignedRange(Opcode opcode, const QVariant &immediate);
    uint16_t handleBitfieldOperationImmediates(Opcode, const QVariant &immediate);

    std::variant<uint64_t, int64_t> getIntegerImmediateFromQVariant(const QVariant &immediate);
};

} // namespace ExpressionEvaluator
