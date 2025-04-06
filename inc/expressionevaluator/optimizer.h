
#pragma once

#include "expressionevaluator/bytecode.h"
#include "expressionevaluator/executionstate.h"
#include "result.h"
#include "symbolbackend.h"

namespace ExpressionEvaluator {

/**
 * @brief This is the static optimization pass for an expression bytecode. After an expression is parsed into bytecode,
 * a static optimization pass is run, with the symbol information. In this pass, ops with string immediates will be
 * eliminated, types and members are converted to offsets, base variable is converted to raw address, etc. Single
 * evaluation block is not touched, though. Since in this pass all information we have are symbol info, dereference
 * ops are not evaluated. The goal is to provide a foundation on which AcquisitionHub can operate without referring to
 * symbol information.
 * When the symbol file of the active project is reloaded, all expression bytecodes must run a static optimization pass
 * again to keep them up-to-date with latest symbol information.
 *
 * @param bytecode Bytecode that just came out of parser
 * @param symbolBackend Symbol backend instance.
 * @return On success: optimized bytecode. On fail: error status.
 */
Result<Bytecode, QString> StaticOptimize(Bytecode &bytecode, SymbolBackend *symbolBackend);

} // namespace ExpressionEvaluator
