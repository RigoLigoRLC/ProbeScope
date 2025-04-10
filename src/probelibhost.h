
#pragma once

#include "probelib/iprobelib.h"
#include "result.h"
#include <QPluginLoader>
#include <atomic>
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
    probelib::WireProtocol wireProtocol() const { return m_wireProtocol; }
    bool sessionActive() const { return m_probeSession.has_value(); }
    Result<void, QString> connect();
    Q_SLOT void disconnect();

    // Exclusive usage
    /// @brief Marks the ProbeLibHost in an "exclusively used by someone" state. For example, when acquisition has
    /// started, you cannot start a probe speed benchmark test. The token passed in is only used to release the lock,
    /// and is not checked for target operations (introduces extra overhead)
    /// @param token Locker token. Required when attempting unlock.
    /// @return true when lock is acquired, false when lock is already held.
    bool getExclusiveLock(quintptr token);
    /// @brief Release the exclusive lock set by getExclusiveLock.
    /// @param token The same locker token passed into getExclusiveLock when acquiring the lock.
    /// @return true when the lock is released, false when the lock is not held by anyone or the token is incorrect.
    bool releaseExclusiveLock(quintptr token);

    // Operations on target
    probelib::ReadResult readMemory8(uint64_t address, size_t count);
    probelib::ReadResult readMemory16(uint64_t address, size_t count);
    probelib::ReadResult readMemory32(uint64_t address, size_t count);
    probelib::ReadResult readMemory64(uint64_t address, size_t count);
    // TODO: Write APIs

private:
    Result<void, probelib::Error> checkSession();

private:
    QVector<probelib::IProbeLib *> m_probeLibs;
    std::optional<std::unique_ptr<probelib::IProbeSession>> m_probeSession;

    int m_connectionSpeed;
    probelib::WireProtocol m_wireProtocol;

    // Exclusive lock
    std::atomic_bool m_exclusivelyLocked;
    quintptr m_lockerToken;

    // Only kept for use in SelectProbeDialog
    probelib::IProbeLib *m_currentProbeLib = nullptr;
    probelib::IAvailableProbe::p m_currentProbe;

    // Only kept for use in SelectDeviceDialog
    QVector<probelib::DeviceCategory> m_deviceCategories;
    size_t m_currentDevice = 0;
};
