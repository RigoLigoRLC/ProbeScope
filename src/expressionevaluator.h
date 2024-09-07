
#pragma once

#include <QList>
#include <QString>
#include <QVariant>
#include <result.h>

struct TSLanguage;

class ExpressionEvaluator : public QObject {
    Q_OBJECT
public:
    ExpressionEvaluator(QObject *parent = nullptr);
    ~ExpressionEvaluator();
    enum class TokenType {
        // Special
        EndOfFile = 0,
        Identifier,
        IntegerLiteral, // Can be hex

        // Keywords
        Sizeof, // "sizeof"

        // Used in pairs
        OpeningParenthesis, // (
        ClosingParenthesis, // )
        OpeningBracket,     // [
        ClosingBracket,     // ]
        OpeningBrace,       // {
        ClosingBrace,       // }

        Ampersand, // &
        Asterisk,  // *
        Plus,      // +
        Minus,     // -
        Dot,       // .

        Arrow,       // ->
        DoubleColon, // ::
    };
    Q_ENUM(TokenType)

    enum LexerState {
        EXPR,
        OFFSET_EXPR,
        OFFSET_EXPR_B,
        UNARY_EXPR,
        UNARY_SYM,
        CAST_EXPR,
        CAST_OPEXPR,
        POSTFIX_EXPR,
        POSTFIX_EXPR_B,
        PRI_EXPR,

        TYPE_IDENT,
        POSTFIX_TYPE_IDENT,
        POSTFIX_TYPE_IDENT_B,
        SCOPED_IDENT,
        SCOPED_IDENT_B,

        DET_EXPR,
        ADD_DET_EXPR_B,
        MUL_DET_EXPR,
        MUL_DET_EXPR_B,
        UNARY_DET_EXPR,
        PRI_DET_EXPR,

        OPEN_PARENTH,
        OPEN_BRACKET,
        OPEN_BRACE,
        CLOSE_PARENTH,
        CLOSE_BRACKET,
        CLOSE_BRACE,
        ARROW,
        DOT,
        DBL_COLON,
        SIZEOF,
        ASTERISK,
        PLUS,
        MINUS,

        // Axioms
        IDENT,
        INT_LIT,

    };
    Q_ENUM(LexerState)

    struct Token {
        TokenType type;
        QVariant embedData;
    };
    static Result<QList<Token>, QString> parseToTokens(QString expression);

    static QString tokenTypeToString(TokenType tt);

    static Result<QString, QString> treeSitter(QString expression);

private:
    static QString lexerStateToString(LexerState ls);

    static void initializeTreeSitter();
    static const TSLanguage* m_tsLang;
    static uint32_t field_type_ident,
                    field_expr;
};
