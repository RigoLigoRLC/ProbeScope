
#pragma once

#include "expressionevaluator/bytecode.h"
#include <QObject>
#include <condition_variable>
#include <mutex>
#include <thread>

class ProbeLibHost;

class AcquisitionHub : public QObject {
    Q_OBJECT
public:
    AcquisitionHub(ProbeLibHost *probeLibHost, QObject *parent = nullptr);
    ~AcquisitionHub();

private:
    //
    // Runtime Requests (when acquisition is running, the state cannot be abruptly changed, so requests from the main
    // thread will have to be queued and wait for the acquisition thread to take actions).
    //
    struct RequestAddEntry {
        size_t entryId;
        ExpressionEvaluator::Bytecode runtimeBytecode;
        int acquisitionFrequencyLimit;
    };
    struct RequestRemoveEntry {
        size_t entryId;
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
        RequestAddEntry,
        RequestRemoveEntry,
        RequestChangeEntryBytecode,
        RequestChangeEntryFrequencyLimit
    >; // clang-format on

private:
    static void acquisitionThread(AcquisitionHub *self);

private:
    ProbeLibHost *m_plh;

    // Acquisition thread, notification timer and synchronization primitives
    std::thread m_acquisitionThread;


    // All acquisition entries
    QMap<size_t, int> m_acquisitionEntries; // FIXME: acquisition entry!!!
};
