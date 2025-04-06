
#pragma once

#include <chrono>
#include <cstdint>
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
    virtual void addDataPoint(size_t entryId, std::chrono::steady_clock::time_point timestamp, Value value);
};
