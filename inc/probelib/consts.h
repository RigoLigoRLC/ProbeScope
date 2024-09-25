
#pragma once

namespace probelib {
    enum class ErrorClass {
        UnspecifiedBackendError = 0,
        BeginConnectionFailure,
        CoreNotSelected,
        OperationUnaligned,
    };
}
