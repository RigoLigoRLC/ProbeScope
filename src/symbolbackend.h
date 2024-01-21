
#pragma once

#include "gdbcontainer.h"
#include <QEventLoop>
#include <QMap>
#include <QObject>
#include <QProgressDialog>
#include <QString>
#include <QTimer>
#include <result.h>

/**
 * @brief SymbolBacked uses GdbContainer under the hood to provide all the necessary symbol information
 *        for data acquisition. Because it only has to evaluate the expressions very infrequently, it
 *        is designed to run with no concurrency (although GdbContainer can do so).
 *
 *        SymbolBackend also has to handle timeouts and GDB crashes. It will pop a dialog to let the user
 *        know of the situation, and, the recover may be unsuccessful. So others calling its methods should
 *        prepare for an unsuccessful API call.
 *
 *        GDB usually should reply very quickly, so the APIs here are designed to be synchronous.
 *
 *        SymbolBackend can only set GDB executable once. You are expected to construct it with a GDB executable
 *        path most of the time, and if it's OOBE, you should ask user about it and set it immediately. It then
 *        will start GDB automatically (or if it's constructed with a GDB exec, done so in the constructor).
 *        If the user wants to change GDB used, by design the application must be restarted.
 */
class SymbolBackend final : public QObject {
    Q_OBJECT

public:
    explicit SymbolBackend(QString gdbPath, QObject *parent = nullptr);
    ~SymbolBackend();

    enum class Error { NoError, GdbResponseTimeout, GdbStartupFail };
    QString errorString(Error error);

    /**
     * @brief As described in the class brief, this method can only be called once when GDB was not set in constructor.
     *        One exception is that if the GDB path you've set before cannot start GDB, you have one more chance.
     * @note This method will start GDB automatically.
     * @note This method is also what used in constructor.
     *
     * @param gdbPath GDB executable path
     * @return true GDB executable path was set properly
     * @return false GDB executable path is already set and is already valid
     */
    bool setGdbExecutableLazy(QString gdbPath);

    /**
     * @brief Get the Gdb Terminal Device object. This QIODevice is only readable and is meant to display GDB activity.
     *
     * @return QIODevice* GDB Terminal device
     */
    GdbReadonlyDevice *getGdbTerminalDevice() { return m_gdb.getGdbTerminalDevice(); }

    Error switchSymbolFile(QString symbolFileFullPath);

private:
    void warnUnstartedGdb();
    Result<gdbmi::Response, Error> retryableGdbCommand(QString command, int timeout = 5000);
    Result<gdbmi::Response, Error> waitGdbResponse(uint64_t token, int timeout = 5000);
    void recoverGdbCrash();

private slots:
    void gdbExited(bool normalExit, int exitCode);
    void gdbResponse(uint64_t token, const gdbmi::Response response);
    void gdbWaitTimedOut();

private:
    GdbContainer m_gdb;
    QString m_symbolFileFullPath;
    bool m_gdbProperlySet;

    QEventLoop m_eventLoop;
    QTimer m_waitTimer;
    QProgressDialog m_progressDialog;

    gdbmi::Response m_lastGdbResponse;
    uint64_t m_expectedGdbResponseToken;
};
