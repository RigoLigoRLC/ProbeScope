    
file(GLOB_RECURSE TEST_BYTECODEVM_SOURCES
    *.cpp
    ${PROJECT_SOURCE_DIR}/inc/expressionevaluator/bytecode.h
    ${PROJECT_SOURCE_DIR}/inc/expressionevaluator/executionstate.h
    ${PROJECT_SOURCE_DIR}/inc/expressionevaluator/opcodes.h
    ${PROJECT_SOURCE_DIR}/src/expressionevaluator/bytecode.cpp
    ${PROJECT_SOURCE_DIR}/src/expressionevaluator/executionstate.cpp
)

message("Test sources: ${TEST_BYTECODEVM_SOURCES}")

add_executable(test-bytecodevm)
qm_configure_target(test-bytecodevm
    SOURCES
        ${TEST_BYTECODEVM_SOURCES}

    INCLUDE_PRIVATE
        ${PROJECT_SOURCE_DIR}/inc
    
    LINKS_PRIVATE
        GTest::gtest_main
    
    QT_LINKS
        Core
)

add_test(NAME test-bytecodevm COMMAND test-bytecodevm)
