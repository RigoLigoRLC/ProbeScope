
#pragma once

#include <QList>
#include <QString>
#include <QVariant>
#include <result.h>


class ExpressionEvaluator {
public:
    enum class TokenType {
        // Special
        EndOfFile = 0,
        Identifier,
        IntegerLiteral, // Can be hex

        // Used in pairs
        OpeningParenthesis, // (
        ClosingParenthesis, // )
        OpeningBracket,     // [
        ClosingBracket,     // ]
        OpeningBrace,       // {
        ClosingBrace,       // }

        Asterisk, // *
        Plus,     // +
        Minus,    // -
        Dot,      // .

        Arrow, // ->
    };
    static Result<QList<QPair<TokenType, QVariant>>, QString> parseToTokens(QString expression);
};
