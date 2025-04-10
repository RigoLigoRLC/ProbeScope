
#include "probelibhost.h"
#include <QApplication>
#include <QDir>

using namespace probelib;

ProbeLibHost::ProbeLibHost(QObject *parent) : QObject(parent) {
    scanProbeLibs();
}

ProbeLibHost::~ProbeLibHost() {}

void ProbeLibHost::scanProbeLibs() {
    QDir pluginsDir(qApp->applicationDirPath());
    pluginsDir.cd("probelibs");
    for (const QString &fileName : pluginsDir.entryList(QDir::Files)) {
        QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
        QObject *plugin = loader.instance();
        if (plugin) {
            IProbeLib *probeLib = qobject_cast<IProbeLib *>(plugin);
            if (probeLib) {
                qDebug() << "Loaded probe library:" << probeLib->name();
            }
            m_probeLibs.append(probeLib);
        }
    }
}


Result<void, QString> ProbeLibHost::selectProbe(IProbeLib *probeLib, IAvailableProbe::p probe) {
    if (!probeLib) {
        return Err(tr("Invalid probe library"));
    }

    if (!probe) {
        return Err(tr("Invalid probe"));
    }

    if (m_probeSession.has_value()) {
        return Err(tr("A probe session is already active"));
    }

    if (auto selectResult = probeLib->selectProbe(probe); selectResult.isErr()) {
        return Err(tr("Failed to select probe: %1").arg(selectResult.unwrapErr().message));
    } else {
        // Save the choice for later use
        m_currentProbeLib = probeLib;
        m_currentProbe = probe;

        // Clear states
        m_deviceCategories.clear();
        m_currentDevice = 0;
        m_connectionSpeed = 4000; // Default to 4MHz
    }

    return Ok();
}


QVector<probelib::DeviceCategory> ProbeLibHost::availableDevices() const {
    if (!m_currentProbeLib) {
        return {};
    }

    return m_currentProbeLib->supportedDevices();
}

Result<void, QString> ProbeLibHost::selectDevice(size_t deviceID) {
    // This is not checked
    m_currentDevice = deviceID;
    return Ok();
}

Result<void, QString> ProbeLibHost::connect() {
    if (!m_currentProbeLib) {
        return Err(tr("No probe library selected"));
    }

    if (!m_currentProbe) {
        return Err(tr("No probe selected"));
    }

    if (m_probeSession.has_value()) {
        return Err(tr("A probe session is already active"));
    }

    // Prepare connection
    m_currentProbeLib->setConnectionSpeed(m_connectionSpeed);

    if (auto connectResult = m_currentProbeLib->connect(m_currentDevice); connectResult.isErr()) {
        return Err(tr("Failed to connect: %1").arg(connectResult.unwrapErr().message));
    } else {
        m_probeSession = std::unique_ptr<IProbeSession>(connectResult.unwrap());
    }

    return Ok();
}

void ProbeLibHost::disconnect() {
    m_probeSession.reset();
}

bool ProbeLibHost::getExclusiveLock(quintptr token) {
    bool expected = false;
    if (m_exclusivelyLocked.compare_exchange_strong(expected, true)) {
        m_lockerToken = token;
        return true;
    }
    return false;
}

bool ProbeLibHost::releaseExclusiveLock(quintptr token) {
    bool expected = true;
    if (token == m_lockerToken && m_exclusivelyLocked.compare_exchange_strong(expected, false)) {
        return true;
    }
    return false;
}

probelib::ReadResult ProbeLibHost::readMemory8(uint64_t address, size_t count) {
    if (auto checkResult = checkSession(); checkResult.isErr()) {
        return Err(checkResult.unwrapErr());
    }
    return m_probeSession->get()->readMemory8(address, count);
}

probelib::ReadResult ProbeLibHost::readMemory16(uint64_t address, size_t count) {
    if (auto checkResult = checkSession(); checkResult.isErr()) {
        return Err(checkResult.unwrapErr());
    }
    return m_probeSession->get()->readMemory16(address, count);
}

probelib::ReadResult ProbeLibHost::readMemory32(uint64_t address, size_t count) {
    if (auto checkResult = checkSession(); checkResult.isErr()) {
        return Err(checkResult.unwrapErr());
    }
    return m_probeSession->get()->readMemory32(address, count);
}

probelib::ReadResult ProbeLibHost::readMemory64(uint64_t address, size_t count) {
    if (auto checkResult = checkSession(); checkResult.isErr()) {
        return Err(checkResult.unwrapErr());
    }
    return m_probeSession->get()->readMemory64(address, count);
}

/***************************************** INTERNAL UTILS *****************************************/

Result<void, probelib::Error> ProbeLibHost::checkSession() {
    if (!m_probeSession.has_value() || !m_probeSession.value()) {
        return Err(probelib::Error{tr("Probe session not ready"), true, probelib::ErrorClass::SessionNotStarted});
    }
    return Ok();
}
