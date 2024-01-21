
#include "symbolbackend.h"
#include <QMessageBox>
#include <qapplication.h>
#include <qnamespace.h>
#include <qprogressdialog.h>

SymbolBackend::SymbolBackend(QString gdbExecutable, QObject *parent) : QObject(parent) {
    m_symbolFileFullPath = "";
    m_gdbProperlySet = false;
    m_waitTimer.setSingleShot(true);

    // It will pop out automatically when construted, close it immediately
    m_progressDialog.close();

    connect(&m_gdb, &GdbContainer::gdbExited, this, &SymbolBackend::gdbExited);
    connect(&m_gdb, &GdbContainer::gdbCommandResponse, this, &SymbolBackend::gdbResponse);

    // Wait GDB response related
    connect(&m_waitTimer, &QTimer::timeout, this, &SymbolBackend::gdbWaitTimedOut);

    setGdbExecutableLazy(gdbExecutable);
}

SymbolBackend::~SymbolBackend() {
    m_gdb.stopGdb();
}

QString SymbolBackend::errorString(SymbolBackend::Error error) {
    switch (error) {
        case Error::NoError:
            return tr("No error");
        case Error::GdbResponseTimeout:
            return tr("GDB response timeout");
        case Error::GdbStartupFail:
            return tr("GDB startup failed");
        default:
            return tr("Unknown error");
    }
}

bool SymbolBackend::setGdbExecutableLazy(QString gdbPath) {
    if (m_gdbProperlySet) {
        return false;
    }

    if (gdbPath.isEmpty()) {
        QMessageBox::warning(nullptr, tr("GDB executable path not set"),
                             tr("GDB executable path is not set. Please set it in settings, "
                                "and manually start GDB from menu after this."));
        return false;
    }

    m_gdb.setGdbExecutablePath(gdbPath);

    // See if it starts successfully
    Error startupResult = Error::NoError;
    connect(&m_gdb, &GdbContainer::gdbStarted, [&](bool successful) {
        if (successful) {
            startupResult = Error::NoError;
        } else {
            startupResult = Error::GdbStartupFail;
        }
    });

    if (startupResult != Error::NoError) {
        QMessageBox::critical(nullptr, tr("GDB startup failed"),
                              tr("GDB startup failed. Please set proper GDB executable path "
                                 "and manually start GDB from menu.\n\n"
                                 "Failed GDB executable path: %1\n")
                                  .arg(gdbPath));
        return false;
    } else {
        m_gdbProperlySet = true; // Now you should not tamper with GDB anymore
        return true;
    }
}

SymbolBackend::Error SymbolBackend::switchSymbolFile(QString symbolFileFullPath) {
    m_symbolFileFullPath = symbolFileFullPath;

    // If GDB is not running, start it
    if (!m_gdb.isGdbRunning()) {
        m_gdb.startGdb();
    }

    // If we have a symbol file set, tell GDB to use it
    auto result = retryableGdbCommand(QString("-file-exec-and-symbols \"%1\"").arg(m_symbolFileFullPath));
    return (result.isErr()) ? result.unwrapErr() : Error::NoError;
}

/***************************************** INTERNAL UTILS *****************************************/

void SymbolBackend::recoverGdbCrash() {
    m_gdb.startGdb();

    // If we have a symbol file set, tell GDB to use it
    waitGdbResponse(m_gdb.sendCommand(QString("-file-exec-and-symbols \"%1\"").arg(m_symbolFileFullPath)));
}

Result<gdbmi::Response, SymbolBackend::Error> SymbolBackend::retryableGdbCommand(QString command, int timeout) {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    forever {
        auto token = m_gdb.sendCommand(command);
        auto result = waitGdbResponse(token, timeout);

        // The GDB command somehow never returned
        if (result.isErr()) {
            auto intention = QMessageBox::critical(nullptr, tr("GDB command unsuccessful"),
                                                   tr("The underlying GDB command was unsuccessful, status: %1.\n"
                                                      "Do you want to retry?\n\n"
                                                      "Command: %2")
                                                       .arg(errorString(result.unwrapErr()), command),
                                                   QMessageBox::Yes | QMessageBox::No);

            if (intention == QMessageBox::Yes) {
                continue;
            } else {
                QApplication::restoreOverrideCursor();
                return result;
            }
        } else {
            // Do a simple check on the result class, and only pop a dialog when there is an error
            auto response = result.unwrap();

            if (response.message == "error") {
                QMessageBox::critical(nullptr, tr("GDB Command Error"),
                                      tr("GDB command error: %1\n\nCommand: %2")
                                          .arg(response.payload.toMap().value("msg").toString(), command));
            }

            QApplication::restoreOverrideCursor();
            return result;
        }
    }
}

Result<gdbmi::Response, SymbolBackend::Error> SymbolBackend::waitGdbResponse(uint64_t token, int timeout) {
    m_expectedGdbResponseToken = token;
    m_waitTimer.start(timeout);
    auto eventLoopRet = SymbolBackend::Error(m_eventLoop.exec());
    // Event loop takes over, until timed out or GDB responded

    // Kill the timer, it's useless now
    m_waitTimer.stop();

    if (eventLoopRet != Error::NoError) {
        return Err(eventLoopRet);
    } else {
        return Ok(m_lastGdbResponse);
    }
}

/***************************************** INTERNAL SLOTS *****************************************/

void SymbolBackend::gdbExited(bool normalExit, int exitCode) {
    if (normalExit) {
        return;
    }

    // GDB crash
    auto intention = QMessageBox::critical(nullptr, tr("GDB crashed"),
                                           tr("GDB crashed (exit code %1). Do you want to restart it?").arg(exitCode),
                                           QMessageBox::Yes | QMessageBox::No);

    if (intention == QMessageBox::Yes) {
        recoverGdbCrash();
    }
}

void SymbolBackend::gdbResponse(uint64_t token, const gdbmi::Response response) {
    if (m_expectedGdbResponseToken == token) {
        m_lastGdbResponse = response;
        m_eventLoop.exit(int(Error::NoError));
    }
}

void SymbolBackend::gdbWaitTimedOut() {
    m_eventLoop.exit(int(Error::GdbResponseTimeout));
}
