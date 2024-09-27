
#pragma once

#include "probelib/iprobelib.h"
#include "result.h"
#include <QPluginLoader>
#include <optional>

class ProbeLibHost : public QObject {
    Q_OBJECT
public:
    ProbeLibHost(QObject *parent = nullptr);
    ~ProbeLibHost();

    void scanProbeLibs();
    QVector<probelib::IProbeLib *> probeLibs() const { return m_probeLibs; }

    // Only SelectProbeDialog should use these
    probelib::IProbeLib *currentProbeLib() const { return m_currentProbeLib; }
    probelib::IAvailableProbe::p currentProbe() const { return m_currentProbe; }

    bool sessionActive() const { return m_probeSession.has_value(); }
    Result<void, QString> connect(probelib::IProbeLib *probeLib, probelib::IAvailableProbe::p probe);

public slots:
    void disconnect();

private:
    QVector<probelib::IProbeLib *> m_probeLibs;
    std::optional<std::unique_ptr<probelib::IProbeSession>> m_probeSession;

    // Only kept for use in SelectProbeDialog
    probelib::IProbeLib *m_currentProbeLib = nullptr;
    probelib::IAvailableProbe::p m_currentProbe;
};
