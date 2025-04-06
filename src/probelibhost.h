
#pragma once

#include "probelib/iprobelib.h"
#include "result.h"
#include <QPluginLoader>
#include <optional>
#include <tuple>

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
    Result<void, QString> selectProbe(probelib::IProbeLib *probeLib, probelib::IAvailableProbe::p probe);

    // Only SelectDeviceDialog should use these
    QVector<probelib::DeviceCategory> availableDevices() const;
    size_t currentDevice() const { return m_currentDevice; }
    Result<void, QString> selectDevice(size_t deviceID);

    // Connection
    /// @brief Set Connection speed in kHz. Will only take effect if set before connect(). Default is 4MHz.
    void setConnectionSpeed(int khz) { m_connectionSpeed = khz; };
    /// @brief Get the connection speed in kHz. If no probe is selected, returns -1. Undefined if never set.
    int connectionSpeed() const { return m_currentProbeLib ? m_currentProbeLib->connectionSpeed().unwrapOr(-1) : -1; }
    bool sessionActive() const { return m_probeSession.has_value(); }
    Result<void, QString> connect();
    Q_SLOT void disconnect();

    // Operations on target

private:
    QVector<probelib::IProbeLib *> m_probeLibs;
    std::optional<std::unique_ptr<probelib::IProbeSession>> m_probeSession;

    int m_connectionSpeed;

    // Only kept for use in SelectProbeDialog
    probelib::IProbeLib *m_currentProbeLib = nullptr;
    probelib::IAvailableProbe::p m_currentProbe;

    // Only kept for use in SelectDeviceDialog
    QVector<probelib::DeviceCategory> m_deviceCategories;
    size_t m_currentDevice = 0;
};
