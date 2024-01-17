
#pragma once

#include <QString>
#include <QVariant>

namespace gdbmi {
#ifdef Q_OS_WIN
    constexpr const char *EOL = "\r\n";
#else
    constexpr const char *EOL = "\n";
#endif

    constexpr const char *EndOfResponse = "(gdb)";

    QString escapedText(const QString &s);

    struct Response {
        enum Type_t { unknown, notify, result, console, log, target, promt } type;

        QString message;
        QVariant payload;
        int token = 0;

        Response(Type_t t = unknown, const QString &m = {}, const QVariant &p = {}, int tok = -1)
            : type{t}, message{m}, payload{p}, token(tok) {}

        bool isValid() const { return type != unknown; }
    };

    QVariantMap parseElements(const QString &str);
    int strToInt(const QString &s, int def);
    Response parse_response(const QString &gdb_mi_text);
}
