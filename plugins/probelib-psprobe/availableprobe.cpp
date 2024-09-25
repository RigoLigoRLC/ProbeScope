
#include "psprobe.h"
#include "psprobe/probe.h"

namespace probelib {
    PSProbeAvailableProbe::PSProbeAvailableProbe(void *rustProbe) : m_rustProbe(rustProbe) {
        char *str;
        size_t len;

        psprobe_probe_get_name(m_rustProbe, &str, &len);
        m_name = QString::fromUtf8(str, len);

        psprobe_probe_get_serial_number(m_rustProbe, &str, &len);
        m_serialNumber = QString::fromUtf8(str, len);

        uint16_t vid, pid;
        psprobe_probe_get_vid_pid(m_rustProbe, &vid, &pid);
        m_description = QString("VID: %1, PID: %2").arg(vid, 4, 16, QChar('0')).arg(pid, 4, 16, QChar('0'));
    }

    PSProbeAvailableProbe::~PSProbeAvailableProbe() {
        psprobe_probe_destroy(m_rustProbe);
    }
}
