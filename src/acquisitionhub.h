
#pragma once

#include "acquisitionbufferchannel.h"
#include "atomic_queue/atomic_queue.h"
#include "expressionevaluator/bytecode.h"
#include <QObject>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

class ProbeLibHost;

class AcquisitionHub : public QObject {
    Q_OBJECT
public:
    AcquisitionHub(ProbeLibHost *probeLibHost, QObject *parent = nullptr);
    ~AcquisitionHub();

    void setAcquisitionBufferChannel(IAcquisitionBufferChannel::p channel);

    void startAcquisition();
    void stopAcquisition();
    bool isAcquisitionActive() const { return m_acquisitionRunning; }

    void setFrequencyFeedbackReportInterval();

    void addWatchEntry(size_t entryId, bool enabled, ExpressionEvaluator::Bytecode runtimeBytecode, int freqLimit);
    void removeWatchEntry(size_t entryId);
    void setEntryEnabled(size_t entryId, bool enable);
    void changeWatchEntryBytecode(size_t entryId, ExpressionEvaluator::Bytecode runtimeBytecode);
    void changeWatchEntryFrequencyLimit(size_t entryId, int freqLimit);

private:
    //
    // Runtime Requests (when acquisition is running, the state cannot be abruptly changed, so requests from the
    // main thread will have to be queued and wait for the acquisition thread to take actions).
    //
    struct RequestStartAcquisition {};
    struct RequestStopAcquisition {};
    struct RequestExit {};
    struct RequestAddEntry {
        size_t entryId;
        bool enabled;
        ExpressionEvaluator::Bytecode runtimeBytecode;
        int acquisitionFrequencyLimit;
    };
    struct RequestRemoveEntry {
        size_t entryId;
    };
    struct RequestSetEntryEnabled {
        size_t entryId;
        bool enable;
    };
    struct RequestChangeEntryBytecode {
        size_t entryId;
        ExpressionEvaluator::Bytecode runtimeBytecode;
    };
    struct RequestChangeEntryFrequencyLimit {
        size_t entryId;
        int acquisitionFrequencyLimit;
    };
    using RuntimeRequest = std::variant< // clang-format off
        std::monostate,
        RequestStartAcquisition,
        RequestStopAcquisition,
        RequestExit,
        RequestAddEntry,
        RequestRemoveEntry,
        RequestSetEntryEnabled,
        RequestChangeEntryBytecode,
        RequestChangeEntryFrequencyLimit
    >; // clang-format on

    //
    // Acquisition entry. This data structure keeps the acquisition bytecode, the current execution status of a watch
    // entry.
    //
    struct AcquisitionEntry {
        ExpressionEvaluator::Bytecode bytecode;
        ExpressionEvaluator::ExecutionState es;
        int frequencyLimit;
        bool enabled;
        std::chrono::steady_clock::time_point lastAcquisitionTime;
        std::chrono::steady_clock::duration minimumWaitDuration;

        // Feedback context
        std::chrono::steady_clock::time_point lastFeedbackTime;
        size_t acquisitionCounter;
    };

    //
    // Platform-dependent precision timer context
    //
    struct PlatformTimer {
#ifdef Q_OS_WIN

#endif
    };

private:
    void sendRequest(RuntimeRequest &&request);

    static void acquisitionThread(AcquisitionHub *self);
    void startAcquisitionTimer();
    void stopAcquisitionTimer();

    /// @brief This is read each time the acquisition starts
    void readFrequencyFeedbackReportIntervalFromQSettings();

private:
    ProbeLibHost *m_plh;

    bool m_acquisitionRunning;

    // Acquisition thread, notification timer and synchronization primitives
    std::thread m_acquisitionThread;
    std::mutex m_mutex;
    std::condition_variable m_cond;
    atomic_queue::AtomicQueue2<RuntimeRequest, 128> m_requestQueue;
    IAcquisitionBufferChannel::p m_bufferChannel;

    // All acquisition entries
    QMap<size_t, AcquisitionEntry> m_acquisitionEntries;

    std::chrono::milliseconds m_frequencyFeedbackReportInterval;

signals:
    // Signals from acquisition thread. PLEASE CONNECT WITH Qt::QueuedConnection!
    void acquisitionStopped();
};
