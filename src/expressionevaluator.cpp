
#include "expressionevaluator.h"

/*
This is the evaluator for watch expressions.
Grammar definition in LL(1) style:

[==== Deterministic expressions ====]

    DET_ADD_SYM ->
        '+' | '-'

    PRI_DET_EXPR -> [Primary Deterministic Expression]
        '('; DET_EXPR; ')' |
        INT_LIT [Integer literal] |
        "sizeof"; '('; IDENT; ')'

    UNARY_DET_EXPR [With unary op] ->
        '-'; PRI_DET_EXPR |
        PRI_DET_EXPR

    MUL_DET_EXPR_B ->
        '*'; UNARY_DET_EXPR; MUL_DET_EXPR_B |
        epsilon

    MUL_DET_EXPR ->
        PRI_DET_EXPR; MUL_DET_EXPR_B

    ADD_DET_EXPR_B ->
        DET_ADD_SYM; PRI_DET_EXPR; ADD_DET_EXPR_B |
        epsilon

    DET_EXPR [Additive Deterministic Expression actually] ->
        PRI_DET_EXPR; ADD_DET_EXPR_B

[==== Atoms ====]
    SCOPED_IDENT_B ->
        "::"; IDENT; SCOPED_IDENT_B |
        epsilon

    SCOPED_IDENT ->
        IDENT; SCOPED_IDENT_B

    TYPE_IDENT_POSTFIX_SYM ->
        '*' [Pointer] | "[]" [Array of]

    POSTFIX_TYPE_IDENT_B ->
        TYPE_IDENT_POSTFIX_SYM; POSTFIX_TYPE_IDENT_B |
        epsilon

    POSTFIX_TYPE_IDENT ->
        SCOPED_IDENT; POSTFIX_TYPE_IDENT_B

    TYPE_IDENT ->
        POSTFIX_TYPE_IDENT

[==== Expressions ====]

    PRI_EXPR ->
        '('; SCOPED_IDENT; ')' |
        IDENT

    POSTFIX_EXPR_B ->
        '.'; IDENT; POSTFIX_EXPR_B |
        "->"; IDENT; POSTFIX_EXPR_B |
        '['; DET_EXPR; ']'; POSTFIX_EXPR_B |
        epsilon

    POSTFIX_EXPR ->
        PRI_EXPR; POSTFIX_EXPR_B

    CAST_EXPR ->
        '('; TYPE_IDENT; ')'; CAST_EXPR; |
        POSTFIX_EXPR

    UNARY_SYM ->
        '*' [Deref] | '&' [Take address] | epsilon

    UNARY_EXPR ->
        UNARY_SYM; CAST_EXPR

    OFFSET_EXPR_B ->
        DET_ADD_SYM [Borrow deterministic symbols];
            DET_EXPR [Offset must be deterministic];
            OFFSET_EXPR_B |
        epsilon

    OFFSET_EXPR ->
        UNARY_EXPR; OFFSET_EXPR_B

    EXPR ->
        OFFSET_EXPR

 */
