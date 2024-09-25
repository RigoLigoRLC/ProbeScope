
#pragma once

#include "consts.h"
#include "result_hack.h"
#include <QObject>
#include <memory>


namespace probelib {
    /**
     * @brief Interface for ProbeScope to list and select available probes.
     * ProbeLib should make its own representation of available probes that implements this interface, and return shared
     * pointers to their instances back to ProbeScope. ProbeScope will select a probe and start a connection with these
     * shared pointers.
     */
    class IAvailableProbe {
    public:
        using p = std::shared_ptr<IAvailableProbe>;

        virtual ~IAvailableProbe() = default;
        virtual QString name() = 0;
        virtual QString serialNumber() = 0;
        virtual QString description() = 0;
    };

    /**
     * @brief Error reporting structure from ProbeLib to ProbeScope.
     * ProbeScope will terminate the connection if a fatal error is reported.
     * The error class member is mainly used to distinguish errors that ProbeScope can handle differently when
     * configured.
     */
    struct Error {
        QString message;
        bool fatal;
        ErrorClass errorClass;
    };

    /**
     * @brief Return type of memory read access functions.
     */
    typedef Result<QByteArray, Error> ReadResult;

    /**
     * @brief ProbeScope variable acquisition requires scatter-gather read access to memory. This structure defines the
     * scatter gather list entry.
     */
    struct ScatterGatherEntry {
        size_t address;
        size_t count;
    };
}
