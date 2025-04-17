
#include "acquisitionbuffer.h"
#include <QDebug>

AcquisitionBuffer::AcquisitionBuffer() : IAcquisitionBufferChannel(), QObject(nullptr) {
    //
}

AcquisitionBuffer::~AcquisitionBuffer() {
    //
}

void AcquisitionBuffer::addDataPoint(size_t entryId, std::chrono::steady_clock::time_point timestamp, Value value) {
    if (auto channel = m_channels.find(entryId); channel == m_channels.end()) {
        qCritical() << "AcquisitionBuffer: Does not have channel for entry" << entryId;
        return;
    } else if (auto result = channel->second.queue.try_push(std::move(std::pair{timestamp, value})); !result) {
        qWarning() << "AcquisitionBuffer: Queue for channel" << entryId << "overflowed!";
    }
}

void AcquisitionBuffer::acquisitionFrequencyFeedback(size_t entryId, double frequency) {
    if (auto channel = m_channels.find(entryId); channel == m_channels.end()) {
        qCritical() << "AcquisitionBuffer: Does not have channel for entry" << entryId;
        return;
    } else {
        channel->second.frequencyFeedback = frequency;
        emit frequencyFeedbackArrived(entryId);
    }
}

void AcquisitionBuffer::addChannel(size_t entryId) {
    if (m_channels.contains(entryId)) {
        qCritical() << "AcquisitionBuffer: Already have channel for entry" << entryId;
        return;
    }

    m_channels.try_emplace(entryId);
}

void AcquisitionBuffer::removeChannel(size_t entryId) {
    if (!m_channels.contains(entryId)) {
        qCritical() << "AcquisitionBuffer: Does not have channel for entry" << entryId;
        return;
    }

    m_channels.erase(entryId);
}

void AcquisitionBuffer::drainChannel(size_t entryId, std::function<void(Timepoint, Value)> processor) {
    if (!m_channels.contains(entryId)) {
        qCritical() << "Does not have channel for entry" << entryId;
        return;
    }

    std::tuple<Timepoint, Value> dataPoint;
    while (m_channels.at(entryId).queue.try_pop(dataPoint)) {
        auto [timepoint, value] = dataPoint;
        processor(timepoint, value);
    }
}

double AcquisitionBuffer::getChannelFrequencyFeedback(size_t entryId) {
    if (!m_channels.contains(entryId)) {
        qCritical() << "Does not have channel for entry" << entryId;
        return NAN;
    }

    return m_channels.at(entryId).frequencyFeedback;
}

double AcquisitionBuffer::valueToDouble(Value value) {
    return std::visit(
        [&](auto &&arg) -> double {
            using T = std::decay_t<decltype(arg)>;
#define MATCH(X) constexpr(std::is_same_v<T, X>)
            // This is so fucking tedious you just return return return return return return return return
            /*  */ if MATCH (uint8_t) {
                return arg;
            } else if MATCH (uint16_t) {
                return arg;
            } else if MATCH (uint32_t) {
                return arg;
            } else if MATCH (uint64_t) {
                return arg;
            } else if MATCH (int8_t) {
                return arg;
            } else if MATCH (int16_t) {
                return arg;
            } else if MATCH (int32_t) {
                return arg;
            } else if MATCH (int64_t) {
                return arg;
            } else if MATCH (float) {
                return arg;
            } else if MATCH (double) {
                return arg;
            } else {
                qCritical() << "What the fuck type did I encounter";
                return NAN;
            }
#undef MATCH
        },
        value);
}

double AcquisitionBuffer::timepointToMillisecond(Timepoint reference, Timepoint timepoint) {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(timepoint - reference).count() / 1000.0;
}
