
#include "gdbcontainer.h"
#include "gdbmi.h"
#include <QProcess>

GdbContainer::GdbContainer(QObject *parent) : QObject(parent) {
    // Initialize members
    m_nextToken = 0;
    m_gdbExecutablePath = "";
    m_shuttingGdbDown = false;

    connect(&m_gdbProcess, &QProcess::started, this, &GdbContainer::gdbStarted);
    connect(&m_gdbProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            &GdbContainer::gdbProcessFinished);
}

bool GdbContainer::startGdb() {
    // Ensure that the GDB executable path is set
    if (m_gdbExecutablePath.isEmpty()) {
        return false;
    }

    // Start GDB
    m_gdbProcess.start(m_gdbExecutablePath, QStringList() << "--interpreter=mi3");
    m_startupRequested = true;
    return true;
}

/***************************************** INTERNAL UTILS *****************************************/

void GdbContainer::sendCommandInternal(const QString &command, uint64_t token) {
    // GDB MI command like "123-var-create" something something
    m_gdbProcess.write(QString::number(token).toUtf8());
    m_gdbProcess.write(command.toUtf8());
    m_gdbProcess.write(gdbmi::EOL);
}

void GdbContainer::tryParseGdbOutput() {
    // Continuously parse the output until no more responses can be parsed
    while (true) {
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
    m_gdbStdoutBuffer.append(m_gdbProcess.readAllStandardOutput());

    tryParseGdbOutput();
}

void GdbContainer::gdbStarted() {
    // Send pending commands
    for (auto it = m_pendingCommands.begin(); it != m_pendingCommands.end(); ++it) {
        sendCommandInternal(it.value(), it.key());
    }
    m_pendingCommands.clear();
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
