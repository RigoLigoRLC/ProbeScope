
#pragma once

#include "gdbmi.h"
#include "gdbreadonlydevice.h"
#include <QMap>
#include <QProcess>

class GdbContainer final : public QObject {
    Q_OBJECT

public:
    GdbContainer(QObject *parent = nullptr);
    ~GdbContainer();

    void setGdbExecutablePath(const QString &path) { m_gdbExecutablePath = path; }

    bool startGdb();
    bool stopGdb();

    bool isGdbRunning() const { return m_gdbProcess.state() != QProcess::NotRunning; }

    GdbReadonlyDevice *getGdbTerminalDevice() { return &m_gdbTerminalDevice; }

    uint64_t sendCommand(QString command);

private:
    // Private functions

    /**
     * @brief Actually sends a command to GDB, with its assigned token
     *
     * @param command Command string with all args escaped and expanded
     * @param token Assigned token
     */
    void sendCommandInternal(const QString &command, uint64_t token);

    /**
     * @brief Called when stdout is captured and buffered, parse the GDB output and emit signals
     */
    void tryParseGdbOutput();

private slots:
    void gdbStartedSlot();
    void gdbErrorOccurred(QProcess::ProcessError error);
    void gdbProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void gdbProcessReadyReadStandardOutput();
    void gdbProcessReadyReadStandardError(); // Only put it into terminal

signals:
    void gdbStarted(bool successful);
    void gdbExited(bool normalExit, int exitCode);
    void gdbCommandResponse(uint64_t token, const gdbmi::Response response);

private:
    uint64_t m_nextToken;
    QString m_gdbExecutablePath;
    QProcess m_gdbProcess;

    /**
     * @brief Buffered GDB stdout data. Parsing is only done when gdb returns a "(gdb)" prompt.
     */
    QByteArray m_gdbStdoutBuffer;

    /**
     * @brief Contains pending commands to be executed *after* GDB has started,
     *        meaning this is only used when GDB start was requested but not running yet.
     */
    QMap<uint64_t, QString> m_pendingCommands;

    /**
     * @brief True once startGdb was called until GDB exits.
     */
    bool m_startupRequested;

    /**
     * @brief Only true when we actually want to shut down gdb (and exit has been sent).
     *        This is used to tell if GDB exits unexpectedly.
     */
    bool m_shuttingGdbDown;

    /**
     * @brief Used to read GDB terminal in readonly mode
     */
    GdbReadonlyDevice m_gdbTerminalDevice;
};
