
#pragma once

#include "probelib/iprobelib.h"
#include <QObject>

namespace probelib {
    class PSProbeAvailableProbe;

    class PSProbe : public QObject, public IProbeLib {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "cc.rigoligo.probescope.probelibs.PSProbe")
        Q_INTERFACES(probelib::IProbeLib)
    public:
        virtual QString name() const override { return tr("ProbeScope"); }
        virtual QString version() const override { return "1.0"; }
        virtual QString description() const override { return tr("ProbeScope Probe Library"); }

        // Initialize/terminate
        virtual bool initialize() override { return true; }
        virtual void terminate() override{};

        // List of available probes
        virtual QVector<IAvailableProbe::p> availableProbes() override;

        // Connect to a probe
        virtual Result<IProbeSession::p, Error> connectToProbe(IAvailableProbe::p probe) override;

    private:
        QVector<std::shared_ptr<PSProbeAvailableProbe>> m_probesFetched;
        void *m_currentProbe = nullptr;
    };

    class PSProbeAvailableProbe : public IAvailableProbe {
    public:
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
}
