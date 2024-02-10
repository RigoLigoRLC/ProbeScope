
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
        struct ConsoleOutput {
            enum Type_t { unknown, notify, console, log, target, promt };
            ConsoleOutput(const Type_t ty = unknown, const QString &t = {}) : type(ty), text(t) {}
            QString text;
            Type_t type;
        };

        QList<ConsoleOutput> outputs;
        QString message;
        QVariant payload;
        int token = 0;

        Response(const QString &m = {}, const QVariant &p = {}, int tok = -1) : message{m}, payload{p}, token(tok) {}
    };

    QVariantMap parseElements(const QString &str);
    int strToInt(const QString &s, int def);
    Response parse_response(const QString &gdb_mi_text);
}
