
#include "psprobe.h"
#include "psprobe/probe.h"

namespace probelib {
    QVector<IAvailableProbe::p> PSProbe::availableProbes() {
        // Clear old probes
        m_probesFetched.clear();

        // Fetch new probes
        size_t count;
        void *probes;
        psprobe_probe_list_get(&probes, &count);
        for (size_t i = 0; i < count; i++) {
            void *probe;
            psprobe_probe_list_get_probe(probes, &i, &probe);
            m_probesFetched.append(std::make_shared<PSProbeAvailableProbe>(probe));
        }
        psprobe_probe_list_destroy(probes);

        // Convert to base interface
        QVector<IAvailableProbe::p> ret;
        foreach (auto probe, m_probesFetched) {
            ret.append(probe);
        }
        return ret;
    }

    Result<IProbeSession::p, Error> PSProbe::connectToProbe(IAvailableProbe::p probe) {
        return Err(Error{"Not Implemented", true, ErrorClass::BeginConnectionFailure});
    }
}