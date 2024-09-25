
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

        // Connect to a probe
        virtual Result<IProbeSession, Error> connectToProbe(IAvailableProbe::p probe) = 0;
    };

    /**
     * @brief An unique pointer to IProbeSession is returned when trying to connect to a probe.
     * As a rule, only one IProbeSession can exist at a time, for each ProbeLib. Your ProbeLib implementation should be
     * designed in a way so that when an connection is active, no more connections can be made until the current
     * connection is terminated. ProbeScope will drop the IProbeSession unique pointer when the connection should be
     * terminated, and connection cleanup should be done in the destructor of your IProbeSession implementation.
     *
     */
    class IProbeSession {
    public:
        virtual ~IProbeSession() = default;
        using p = std::unique_ptr<IProbeSession>;

        // Core selection API
        virtual Result<QVector<size_t>, Error> listCores() = 0;
        virtual Result<void, Error> selectCore(size_t core) = 0;

        // Simple Burst Read/Write
        virtual ReadResult readMemory8(size_t address, size_t count) = 0;
        virtual ReadResult readMemory16(size_t address, size_t count) = 0;
        virtual ReadResult readMemory32(size_t address, size_t count) = 0;

        virtual Result<void, Error> writeMemory8(size_t address, const QByteArray &data) = 0;
        virtual Result<void, Error> writeMemory16(size_t address, const QByteArray &data) = 0;
        virtual Result<void, Error> writeMemory32(size_t address, const QByteArray &data) = 0;

        // Scatter-Gather Read
        virtual Result<void, Error> setReadScatterGatherList(const QVector<ScatterGatherEntry> &list) = 0;
        virtual ReadResult readScatterGather() = 0;
    };
}

Q_DECLARE_INTERFACE(probelib::IProbeLib, "cc.rigoligo.probescope.IProbeLib")
