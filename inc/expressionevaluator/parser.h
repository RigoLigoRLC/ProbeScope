
#pragma once

#include "expressionevaluator/bytecode.h"
#include "expressionevaluator/opcodes.h"
#include <QString>
#include <QVariant>
#include <result.h>
#include <tree_sitter/api.h>


struct TSLanguage;

namespace ExpressionEvaluator {

class Parser : public QObject {
    Q_OBJECT
public:
    Parser(QObject *parent = nullptr);
    ~Parser();

    static Result<QString, QString> treeSitter(QString expression);
    static Result<Bytecode, QString> parseToBytecode(QString expression);


private:
    static TSParser *getTsParser();

    // clang-format off
    // tree-sitter node symbols (symbols are numeric IDs for node types)
    struct NodeSymbols {
        uint32_t source_file;
        uint32_t expr;
        uint32_t offset_expr;
        uint32_t cast_expr;
        uint32_t unary_expr;
        uint32_t postfix_expr;
        uint32_t postfix_comp;
        uint32_t pri_expr;
        uint32_t single_eval_block;
        uint32_t det_expr;
        uint32_t mul_det_expr;
        uint32_t unary_det_expr;
        uint32_t pri_det_expr;
        uint32_t sizeof_expr;
        uint32_t type_ident;
        uint32_t scoped_ident;
        uint32_t array_cap;
        uint32_t sym_offset;
        uint32_t sym_unary;
        uint32_t sym_dotmember;
        uint32_t sym_ptrmember;
        uint32_t sym_takeaddr;
        uint32_t sym_plus;
        uint32_t sym_minus;
        uint32_t sym_deref;
        uint32_t sym_namespace;
        uint32_t ident;
        uint32_t num_lit;
        uint32_t num_dec;
        uint32_t num_hex;
        uint32_t num_bin;
        uint32_t ERROR;
    };
    static NodeSymbols id;
    // clang-format on
};

} // namespace ExpressionEvaluator
