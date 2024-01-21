
#include "gdbcontainer.h"
#include "gdbmi.h"
#include <QProcess>
#include <cstdint>

GdbContainer::GdbContainer(QObject *parent) : QObject(parent) {
    // Initialize members
    m_nextToken = 0;
    m_gdbExecutablePath = "";
    m_shuttingGdbDown = false;

    connect(&m_gdbProcess, &QProcess::started, this, &GdbContainer::gdbStartedSlot);
    connect(&m_gdbProcess, &QProcess::readyReadStandardOutput, this, &GdbContainer::gdbProcessReadyReadStandardOutput);
    connect(&m_gdbProcess, &QProcess::readyReadStandardOutput, &m_gdbTerminalDevice, &GdbReadonlyDevice::readyRead);
    connect(&m_gdbProcess, &QProcess::readyReadStandardError, &m_gdbTerminalDevice, &GdbReadonlyDevice::readyRead);
    connect(&m_gdbProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            &GdbContainer::gdbProcessFinished);
}

GdbContainer::~GdbContainer() {
    // Shut down GDB if it's running
    if (m_gdbProcess.state() != QProcess::NotRunning) {
        m_shuttingGdbDown = true;
        m_gdbProcess.terminate();
        m_gdbProcess.waitForFinished();
    }
}

bool GdbContainer::startGdb() {
    // Ensure that the GDB executable path is set
    if (m_gdbExecutablePath.isEmpty()) {
        return false;
    }

    // Start GDB
    m_gdbProcess.start(m_gdbExecutablePath, QStringList() << "--interpreter=mi3");
    m_gdbProcess.write(gdbmi::EOL);
    m_startupRequested = true;
    return true;
}

bool GdbContainer::stopGdb() {
    // Ensure that GDB is running
    if (m_gdbProcess.state() == QProcess::NotRunning) {
        return false;
    }

    // Stop GDB
    m_shuttingGdbDown = true;
    sendCommand(QString("-gdb-exit") + gdbmi::EOL);
    m_gdbProcess.waitForFinished(500);
    if (m_gdbProcess.state() != QProcess::NotRunning) {
        // Disconnect our alert first
        disconnect(&m_gdbProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
                   &GdbContainer::gdbProcessFinished);
        m_gdbProcess.kill();
        m_gdbProcess.waitForFinished();
    }
    return true;
}

uint64_t GdbContainer::sendCommand(QString command) {
    // If GDB is not running, buffer the command
    if (m_gdbProcess.state() == QProcess::NotRunning) {
        // Generate a token
        auto token = m_nextToken++;

        // Buffer the command
        m_pendingCommands.insert(token, command);

        // Start GDB if not started yet
        if (!m_startupRequested) {
            startGdb();
        }

        // Return the token
        return token;
    }

    // If GDB is running, send the command
    auto token = m_nextToken++;
    sendCommandInternal(command, token);
    return token;
}

/***************************************** INTERNAL UTILS *****************************************/

void GdbContainer::sendCommandInternal(const QString &command, uint64_t token) {
    // GDB MI command like "123-var-create" something something
    // Concatenate it first since we still need to write to terminal
    QString commandWithToken = QString::number(token) + command + gdbmi::EOL;

    m_gdbProcess.write(commandWithToken.toUtf8());
    m_gdbTerminalDevice.internalAddData(commandWithToken.toUtf8());
}

void GdbContainer::tryParseGdbOutput() {
    // Continuously parse the output until no more responses can be parsed
    auto beginIt = m_gdbStdoutBuffer.begin();

    while (true) {
        // Consume and drop non-response lines
        constexpr const char *NonResponsePrefixes = "&~@*+=";

        while (beginIt != m_gdbStdoutBuffer.end() && strchr(NonResponsePrefixes, *beginIt) != nullptr) {
            beginIt = std::search(beginIt, m_gdbStdoutBuffer.end(), gdbmi::EOL, gdbmi::EOL + strlen(gdbmi::EOL));
            if (beginIt != m_gdbStdoutBuffer.end()) {
                ++beginIt;
            }
        }

        // Search for end of response
        auto searchResult = std::search(m_gdbStdoutBuffer.begin(), m_gdbStdoutBuffer.end(), gdbmi::EndOfResponse,
                                        gdbmi::EndOfResponse + strlen(gdbmi::EndOfResponse));

        // If not found, return
        if (searchResult == m_gdbStdoutBuffer.end()) {
            return;
        }

        // If found, parse the response
        // Trim it to remove EOLs at the ends
        auto response = gdbmi::parse_response(
            QString::fromUtf8(m_gdbStdoutBuffer.data(), searchResult - m_gdbStdoutBuffer.begin()).trimmed());

        // Remove the parsed response from the buffer
        m_gdbStdoutBuffer.remove(0, searchResult - m_gdbStdoutBuffer.begin() + strlen(gdbmi::EndOfResponse));

        // Emit response
        emit gdbCommandResponse(response.token, response);
    }
}

/***************************************** INTERNAL SLOTS *****************************************/

void GdbContainer::gdbProcessReadyReadStandardOutput() {
    auto stdoutNewData = m_gdbProcess.readAllStandardOutput();

    m_gdbStdoutBuffer.append(stdoutNewData);
    m_gdbTerminalDevice.internalAddData(stdoutNewData);

    tryParseGdbOutput();
}

void GdbContainer::gdbProcessReadyReadStandardError() {
    auto stderrNewData = m_gdbProcess.readAllStandardError();

    m_gdbTerminalDevice.internalAddData(stderrNewData);
}

void GdbContainer::gdbStartedSlot() {
    // Send pending commands
    for (auto it = m_pendingCommands.begin(); it != m_pendingCommands.end(); ++it) {
        sendCommandInternal(it.value(), it.key());
    }
    m_pendingCommands.clear();

    emit gdbStarted(true);
}

void GdbContainer::gdbProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    m_startupRequested = false;

    if (!m_shuttingGdbDown || exitStatus != QProcess::NormalExit) {
        // GDB exited unexpectedly
        emit gdbExited(false, exitCode);
        return;
    }

    // GDB exited normally
    emit gdbExited(true, exitCode);
}

void GdbContainer::gdbErrorOccurred(QProcess::ProcessError error) {
    switch (error) {
        case QProcess::FailedToStart:
            emit gdbStarted(false);
            m_startupRequested = false;
            break;
        // Not used
        case QProcess::Crashed:
        case QProcess::Timedout:
        case QProcess::ReadError:
        case QProcess::WriteError:
        case QProcess::UnknownError:
            break;
    }
}
