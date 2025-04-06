
#pragma once

#include "consts.h"
#include "result_hack.h"
#include <QObject>
#include <memory>
#include <tuple>


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

    /**
     * @brief This is the interface used to report what kind of devices (MCU SKUs, for example) to ProbeScope.
     * A category will be displayed as a tree node and devices belonging to it will be children.
     * Each device will have an ID that ProbeScope will use to select the device.
     */
    struct DeviceCategory {
        QString name;
        QVector<std::tuple<size_t, QString>> devices; ///< ID, Device name
    };

    /**
     * @brief This is the structure to return when ProbeScope asks ProbeLib for a list of available probes.
     * coreId is the key to index into ProbeLib's internal list of cores; and coreDescription is the human-readable
     * description of the core, and it can be core "name" (like "Master Core"), microarchitecture (like "Cortex-M4"),
     * or anything human readable to distinguish between cores.
     * @warning Do not assume coreId is contiguous or 0-based. It can be any arbitrary number.
     */
    struct CoreDescriptor {
        size_t coreId;
        QString coreDescription;
    };
}
