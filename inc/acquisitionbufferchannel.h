
#pragma once

#include <chrono>
#include <cstdint>
#include <memory>
#include <variant>

/**
 * @brief This pure virtual class defines an interface between acquisition module and storage module. Because
 * acquisition runs on one separate thread and UI thread doesn't have to update the UI so frequently, the acquired data
 * should be buffered and only refreshed to the screen when needed. To reduce APIs exposed to acquisition module, an
 * object implementing this interface is passed and only specifically represents the acquisition buffer channel.
 */
class IAcquisitionBufferChannel {
public:
    using Value = std::variant<uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t, float, double>;
    using p = std::shared_ptr<IAcquisitionBufferChannel>;

    virtual ~IAcquisitionBufferChannel(){};
    virtual void addDataPoint(size_t entryId, std::chrono::steady_clock::time_point timestamp, Value value) = 0;

    /**
     * @brief Call this function to provide feedback on actual acquisition frequency. This function should be called
     * relatively infrequent because each call will cause the UI to refresh.
     * @param entryId Watch entry ID.
     * @param frequency frequency feedback value.
     */
    virtual void acquisitionFrequencyFeedback(size_t entryId, double frequency) = 0;
};
