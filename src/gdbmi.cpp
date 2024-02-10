
#include "gdbmi.h"
#include <QRegularExpression>

// ProbeScope:
// Code taken from and based on https://github.com/martinribelotta/gdbfrontend/blob/master/debugmanager.cpp

namespace gdbmi {
    QString escapedText(const QString &s) {
        return QString{s}.replace(QRegularExpression{R"(\\(.))"}, "\1");
    }

    // Partial code is from pygdbmi
    // See on: https://github.com/cs01/pygdbmi/blob/master/pygdbmi/gdbmiparser.py

    struct {
        const QRegularExpression::PatternOption DOTALL = QRegularExpression::DotMatchesEverythingOption;
        QRegularExpression compile(const QString &text,
                                   QRegularExpression::PatternOption flags = QRegularExpression::NoPatternOption) {
            return QRegularExpression{text, QRegularExpression::MultilineOption | flags};
        }
    } re;

    // GDB machine interface output patterns to match
    // https://sourceware.org/gdb/onlinedocs/gdb/GDB_002fMI-Stream-Records.html#GDB_002fMI-Stream-Records

    // https://sourceware.org/gdb/onlinedocs/gdb/GDB_002fMI-Result-Records.html#GDB_002fMI-Result-Records
    // In addition to a number of out-of-band notifications,
    // the response to a gdb/mi command includes one of the following result indications:
    // done, running, connected, error, exit
    const auto _GDB_MI_RESULT_RE = re.compile(R"(^(\d*)\^(\S+?)(?:,(.*))?$)");

    // https://sourceware.org/gdb/onlinedocs/gdb/GDB_002fMI-Async-Records.html#GDB_002fMI-Async-Records
    // Async records are used to notify the gdb/mi client of additional
    // changes that have occurred. Those changes can either be a consequence
    // of gdb/mi commands (e.g., a breakpoint modified) or a result of target activity
    // (e.g., target stopped).
    const auto _GDB_MI_NOTIFY_RE = re.compile(R"(^(\d*)[*=](\S+?),(.*)$)");

    // https://sourceware.org/gdb/onlinedocs/gdb/GDB_002fMI-Stream-Records.html#GDB_002fMI-Stream-Records
    // "~" string-output
    // The console output stream contains text that should be displayed
    // in the CLI console window. It contains the textual responses to CLI commands.
    const auto _GDB_MI_CONSOLE_RE = re.compile(R"re(~"(.*)")re", re.DOTALL);

    // https://sourceware.org/gdb/onlinedocs/gdb/GDB_002fMI-Stream-Records.html#GDB_002fMI-Stream-Records
    // "&" string-output
    // The log stream contains debugging messages being produced by gdb's internals.
    const auto _GDB_MI_LOG_RE = re.compile(R"re(&"(.*)")re", re.DOTALL);

    // https://sourceware.org/gdb/onlinedocs/gdb/GDB_002fMI-Stream-Records.html#GDB_002fMI-Stream-Records
    // "@" string-output
    // The target output stream contains any textual output from the
    // running target. This is only present when GDB's event loop is truly asynchronous,
    // which is currently only the case for remote targets.
    const auto _GDB_MI_TARGET_OUTPUT_RE = re.compile(R"re(@"(.*)")re", re.DOTALL);

    // Response finished
    const auto _GDB_MI_RESPONSE_FINISHED_RE = re.compile(R"(^\(gdb\)\s*$)");

    const QString _CLOSE_CHARS{"}]\""};

    bool response_is_finished(const QString &gdb_mi_text) {
        // Return true if the gdb mi response is ending
        // Returns: True if gdb response is finished
        return _GDB_MI_RESPONSE_FINISHED_RE.match(gdb_mi_text).hasMatch();
    }

    namespace priv {

        QString::const_iterator &skipspaces(QString::const_iterator &it) {
            while (it->isSpace())
                ++it;
            return it;
        }

        QString parseString(const QString &s, QString::const_iterator &it) {
            QString v;
            while (it != s.cend()) {
                if (*it == '"')
                    break;
                if (*it == '\\')
                    if (++it == s.cend())
                        break;
                v += *it++;
            }
            ++it;
            return v;
        }

        QString parseKey(const QString &str, QString::const_iterator &it) {
            QString key;
            while (it != str.cend()) {
                if (*it == '=')
                    break;
                if (!it->isSpace())
                    key += *it;
                ++it;
            }
            return key;
        }

        QString consumeTo(QChar c, const QString &str, QString::const_iterator &it) {
            QString consumed;
            while (it != str.cend()) {
                if (*it == c) {
                    ++it;
                    break;
                }
                consumed += *it++;
            }
            return consumed;
        }

        QVariantList parseArray(const QString &str, QString::const_iterator &it);
        QVariantMap parseKeyVal(const QString &str, QString::const_iterator &it, QChar terminator = '\0');
        QVariantMap parseDict(const QString &str, QString::const_iterator &it);
        QVariant parseValue(const QString &str, QString::const_iterator &it, QChar terminator);

        QVariantList parseArray(const QString &str, QString::const_iterator &it) {
            QVariantList l;
            while (it != str.cend() && *it != ']') {
                l.append(parseValue(str, it, ']'));
                if (*it == ']') {
                    ++it;
                    break;
                }
                consumeTo(',', str, it);
            }
            return l;
        }

        QVariantMap parseDict(const QString &str, QString::const_iterator &it) {
            QVariantMap m = parseKeyVal(str, it, '}');
            ++it;
            return m;
        }

        QVariantMap parseKeyVal(const QString &str, QString::const_iterator &it, QChar terminator) {
            QVariantMap m;
            while (it != str.cend() && *it != terminator) {
                auto k = parseKey(str, skipspaces(it));
                auto v = parseValue(str, skipspaces(++it), terminator);
                m.insertMulti(k, v);
                if (*it == terminator) {
                    break;
                }
                consumeTo(',', str, it);
            }
            return m;
        }

        QVariant parseValue(const QString &str, QString::const_iterator &it, QChar terminator) {
            if (*it == '"') {
                return parseString(str, ++it);
            } else if (*it == '[') {
                return parseArray(str, ++it);
            } else if (*it == '{') {
                return parseDict(str, ++it);
            }
            if (gdbmi::_CLOSE_CHARS.contains(*it))
                return {};
            return parseKeyVal(str, it, terminator);
        }

    }

    QVariantMap parseElements(const QString &str) {
        auto it = str.cbegin();
        return priv::parseKeyVal(str, it);
    }

    int strToInt(const QString &s, int def) {
        bool ok = false;
        int v = s.toInt(&ok);
        return ok ? v : def;
    }

    Response parse_response(const QString &gdb_mi_text) {
        // Takes in all the output between the command execution and the result ("(gdb)\n" ending string),
        // and spit one response out. The response would contain all the so-to-speak console messages
        // in a list in the order they are printed out and in their unprocessed form (because it's rarely needed),
        // and the command result.

        // Original comments:
        // Parse gdb mi text and turn it into a dictionary.
        // See https://sourceware.org/gdb/onlinedocs/gdb/GDB_002fMI-Stream-Records.html#GDB_002fMI-Stream-Records
        // for details on types of gdb mi output.
        // Args:
        //     gdb_mi_text (str): String output from gdb
        // Returns:
        //    dict with the following keys:
        //    type (either 'notify', 'result', 'console', 'log', 'target', 'done'),
        //    message (str or None),
        //    payload (str, list, dict, or None)
        //
        Response ret;
        QRegularExpressionMatch m;

        // Separate the text by line breaks
        foreach (auto line, gdb_mi_text.split(gdbmi::EOL)) {
            if ((m = _GDB_MI_NOTIFY_RE.match(line)).hasMatch()) {
                ret.outputs.append({Response::ConsoleOutput::notify, line});
            } else if ((m = _GDB_MI_RESULT_RE.match(line)).hasMatch()) {
                ret.message = m.captured(2);
                ret.payload = parseElements(m.captured(3));
                ret.token = strToInt(m.captured(1), -1);
            } else if ((m = _GDB_MI_CONSOLE_RE.match(line)).hasMatch()) {
                ret.outputs.append({Response::ConsoleOutput::console, line});
            } else if ((m = _GDB_MI_LOG_RE.match(line)).hasMatch()) {
                ret.outputs.append({Response::ConsoleOutput::log, line});
            } else if ((m = _GDB_MI_TARGET_OUTPUT_RE.match(line)).hasMatch()) {
                ret.outputs.append({Response::ConsoleOutput::target, line});
            } else if (_GDB_MI_RESPONSE_FINISHED_RE.match(line).hasMatch()) {
                ret.outputs.append({Response::ConsoleOutput::promt, line});
            } else {
                // This was not gdb mi output, so it must have just been printed by
                // the inferior program that's being debugged
                ret.outputs.append({Response::ConsoleOutput::unknown, line});
            }
        }

        return ret;
    }
}