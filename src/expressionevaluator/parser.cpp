
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

    // Now we have an expr node, as a common starting point, start building bytecode
    Bytecode ret;
    bool definedBase = false;

    auto nodeToString = [&](TSNode node) -> QString {
        // FIXME: the 1st argument cannot use reference because we have to deal with rvalue (directly call this function
        // with a current_node function call.) Think about a solution later.
        const auto start = ts_node_start_byte(node), end = ts_node_end_byte(node);
        return QString::fromUtf8(utf8.mid(start, end - start));
    };

    auto goToFirstDeepest = [&](TSTreeCursor &cursor) {
        while (ts_tree_cursor_goto_first_child(&cursor))
            ;
    };

    auto handleScopedIdent = [&](TSTreeCursor *origCursor, bool isBase) {
        auto localRoot = ts_tree_cursor_current_node(origCursor);
        auto cursor = ts_tree_cursor_new(localRoot);
        const auto baseDepth = ts_tree_cursor_current_depth(&cursor);
        goToFirstDeepest(cursor);
        auto iterCount = ts_tree_cursor_current_depth(&cursor) - baseDepth;
        forever {
            // Now we walk to the last sibling, that one must be ident
            // When we in innermost layer there's only an ident node; when not, we have scoped_ident-sym_namespace-ident
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
    };

    auto defineBase = [&](TSTreeCursor *origCursor) -> Result<void, QString> {
        auto node = ts_tree_cursor_current_node(origCursor);
        auto nodeid = ts_node_symbol(node);
        if (nodeid == id.ident) {
            ret.pushInstruction(LoadBase, nodeToString(node));
        } else if (nodeid == id.scoped_ident) {
            // When outer world called defineBase with a scoped identifier, it passes the root scoped_ident node
            // which we will eventually hit it inside this function here when traversing back the subtree.
            handleScopedIdent(origCursor, true);
        }
        return Ok();
    };

    auto simplifyExpr = [&](TSTreeCursor *origCursor) -> Result<void, QString> {
        // If you want me to simplify an expr, you better really gave me an expr to simplify
        Q_ASSERT(ts_node_symbol(ts_tree_cursor_current_node(origCursor)) == id.expr);

        // In this loop the expr node tree is squeezed. When the node appeared only as an intermediate for a deeper
        // syntactical structure, it's discarded. Otherwise, we recognize it as something useful and call other handler
        // function.
        auto cursor = ts_tree_cursor_copy(origCursor);
        forever {
            auto node = ts_tree_cursor_current_node(&cursor);
            auto nodeid = ts_node_symbol(node);
            auto childCount = ts_node_child_count(node);

            if (childCount == 1 && (nodeid != id.single_eval_block)) {
                // One child means mostly squeezable. One special case is single_eval_block which should emit special
                // instructions so we give it a special check.
                ts_tree_cursor_goto_first_child(&cursor);
            } else if ((childCount == 0 && nodeid == id.ident) || (childCount > 1 && nodeid == id.scoped_ident)) {
                // A freestanding single identifier or a freestanding scoped identifier, recognize it as base
                defineBase(&cursor);
                break;
            } else {
                // TODO: No other situations are handled here, bail out.
                ts_tree_cursor_delete(&cursor);
                return Err(QStringLiteral("Unimplemented"));
            }
        }

        ts_tree_cursor_delete(&cursor);
        return Ok();
    };

    auto processResult = simplifyExpr(&cursor);
    ts_tree_cursor_delete(&cursor);

    if (processResult.isErr()) {
        return Err(processResult.unwrapErr());
    } else {
        return Ok(ret);
    }
}

} // namespace ExpressionEvaluator
