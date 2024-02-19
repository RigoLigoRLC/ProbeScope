
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
        ScopedIdentifier, // Scoped identifier, like when taking a type inside a namespace
        Identifier,       // Bare identifier, like when taking member
        IntegerLiteral,   // Can be hex

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

        Arrow, // ->
    };
    static Result<QList<QPair<TokenType, QVariant>>, QString> parseToTokens(QString expression);
};
