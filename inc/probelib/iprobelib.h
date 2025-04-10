
/**
 * @brief probelib::IProbeLib Interface Definition
 * This is interface version 1 (under development).
 */

#pragma once

#include "misc.h"
#include "result_hack.h"
#include <QObject>
#include <QVector>


namespace probelib {
class IProbeSession;

class IProbeLib {
public:
    virtual ~IProbeLib() = default;

    // Probe Library metadata
    virtual QString name() const = 0;
    virtual QString version() const = 0;
    virtual QString description() const = 0;

    // Initialize/terminate
    virtual bool initialize() = 0;
    virtual void terminate() = 0;

    // List of available probes
    virtual QVector<IAvailableProbe::p> availableProbes() = 0;
    /// @brief Select a probe from the list of available probes.
    virtual Result<void, Error> selectProbe(IAvailableProbe::p probe) = 0;

    // List of supported devices
    virtual const QVector<DeviceCategory> supportedDevices() = 0;

    // Connection
    /// @brief Sets the connection speed for the probe. ProbeScope will call it only once before calling connect().
    /// @return Should return actual connection speed that the probe decided it will use.
    virtual Result<uint32_t, Error> setConnectionSpeed(uint32_t speed) = 0;
    /// @brief Should return the connection speed that the probe decided it will use.
    virtual Result<uint32_t, Error> connectionSpeed() = 0;
    /// @brief Sets the protocol for the probe. ProbeScope will call it only once before calling connect().
    /// @return Should return nothing when successfully set. If not, an error string should be returned.
    virtual Result<void, Error> setProtocol(WireProtocol protocol) = 0;
    /// @brief should return the protocol that the probe currently uses.
    virtual Result<WireProtocol, Error> protocol() = 0;
    /// @brief Connect to a device with the previously selected probe.
    virtual Result<IProbeSession *, Error> connect(size_t deviceId) = 0;
};

/**
 * @brief An pointer to IProbeSession is returned when trying to connect to a probe.
 * As a rule, only one IProbeSession can exist at a time, for each ProbeLib. Your ProbeLib implementation should be
 * designed in a way so that when an connection is active, no more connections can be made until the current
 * connection is terminated. ProbeScope will drop the IProbeSession pointer when the connection should be
 * terminated, and connection cleanup should be done in the destructor of your IProbeSession implementation.
 *
 */
class IProbeSession {
public:
    virtual ~IProbeSession() = default;

    // Core selection API
    virtual Result<QVector<CoreDescriptor>, Error> listCores() = 0;
    virtual Result<void, Error> selectCore(size_t core) = 0;

    // Simple Burst Read/Write
    virtual ReadResult readMemory8(uint64_t address, size_t count) = 0;
    virtual ReadResult readMemory16(uint64_t address, size_t count) = 0;
    virtual ReadResult readMemory32(uint64_t address, size_t count) = 0;
    virtual ReadResult readMemory64(uint64_t address, size_t count) = 0;

    virtual Result<void, Error> writeMemory8(uint64_t address, const QByteArray &data) = 0;
    virtual Result<void, Error> writeMemory16(uint64_t address, const QByteArray &data) = 0;
    virtual Result<void, Error> writeMemory32(uint64_t address, const QByteArray &data) = 0;
    virtual Result<void, Error> writeMemory64(uint64_t address, const QByteArray &data) = 0;

    // Scatter-Gather Read
    virtual Result<void, Error> setReadScatterGatherList(const QVector<ScatterGatherEntry> &list) = 0;
    virtual ReadResult readScatterGather() = 0;
};
} // namespace probelib

Q_DECLARE_INTERFACE(probelib::IProbeLib, "cc.rigoligo.probescope.IProbeLib")
