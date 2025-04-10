
#pragma once

namespace probelib {

enum class ErrorClass {
    UnspecifiedBackendError = 0,
    SessionNotStarted,
    ScanProbeFailure,
    PreConfigurationFailure,
    BeginConnectionFailure,
};

enum class WireProtocol {
    Unspecified = 0,
    Swd,
    Jtag,
};

} // namespace probelib
