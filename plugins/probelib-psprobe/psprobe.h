
#pragma once

#include "probelib/iprobelib.h"
#include <QObject>

namespace probelib {
    class PSProbe : public IProbeLib {
        Q_OBJECT
        Q_PLUGIN_METADATA(IID "cc.rigoligo.probescope.probelibs.PSProbe")
        Q_INTERFACES(probelib::IProbeLib)
    public:
    };
}
