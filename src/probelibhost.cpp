
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


Result<void, QString> ProbeLibHost::connect(IProbeLib *probeLib, IAvailableProbe::p probe) {
    if (!probeLib) {
        return Err(tr("Invalid probe library"));
    }

    if (!probe) {
        return Err(tr("Invalid probe"));
    }

    if (auto connectResult = probeLib->connectToProbe(probe); connectResult.isErr()) {
        return Err(tr("Failed to connect to probe: %1").arg(connectResult.unwrapErr().message));
    } else {
        // A new connection must be established when previous one is disconnected
        Q_ASSERT(!m_probeSession.has_value());
        m_probeSession = std::unique_ptr<IProbeSession>(connectResult.unwrap());

        // Save probe info
        m_currentProbeLib = probeLib;
        m_currentProbe = probe;
    }

    return Ok();
}

void ProbeLibHost::disconnect() {
    m_probeSession.reset();
}
