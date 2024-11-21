
#pragma once

#include "probelib/iprobelib.h"
#include <QObject>
#include <atomic>
#include <functional>


namespace probelib {
    class PSProbeAvailableProbe;

    class PSProbe : public QObject, public IProbeLib {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "cc.rigoligo.probescope.probelibs.PSProbe")
        Q_INTERFACES(probelib::IProbeLib)
    public:
        PSProbe();
        virtual QString name() const override { return tr("ProbeScope"); }
        virtual QString version() const override { return "1.0"; }
        virtual QString description() const override { return tr("ProbeScope Probe Library"); }

        // Initialize/terminate
        virtual bool initialize() override { return true; }
        virtual void terminate() override{};

        // List of available probes
        virtual QVector<IAvailableProbe::p> availableProbes() override;
        virtual Result<void, Error> selectProbe(IAvailableProbe::p probe) override;

        // List of supported devices
        virtual const QVector<DeviceCategory> supportedDevices() override;

        // Connection
        virtual Result<uint32_t, Error> setConnectionSpeed(uint32_t speed) override;
        virtual Result<uint32_t, Error> connectionSpeed() override;
        virtual Result<IProbeSession *, Error> connect(size_t deviceId) override;

    private:
        void unsetConnectionActiveFlag() { m_connectionActive = false; }

    private:
        QVector<QString> m_devices;
        QVector<DeviceCategory> m_deviceCategories;

        QVector<std::shared_ptr<PSProbeAvailableProbe>> m_probesFetched;
        void *m_currentProbe = nullptr; ///< Set when selectProbe() is called. This is ExtProbe on Rust side.

        bool m_connectionActive = false;
    };

    class PSProbeAvailableProbe : public IAvailableProbe {
    public:
        friend class PSProbe;
        PSProbeAvailableProbe(void *rustProbe);
        virtual ~PSProbeAvailableProbe();
        virtual QString name() override { return m_name; }
        virtual QString serialNumber() override { return m_serialNumber; }
        virtual QString description() override { return m_description; }

    private:
        void *m_rustProbe;
        QString m_name;
        QString m_serialNumber;
        QString m_description;
    };

    class PSProbeSession : public IProbeSession {
    public:
        PSProbeSession(void *session, std::function<void()> onDisconnect);
        virtual ~PSProbeSession();
        virtual Result<QVector<CoreDescriptor>, Error> listCores() override;
        virtual Result<void, Error> selectCore(size_t core) override;
        virtual ReadResult readMemory8(size_t address, size_t count) override;
        virtual ReadResult readMemory16(size_t address, size_t count) override;
        virtual ReadResult readMemory32(size_t address, size_t count) override;
        virtual ReadResult readMemory64(size_t address, size_t count) override;
        virtual Result<void, Error> writeMemory8(size_t address, const QByteArray &data) override;
        virtual Result<void, Error> writeMemory16(size_t address, const QByteArray &data) override;
        virtual Result<void, Error> writeMemory32(size_t address, const QByteArray &data) override;
        virtual Result<void, Error> writeMemory64(size_t address, const QByteArray &data) override;
        virtual Result<void, Error> setReadScatterGatherList(const QVector<ScatterGatherEntry> &list) override;
        virtual ReadResult readScatterGather() override;

    private:
        void *m_session;
        std::atomic_size_t m_coreSelected;

        std::function<void()> m_disconnectCallback;
    };
}
