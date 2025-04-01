/// <reference types="tree-sitter-cli/dsl" />
// @ts-check

module.exports = grammar({
    name: 'ProbeScope_Watch_Expr',

    rules: {
        source_file: $ => $.expr,

        expr: $ => $.offset_expr,

        offset_expr: $ => choice(
            $.cast_expr,
            prec(1, seq(
                $.offset_expr,
                $.sym_offset,
                $.det_expr
            ))
        ),

        cast_expr: $ => choice(
            $.unary_expr,
            seq(
                '(',
                field('type_ident', $.type_ident),
                ')',
                $.cast_expr
            )
        ),

        unary_expr: $ => choice(
            $.postfix_expr,
            seq(
                $.sym_unary,
                $.cast_expr
            ),
        ),

        postfix_expr: $ => choice(
            $.pri_expr,
            seq(
                $.postfix_expr,
                $.postfix_comp
            )
        ),

        postfix_comp: $ => choice(
            seq($.sym_dotmember, $.ident),
            seq($.sym_ptrmember, $.ident),
            seq('[', $.det_expr, ']')
        ),

        pri_expr: $ => prec(2, choice(
            seq('(', $.expr, ')'),
            $.single_eval_block,
            $.scoped_ident,
        )),

        single_eval_block: $ => seq(
            '{', $.expr, '}'
        ),

        det_expr: $ => choice(
            $.mul_det_expr,
            seq($.det_expr, $.sym_plus, $.mul_det_expr),
            seq($.det_expr, $.sym_minus, $.mul_det_expr)
        ),

        mul_det_expr: $ => choice(
            $.unary_det_expr,
            seq($.mul_det_expr, '*', $.unary_det_expr)
        ),

        unary_det_expr: $ => choice(
            $.pri_det_expr,
            seq('-', $.pri_det_expr)
        ),
        
        pri_det_expr: $ => choice(
            seq('(', $.det_expr, ')'),
            $.sizeof_expr,
            $.num_lit
        ),

        sizeof_expr: $ => seq(
            'sizeof',
            '(',
            field('type_ident', $.type_ident),
            ')'
        ),

        type_ident: $ => choice(
            $.scoped_ident,
            seq(
                $.type_ident,
                choice(
                    $.sym_deref,
                    $.array_cap
                )
            )
        ),

        scoped_ident: $ => choice(
            $.ident,
            seq(
                $.scoped_ident,
                $.sym_namespace,
                $.ident
            )
        ),

        /*
        type_ident: $ => choice(
            $.ident,
            seq(
                $.type_ident,
                choice(
                    seq($.sym_namespace, $.ident),
                    $.sym_deref,
                    $.array_cap
                )
            )
        ),
        */

        array_cap: $ => seq(
            '[',
            ']'
        ),
        
        sym_offset: $ => choice(
            $.sym_plus,
            $.sym_minus
        ),
        sym_unary: $ => choice(
            $.sym_deref,
            $.sym_takeaddr
        ),
        sym_dotmember: $ => '.',
        sym_ptrmember: $ => '->',
        sym_takeaddr: $ => '&',
        sym_plus: $ => '+',
        sym_minus: $ => '-',
        sym_deref: $ => '*',
        sym_namespace: $ => '::',

        ident: $ => /[a-zA-Z_][a-zA-Z0-9_]*/,
        num_lit: $ =>  choice(
            seq('0x', $.num_hex),
            seq('0b', $.num_bin),
            $.num_dec,
        ),
        num_dec: $ => /[0-9_]+/,
        num_hex: $ => /[0-9a-fA-F_]+/,
        num_bin: $ => /[0-1_]+/,
    }
});
