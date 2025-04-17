
#pragma once

#include "acquisitionbufferchannel.h"
#include <QObject>
#include <atomic_queue/atomic_queue.h>
#include <functional>
#include <map>

class AcquisitionBuffer : public QObject, public IAcquisitionBufferChannel {
    Q_OBJECT
public:
    AcquisitionBuffer();
    virtual ~AcquisitionBuffer() override;
    using Clock = std::chrono::steady_clock;
    using Timepoint = Clock::time_point;

    virtual void addDataPoint(size_t entryId, Timepoint timestamp, Value value) override;
    virtual void acquisitionFrequencyFeedback(size_t entryId, double frequency) override;

    void addChannel(size_t entryId);
    void removeChannel(size_t entryId);
    void drainChannel(size_t entryId, std::function<void(Timepoint, Value)> processor);
    double getChannelFrequencyFeedback(size_t entryId);

    static double valueToDouble(Value value);
    static double timepointToMillisecond(Timepoint reference, Timepoint timepoint);

private:
    static constexpr size_t Size = 8192;
    using ChannelQueue = atomic_queue::AtomicQueueB2<std::tuple<Timepoint, Value>>;
    struct Channel {
        Channel() : queue(Size) {} // FIXME: Make queue size editable in settings
        ChannelQueue queue;
        double frequencyFeedback;
        bool overflowFlag;
    };

    std::map<size_t, Channel> m_channels;

signals:
    /// @brief Emitted when frequency feedback was set for a channel. PLEASE Connect with QueuedConnection!
    void frequencyFeedbackArrived(size_t entryId);
};
