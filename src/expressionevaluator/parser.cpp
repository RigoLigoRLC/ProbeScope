
#include "expressionevaluator/parser.h"
#include <QDebug>
#include <QMetaEnum>
#include <QStack>
#include <mutex>

extern "C" const TSLanguage *tree_sitter_ProbeScope_Watch_Expr(void);

static TSNode squeezeTree(TSNode node) {
    TSNode ret = node;

    while (ts_node_named_child_count(ret) == 1) {
        ret = ts_node_child(ret, 0);
    }

    return ret;
}

namespace ExpressionEvaluator {

Parser::NodeSymbols Parser::id = {};

Parser::Parser(QObject *parent) : QObject(parent) {}

Parser::~Parser() {}

TSParser *Parser::getTsParser() {
    static std::once_flag initOnce;
    static const TSLanguage *tsLang = nullptr;
    static std::unique_ptr<TSParser, void (*)(TSParser *)> parser(ts_parser_new(),
                                                                  [](TSParser *parser) { ts_parser_delete(parser); });

    std::call_once(initOnce, [&]() {
        tsLang = tree_sitter_ProbeScope_Watch_Expr();
        ts_parser_set_language(parser.get(), tsLang);
#define INIT_TS_SYMBOL(name)                                                                                           \
    id.name = ts_language_symbol_for_name(tsLang, #name, strlen(#name), true);                                         \
    Q_ASSERT(id.name)
        // Initialize token IDs
        INIT_TS_SYMBOL(source_file);
        INIT_TS_SYMBOL(expr);
        INIT_TS_SYMBOL(offset_expr);
        INIT_TS_SYMBOL(cast_expr);
        INIT_TS_SYMBOL(unary_expr);
        INIT_TS_SYMBOL(postfix_expr);
        INIT_TS_SYMBOL(postfix_comp);
        INIT_TS_SYMBOL(pri_expr);
        INIT_TS_SYMBOL(single_eval_block);
        INIT_TS_SYMBOL(det_expr);
        INIT_TS_SYMBOL(mul_det_expr);
        INIT_TS_SYMBOL(unary_det_expr);
        INIT_TS_SYMBOL(pri_det_expr);
        INIT_TS_SYMBOL(sizeof_expr);
        INIT_TS_SYMBOL(type_ident);
        INIT_TS_SYMBOL(scoped_ident);
        INIT_TS_SYMBOL(array_cap);
        INIT_TS_SYMBOL(sym_offset);
        INIT_TS_SYMBOL(sym_unary);
        INIT_TS_SYMBOL(sym_dotmember);
        INIT_TS_SYMBOL(sym_ptrmember);
        INIT_TS_SYMBOL(sym_takeaddr);
        INIT_TS_SYMBOL(sym_plus);
        INIT_TS_SYMBOL(sym_minus);
        INIT_TS_SYMBOL(sym_deref);
        INIT_TS_SYMBOL(sym_namespace);
        INIT_TS_SYMBOL(ident);
        INIT_TS_SYMBOL(num_lit);
        INIT_TS_SYMBOL(num_dec);
        INIT_TS_SYMBOL(num_hex);
        INIT_TS_SYMBOL(num_bin);
        INIT_TS_SYMBOL(ERROR);

#undef INIT_TS_SYMBOL
    });

    return parser.get();
}

Result<QString, QString> Parser::treeSitter(QString expression) {
    auto parser = getTsParser();
    auto utf8 = expression.toUtf8();
    auto tree = ts_parser_parse_string(parser, nullptr, utf8.data(), utf8.size());
    auto root = ts_tree_root_node(tree);
    auto node = squeezeTree(root);
    auto sexpr = ts_node_string(node);


    QString ret(sexpr);
    free(sexpr);
    ts_tree_delete(tree);

    return Ok(ret);
}

Result<Bytecode, QString> Parser::parseToBytecode(QString expression) {
    auto parser = getTsParser();
    auto utf8 = expression.toUtf8();
    auto tree = ts_parser_parse_string(parser, nullptr, utf8.data(), utf8.size());
    auto root = ts_tree_root_node(tree);

    if (ts_node_has_error(root)) {
        return Err(tr("Parse failure"));
    }

    auto cursor = ts_tree_cursor_new(root);
    while (ts_node_symbol(ts_tree_cursor_current_node(&cursor)) != id.expr) {
        if (!ts_tree_cursor_goto_first_child(&cursor)) {
            return Err(tr("Parser internal failure: \"expr\" node not found."));
        }
    }

    struct ParseSession {
        TSParser *parser;
        QByteArray utf8Expr;
        TSTree *tree;
        TSNode root;
        Bytecode ret;
        NodeSymbols &symbols;
        bool definedBase;

        using PassResult = Result<void, QString>;

        ParseSession(NodeSymbols &symbols, TSParser *parser, QString expression)
            : symbols(symbols), parser(parser), utf8Expr(expression.toUtf8()), definedBase(false) {
            tree = ts_parser_parse_string(parser, nullptr, utf8Expr.data(), utf8Expr.size());
            root = ts_tree_root_node(tree);
        }

        auto nodeToString(TSNode node) {
            // FIXME: the 1st argument cannot use reference because we have to deal with rvalue (directly call this
            // function with a current_node function call.) Think about a solution later.
            const auto start = ts_node_start_byte(node), end = ts_node_end_byte(node);
            return QString::fromUtf8(utf8Expr.mid(start, end - start));
        };

        void goToFirstDeepest(TSTreeCursor &cursor) {
            while (ts_tree_cursor_goto_first_child(&cursor))
                ;
        };

        PassResult handleScopedIdent(TSTreeCursor *origCursor, bool isBase) {
            auto localRoot = ts_tree_cursor_current_node(origCursor);
            auto cursor = ts_tree_cursor_new(localRoot);
            const auto baseDepth = ts_tree_cursor_current_depth(&cursor);
            goToFirstDeepest(cursor);
            auto iterCount = ts_tree_cursor_current_depth(&cursor) - baseDepth;
            forever {
                // Now we walk to the last sibling, that one must be ident
                // When we in innermost layer there's only an ident node; when not, we have
                // scoped_ident-sym_namespace-ident
                while (ts_tree_cursor_goto_next_sibling(&cursor))
                    ;
                // Add this ident node. When iteration count was decremented to zero, it's the top
                // LoadBase/TypeLoadType insn, otherwise it's scope insn
                auto strImm = nodeToString(ts_tree_cursor_current_node(&cursor));
                bool isAtTop = --iterCount == 0;
                Opcode op = isAtTop ? (isBase ? LoadBase : TypeLoadType) : (isBase ? BaseLoadScope : TypeLoadScope);
                ret.pushInstruction(op, strImm);
                // If we're already at top level of this subtree, return. Otherwise goto parent node.
                if (isAtTop) {
                    break;
                } else {
                    ts_tree_cursor_goto_parent(&cursor);
                }
            }
            ts_tree_cursor_delete(&cursor);
            return Ok();
        };

        PassResult defineBase(TSTreeCursor *origCursor) {
            auto node = ts_tree_cursor_current_node(origCursor);
            auto nodeid = ts_node_symbol(node);
            // We always reset scope before defining base or type
            ret.pushInstruction(BaseResetScope, {});
            if (nodeid == id.ident) {
                ret.pushInstruction(LoadBase, nodeToString(node));
            } else if (nodeid == id.scoped_ident) {
                // When outer world called defineBase with a scoped identifier, it passes the root scoped_ident node
                // which we will eventually hit it inside this function here when traversing back the subtree.
                handleScopedIdent(origCursor, true);
            }
            return Ok();
        };

        uint64_t handleNumberLiteral(TSTreeCursor *origCursor) {
            auto cursor = ts_tree_cursor_copy(origCursor);
            ts_tree_cursor_goto_first_child(&cursor);

            auto nodeid = ts_node_symbol(ts_tree_cursor_current_node(&cursor));
            if (nodeid == id.num_dec) {
                // Decimal
                return nodeToString(ts_tree_cursor_current_node(&cursor)).toULongLong();
            }
            // TODO: Hex, Bin
        }

        PassResult evaluateDeterminateSubexpr(TSTreeCursor *origCursor) {
            auto cursor = ts_tree_cursor_copy(origCursor);
            ts_tree_cursor_goto_first_child(&cursor);

            forever {
                // Like a normal expression, determinate subexpressions are too mostly squeezable. Squeeze useless nodes
                auto node = ts_tree_cursor_current_node(&cursor);
                auto nodeid = ts_node_symbol(node);
                if (ts_node_child_count(node) == 1 && nodeid != id.num_lit) {
                    ts_tree_cursor_goto_first_child(&cursor);
                    continue;
                }

                if (nodeid == id.num_lit) {
                    ret.pushInstruction(MetaLoadInt, {handleNumberLiteral(&cursor)});
                    break;
                } else {
                    return Err(tr("Det expr unimplemented"));
                }
            }

            return Ok();
        }

        PassResult handlePostfixExpr(TSTreeCursor *origCursor) {
            auto cursor = ts_tree_cursor_copy(origCursor);
            ts_tree_cursor_goto_first_child(&cursor);

            // First child is a postfix_expr again
            // If child count is 1, drop to simplifyExpr, otherwise call handlePostfixExpr recursively
            auto childCount = ts_node_named_child_count(ts_tree_cursor_current_node(&cursor));
            if (childCount == 1) {
                ts_tree_cursor_goto_first_child(&cursor);
                if (auto result = simplifyExpr(&cursor); result.isErr()) {
                    return Err(result.unwrapErr());
                }
                ts_tree_cursor_goto_parent(&cursor);
            } else {
                handlePostfixExpr(&cursor);
            }

            // Next child is a postfix_comp. Go to its children
            ts_tree_cursor_goto_next_sibling(&cursor);
            ts_tree_cursor_goto_first_child(&cursor);
            if (auto nodeid = ts_node_symbol(ts_tree_cursor_current_node(&cursor)); nodeid == id.sym_dotmember) {
                // Dot member, emit member insn
                ts_tree_cursor_goto_next_sibling(&cursor);
                ret.pushInstruction(BaseMember, nodeToString(ts_tree_cursor_current_node(&cursor)));
            } else if (nodeid == id.sym_ptrmember) {
                // Pointer member, emit deref, then emit member insn
                ts_tree_cursor_goto_next_sibling(&cursor);
                ret.pushInstruction(BaseDeref, {});
                ret.pushInstruction(BaseMember, nodeToString(ts_tree_cursor_current_node(&cursor)));
            } else {
                // Check if next sibling is a determinate subexpression (it has to be)
                ts_tree_cursor_goto_next_sibling(&cursor);
                if (ts_node_symbol(ts_tree_cursor_current_node(&cursor)) != id.det_expr) {
                    return Err(tr("Parser internal failure: Invalid postfix expr (%1)")
                                   .arg(nodeToString(ts_tree_cursor_current_node(origCursor))));
                }

                // Emit an offset insn for this subscript access
                if (auto result = evaluateDeterminateSubexpr(&cursor); result.isErr()) {
                    return Err(result.unwrapErr());
                }
                ret.pushInstruction(Offset, {});

                // Dereference the base type
                ret.pushInstruction(BaseDeref, {});
            }
            return Ok();
        };

        PassResult simplifyExpr(TSTreeCursor *origCursor) {
            // If you want me to simplify an expr, you better really gave me an expr to simplify
            auto tmpid = ts_node_symbol(ts_tree_cursor_current_node(origCursor));
            Q_ASSERT(tmpid == id.expr || tmpid == id.pri_expr);

            // In this loop the expr node tree is squeezed. When the node appeared only as an intermediate for a deeper
            // syntactical structure, it's discarded. Otherwise, we recognize it as something useful and call other
            // handler function.
            auto cursor = ts_tree_cursor_copy(origCursor);
            forever {
                auto node = ts_tree_cursor_current_node(&cursor);
                auto nodeid = ts_node_symbol(node);
                auto childCount = ts_node_child_count(node);

                if (childCount == 1) {
                    // One child means mostly squeezable. One special case is single_eval_block which should emit
                    // special instructions so we give it a special check.
                    if (nodeid != id.single_eval_block) {
                        ts_tree_cursor_goto_first_child(&cursor);
                    } else {
                        // TODO: single_eval_block
                        goto bail_out;
                    }
                } else if ((childCount == 0 && nodeid == id.ident) || (childCount > 1 && nodeid == id.scoped_ident)) {
                    // A freestanding single identifier or a freestanding scoped identifier, recognize it as base
                    if (auto result = defineBase(&cursor); result.isErr()) {
                        return Err(result.unwrapErr());
                    }
                    break;
                } else if (childCount > 1) {
                    // Okay, after the special decision processes, we've met a generic non-squeezable node.
                    if (nodeid == id.postfix_expr) {
                        if (auto result = handlePostfixExpr(&cursor); result.isErr()) {
                            return Err(result.unwrapErr());
                        }
                        break;
                    }
                } else {
                bail_out:
                    // TODO: No other situations are handled here, bail out.
                    ts_tree_cursor_delete(&cursor);
                    return Err(QStringLiteral("Unimplemented"));
                }
            }

            ts_tree_cursor_delete(&cursor);
            return Ok();
        };
    };


    // Now we have an expr node, as a common starting point, start building bytecode
    ParseSession session(id, getTsParser(), expression);

    auto processResult = session.simplifyExpr(&cursor);
    ts_tree_cursor_delete(&cursor);

    // FIXME: When to use BaseEval?
    session.ret.pushInstruction(BaseEval, {});
    session.ret.pushInstruction(ReturnAsBase, {});

    if (processResult.isErr()) {
        return Err(processResult.unwrapErr());
    } else {
        return Ok(session.ret);
    }
}

} // namespace ExpressionEvaluator
