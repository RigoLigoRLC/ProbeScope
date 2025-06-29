
#include "expressionevaluator/executionstate.h"
#include "expressionevaluator/opcodes.h"
#include <gtest/gtest.h>
#include <expressionevaluator/bytecode.h>

using namespace ExpressionEvaluator;

uint64_t ExecuteForU64(Bytecode &bc) {
    ExecutionState es;
    bc.execute(es, Bytecode::genericComputationExecutor);
    return es.stack.back();
}

TEST(TestBytecodeVM, TestSignExtend) {
    Bytecode bc;
    bc.pushInstruction(MetaLoadInt, {0x4001});
    bc.pushInstruction(MaskBitsSignExtend, {15});
    bc.pushInstruction(ReturnU64, {});
    EXPECT_EQ(ExecuteForU64(bc), 0xffffffffffffc001);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
