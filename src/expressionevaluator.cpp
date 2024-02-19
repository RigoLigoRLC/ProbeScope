
#include "expressionevaluator.h"
#include <QStack>

/*
This is the evaluator for watch expressions.
Grammar definition in LL(1) style:

[==== Atoms ====]
    OPEN_PARENTH -> '('
    OPEN_BRACKET -> '['
    OPEN_BRACE -> '{'
    CLOSE_PARENTH -> ')'
    CLOSE_BRACKET -> ']'
    CLOSE_BRACE -> '}'
    ARROW -> "->"
    DOT -> "."
    DBL_COLON -> "::"
    SIZEOF -> "sizeof"
    ASTERISK -> '*'
    PLUS -> '+'
    MINUS -> '-'

[==== Deterministic expressions ====]
    PRI_DET_EXPR -> [Primary Deterministic Expression]
        '('; DET_EXPR; ')' |
        "sizeof"; '('; TYPE_IDENT; ')'
        INT_LIT [Integer literal] |

    UNARY_DET_EXPR [With unary op] ->
        '-'; PRI_DET_EXPR |
        PRI_DET_EXPR

    MUL_DET_EXPR_B ->
        '*'; UNARY_DET_EXPR; MUL_DET_EXPR_B |
        epsilon

    MUL_DET_EXPR ->
        PRI_DET_EXPR; MUL_DET_EXPR_B

    ADD_DET_EXPR_B ->
        '+'; PRI_DET_EXPR; ADD_DET_EXPR_B |
        '-'; PRI_DET_EXPR; ADD_DET_EXPR_B |
        epsilon

    DET_EXPR [Additive Deterministic Expression actually] ->
        PRI_DET_EXPR; ADD_DET_EXPR_B

[==== Basic blocks ====]
    SCOPED_IDENT_B ->
        "::"; IDENT; SCOPED_IDENT_B |
        epsilon

    SCOPED_IDENT ->
        IDENT; SCOPED_IDENT_B

    POSTFIX_TYPE_IDENT_B ->
        '*'; POSTFIX_TYPE_IDENT_B |
        '['; ']'; POSTFIX_TYPE_IDENT_B |
        epsilon

    POSTFIX_TYPE_IDENT ->
        SCOPED_IDENT; POSTFIX_TYPE_IDENT_B

    TYPE_IDENT ->
        POSTFIX_TYPE_IDENT

[==== Expressions ====]

    PRI_EXPR ->
        '('; SCOPED_IDENT; ')' |
        '{'; SCOPED_IDENT; '}' | [Curly braces means eval only once or on user demand]
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
        '+'; DET_EXPR; OFFSET_EXPR_B | [Offset must be deterministic]
        '-'; DET_EXPR; OFFSET_EXPR_B |
        epsilon

    OFFSET_EXPR ->
        UNARY_EXPR; OFFSET_EXPR_B

    EXPR ->
        OFFSET_EXPR; EXPR |
        epsilon

 */


Result<QList<ExpressionEvaluator::Token>, QString> ExpressionEvaluator::parseToTokens(QString expression) {
    QList<Token> ret;

    struct LexerContext {
        QString text, errorMsg;
        size_t length, offset;
        QStack<LexerState> states;
        LexerContext(QString expr) : text(expr), length(expr.size()), offset(0) { push(EXPR); }
        void push(LexerState state) { states.push(state); }
        LexerState pop() { return states.pop(); }
        LexerState top() { return states.top(); }
        QChar take() {
            if (offset >= length) {
                return QChar::Null;
            }
            return text.at(offset++);
        }
        QChar peek() {
            if (offset + 1 >= length) {
                return QChar::Null;
            }
            return text.at(offset + 1);
        }
        bool takeExpect(QChar ch, QString whatExpecteed = "") {
            if (take() != ch) {
                if (whatExpecteed.isEmpty()) {
                    errorMsg = QObject::tr("\"%1\" expected at offset %2").arg(ch).arg(offset);
                } else {
                    errorMsg = QObject::tr("\"%1\" expected at offset %2").arg(whatExpecteed).arg(offset);
                }
                return false;
            }
            return true;
        }
    } ctx(expression);

    forever {
        switch (ctx.pop()) {
            case EXPR: {
                if (ctx.peek() == QChar::Null) {
                    goto finishLoop;
                }
                ctx.push(EXPR);
                ctx.push(OFFSET_EXPR);
                break;
            }
            case OFFSET_EXPR: {
                ctx.push(OFFSET_EXPR_B);
                ctx.push(UNARY_EXPR);
                break;
            }
            case OFFSET_EXPR_B: {
                switch (ctx.peek().toLatin1()) {
                    case '+':
                        ret.append({TokenType::Plus});
                        ctx.push(OFFSET_EXPR_B);
                        ctx.push(DET_EXPR);
                        break;

                    case '-':
                        ret.append({TokenType::Minus});
                        ctx.push(OFFSET_EXPR_B);
                        ctx.push(DET_EXPR);
                        break;
                }
                break;
            }
            case UNARY_EXPR: {
                ctx.push(CAST_EXPR);
                ctx.push(UNARY_SYM);
                break;
            }
            case UNARY_SYM: {
                switch (ctx.peek().toLatin1()) {
                    case '*':
                        ctx.take();
                        ret.append({TokenType::Asterisk});
                        break;
                    case '&':
                        ctx.take();
                        ret.append({TokenType::Ampersand});
                        break;
                }
                break;
            }
            case CAST_EXPR: {
                if (ctx.peek() != '(') {
                    ctx.push(POSTFIX_EXPR);
                    break;
                }
                ctx.push(CAST_EXPR);
                ctx.push(CLOSE_PARENTH);
                ctx.push(TYPE_IDENT);
                ctx.push(OPEN_PARENTH);
                break;
            }
            case POSTFIX_EXPR: {
                ctx.push(POSTFIX_EXPR_B);
                ctx.push(PRI_EXPR);
                break;
            }
            case POSTFIX_EXPR_B: {
                switch (ctx.peek().toLatin1()) {
                    case '.':
                        ctx.push(POSTFIX_EXPR_B);
                        ctx.push(IDENT);
                        ctx.push(DOT);
                        break;
                    case '-':
                        ctx.push(POSTFIX_EXPR_B);
                        ctx.push(IDENT);
                        ctx.push(ARROW);
                        break;
                    case '[':
                        ctx.push(POSTFIX_EXPR_B);
                        ctx.push(CLOSE_BRACKET);
                        ctx.push(DET_EXPR);
                        ctx.push(OPEN_BRACKET);
                        break;
                }
                break;
            }
            case PRI_EXPR: {
                switch (ctx.peek().toLatin1()) {
                    case '(':
                        ctx.push(CLOSE_PARENTH);
                        ctx.push(SCOPED_IDENT);
                        ctx.push(OPEN_PARENTH);
                        break;
                    case '{':
                        ctx.push(CLOSE_BRACE);
                        ctx.push(SCOPED_IDENT);
                        ctx.push(OPEN_BRACE);
                        break;
                    default:
                        ctx.push(IDENT);
                }
                break;
            }
            case TYPE_IDENT: {
                ctx.push(POSTFIX_TYPE_IDENT);
                break;
            }
            case POSTFIX_TYPE_IDENT: {
                ctx.push(POSTFIX_TYPE_IDENT_B);
                ctx.push(SCOPED_IDENT);
                break;
            }
            case POSTFIX_TYPE_IDENT_B: {
                switch (ctx.peek().toLatin1()) {
                    case '*':
                        ctx.push(POSTFIX_TYPE_IDENT_B);
                        ctx.push(ASTERISK);
                        break;
                    case '[':
                        ctx.push(POSTFIX_TYPE_IDENT_B);
                        ctx.push(CLOSE_BRACKET);
                        ctx.push(OPEN_BRACKET);
                        break;
                }
                break;
            }
            case SCOPED_IDENT: {
                ctx.push(SCOPED_IDENT_B);
                ctx.push(IDENT);
                break;
            }
            case SCOPED_IDENT_B: {
                if (ctx.peek() == ':') {
                    ctx.push(SCOPED_IDENT_B);
                    ctx.push(IDENT);
                    ctx.push(DBL_COLON);
                }
                break;
            }
            case DET_EXPR: {
                ctx.push(ADD_DET_EXPR_B);
                ctx.push(PRI_DET_EXPR);
                break;
            }
            case ADD_DET_EXPR_B: {
                switch (ctx.peek().toLatin1()) {
                    case '+':
                        ctx.push(ADD_DET_EXPR_B);
                        ctx.push(PRI_DET_EXPR);
                        ctx.push(PLUS);
                        break;
                    case '-':
                        ctx.push(ADD_DET_EXPR_B);
                        ctx.push(PRI_DET_EXPR);
                        ctx.push(MINUS);
                        break;
                }
                break;
            }
            case MUL_DET_EXPR: {
                ctx.push(MUL_DET_EXPR_B);
                ctx.push(PRI_DET_EXPR);
                break;
            }
            case MUL_DET_EXPR_B: {
                if (ctx.peek() == '*') {
                    ctx.push(MUL_DET_EXPR_B);
                    ctx.push(UNARY_DET_EXPR);
                    ctx.push(ASTERISK);
                }
                break;
            }
            case UNARY_DET_EXPR: {
                if (ctx.peek() == '-') {
                    ctx.push(PRI_DET_EXPR);
                    ctx.push(MINUS);
                } else {
                    ctx.push(PRI_DET_EXPR);
                }
                break;
            }
            case PRI_DET_EXPR: {
                switch (ctx.peek().toLatin1()) {
                    case '(':
                        ctx.push(CLOSE_PARENTH);
                        ctx.push(DET_EXPR);
                        ctx.push(OPEN_PARENTH);
                        break;
                    case 's':
                        ctx.push(CLOSE_PARENTH);
                        ctx.push(TYPE_IDENT);
                        ctx.push(OPEN_PARENTH);
                        ctx.push(SIZEOF);
                        break;
                    default:
                        ctx.push(INT_LIT);
                        break;
                }
                break;
            }

                // Atoms
            case OPEN_PARENTH:
                if (!ctx.takeExpect('('))
                    goto error;
                ret.push_back({TokenType::OpeningParenthesis});
                break;
            case OPEN_BRACKET:
                if (!ctx.takeExpect('['))
                    goto error;
                ret.push_back({TokenType::OpeningBracket});
                break;
            case OPEN_BRACE:
                if (!ctx.takeExpect('{'))
                    goto error;
                ret.push_back({TokenType::OpeningBrace});
                break;
            case CLOSE_PARENTH:
                if (!ctx.takeExpect(')'))
                    goto error;
                ret.push_back({TokenType::ClosingParenthesis});
                break;
            case CLOSE_BRACKET:
                if (!ctx.takeExpect(']'))
                    goto error;
                ret.push_back({TokenType::ClosingBracket});
                break;
            case CLOSE_BRACE:
                if (!ctx.takeExpect('}'))
                    goto error;
                ret.push_back({TokenType::ClosingBrace});
                break;
            case ARROW:
                if (!ctx.takeExpect('-', "->"))
                    goto error;
                if (!ctx.takeExpect('>', "->"))
                    goto error;
                ret.push_back({TokenType::Arrow});
                break;
            case DOT:
                if (!ctx.takeExpect('.'))
                    goto error;
                ret.push_back({TokenType::Dot});
                break;
            case DBL_COLON:
                if (!ctx.takeExpect(':', "::"))
                    goto error;
                if (!ctx.takeExpect(':', "::"))
                    goto error;
                ret.push_back({TokenType::DoubleColon});
                break;
            case SIZEOF: {
                constexpr auto cmp = "sizeof";
                for (const char *it = cmp; *it; it++) {
                    if (!ctx.takeExpect(*it, "sizeof")) {
                        goto error;
                    }
                }
                ret.push_back({TokenType::Sizeof});
                break;
            }
            case ASTERISK:
                if (!ctx.takeExpect('*'))
                    goto error;
                ret.push_back({TokenType::Asterisk});
                break;
            case PLUS:
                if (!ctx.takeExpect('+'))
                    goto error;
                ret.push_back({TokenType::Plus});
                break;
            case MINUS:
                if (!ctx.takeExpect('-'))
                    goto error;
                ret.push_back({TokenType::Minus});
                break;

                // Axioms
            case IDENT: {
                auto peek = ctx.peek();
                QString ident;
                // First character: /[A-Za-z_]/
                if (peek.isNumber() && !peek.isLetter() && peek != '_') {
                    ctx.errorMsg = QObject::tr("Identifier expected at offset %1").arg(ctx.offset);
                    goto error;
                }
                ident.append(ctx.take());
                peek = ctx.peek();
                // Following: /[A-Za-z0-9_]*/
                while (peek.isLetterOrNumber() || peek == '_') {
                    ident.append(ctx.take());
                    peek = ctx.peek();
                }
                ret.append({TokenType::Identifier, ident});
                break;
            }
            case INT_LIT: {
                break;
            }
        }
    }
finishLoop:

error:
}
