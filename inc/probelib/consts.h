
#pragma once

namespace probelib {
    enum class ErrorClass {
        UnspecifiedBackendError = 0,
        ScanProbeFailure,
        BeginConnectionFailure,
        CoreNotSelected,
        OperationUnaligned,
    };
}
