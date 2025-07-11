
#include "acquisitionhub.h"
#include "probelibhost.h"
#include <QSettings>

AcquisitionHub::AcquisitionHub(ProbeLibHost *probeLibHost, QObject *parent)
    : m_plh(probeLibHost), QObject(parent), m_acquisitionThread(acquisitionThread, this), m_acquisitionRunning(false) {
    //
}

AcquisitionHub::~AcquisitionHub() {
    sendRequest(RequestExit{});
    m_acquisitionThread.join();
}

void AcquisitionHub::setAcquisitionBufferChannel(IAcquisitionBufferChannel::p channel) {
    m_bufferChannel = channel;
}

void AcquisitionHub::startAcquisition() {
    sendRequest(RequestStartAcquisition{});
}

void AcquisitionHub::stopAcquisition() {
    sendRequest(RequestStopAcquisition{});
}

void AcquisitionHub::setFrequencyFeedbackReportInterval() {}

void AcquisitionHub::addWatchEntry(size_t entryId, bool enabled, ExpressionEvaluator::Bytecode runtimeBytecode,
                                   int freqLimit) {
    sendRequest(RequestAddEntry{entryId, enabled, runtimeBytecode, freqLimit});
}

void AcquisitionHub::removeWatchEntry(size_t entryId) {
    sendRequest(RequestRemoveEntry{entryId});
}

void AcquisitionHub::setEntryEnabled(size_t entryId, bool enable) {
    sendRequest(RequestSetEntryEnabled{entryId, enable});
}

void AcquisitionHub::changeWatchEntryBytecode(size_t entryId, ExpressionEvaluator::Bytecode runtimeBytecode) {
    sendRequest(RequestChangeEntryBytecode{entryId, runtimeBytecode});
}

void AcquisitionHub::changeWatchEntryFrequencyLimit(size_t entryId, int freqLimit) {
    sendRequest(RequestChangeEntryFrequencyLimit{entryId, freqLimit});
}

/***************************************** INTERNAL UTILS *****************************************/

void AcquisitionHub::sendRequest(AcquisitionHub::RuntimeRequest &&request) {
    // FIXME: what if the queue is really full?
    while (!m_requestQueue.try_push(request))
        ;
    m_cond.notify_all();
}

void AcquisitionHub::acquisitionThread(AcquisitionHub *self) {
    //
    auto &running = self->m_acquisitionRunning;
    bool runLoop = true;
    std::unique_lock<std::mutex> lock(self->m_mutex);
    while (runLoop) {
        // Run acquisition for each entry
        auto now = std::chrono::steady_clock::now();
        if (running) {
            for (auto it = self->m_acquisitionEntries.begin(); it != self->m_acquisitionEntries.end(); ++it) {
                // Check if an entry is enabled
                if (!it->enabled) {
                    continue;
                }

                // Check if deadline reached, if not, don't work on this acquisition just for now
                // FIXME: use a timer to wake this thread up.
                if (now - it->lastAcquisitionTime < it->minimumWaitDuration) {
                    continue;
                }

                // Update last acquisition time (only for whose PC=0)
                if (it->es.PC == 0) {
                    it->lastAcquisitionTime = std::chrono::steady_clock::now();
                }

                // Run bytecode
                // For each acquisition entry, each time we visit it we only give it one chance to access device memory
                // so we don't waste too much time on a single expression
                bool memAccessed = false;
                using namespace ExpressionEvaluator;
                auto execResult = it->bytecode.execute(
                    it->es, [&](ExecutionState &es, Opcode op, Bytecode::ImmType imm) -> Bytecode::ExecutionResult {
                        // Try generic executor
                        if (Bytecode::genericComputationExecutor(es, op, imm) == Bytecode::Continue) {
                            return Bytecode::Continue;
                        }

                        auto processRead = [&](probelib::ReadResult result) {
                            if (result.isErr()) {
                                qWarning() << "Read memory 8" << Qt::hex << es.stack.last() << "failed"
                                           << result.unwrapErr().message;
                                es.resetAll();
                                return Bytecode::ErrorBreak;
                            } else {
                                es.stack.last() = 0;
                                memcpy(&es.stack.last(), result.unwrap().data(), result.unwrap().size());
                                return Bytecode::Continue;
                            }
                        };
                        auto processReturn = [&]<typename T>(uint64_t word, T dummy) {
                            T t;
                            memcpy(&t, &word, sizeof(T));
                            if (self->m_bufferChannel) {
                                self->m_bufferChannel->addDataPoint(it.key(), now, t);
                            }

                            // Each time we return a value, we check if we need to report frequency feedback
                            if (auto interval = now - it->lastFeedbackTime;
                                interval >= self->m_frequencyFeedbackReportInterval) {
                                auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(interval).count();
                                self->m_bufferChannel->acquisitionFrequencyFeedback(it.key(), it->acquisitionCounter *
                                                                                                  1000.0 / ms);
                                it->acquisitionCounter = 0;
                                it->lastFeedbackTime = now;
                            }

                            // After checking feedback, we increment the acquisition counter
                            ++it->acquisitionCounter;
                        };
                        // If generic executor can't handle it, we're facing some complex instructions
                        switch (op) {
                            // Memory accesses
                            case Deref8: return processRead(self->m_plh->readMemory8(es.stack.last(), 1));
                            case Deref16: return processRead(self->m_plh->readMemory16(es.stack.last(), 1));
                            case Deref32: return processRead(self->m_plh->readMemory32(es.stack.last(), 1));
                            case Deref64: return processRead(self->m_plh->readMemory64(es.stack.last(), 1));
                            case ReturnU8: processReturn(es.stack.last(), uint8_t(0)); break;
                            case ReturnU16: processReturn(es.stack.last(), uint16_t(0)); break;
                            case ReturnU32: processReturn(es.stack.last(), uint32_t(0)); break;
                            case ReturnU64: processReturn(es.stack.last(), uint64_t(0)); break;
                            case ReturnI8: processReturn(es.stack.last(), int8_t(0)); break;
                            case ReturnI16: processReturn(es.stack.last(), int16_t(0)); break;
                            case ReturnI32: processReturn(es.stack.last(), int32_t(0)); break;
                            case ReturnI64: processReturn(es.stack.last(), int64_t(0)); break;
                            case ReturnF32: processReturn(es.stack.last(), float(0)); break;
                            case ReturnF64: processReturn(es.stack.last(), double(0)); break;
                            default: Q_UNREACHABLE(); return Bytecode::ErrorBreak;
                        }
                        return Bytecode::Continue;
                    });

                if (execResult >= Bytecode::BeginErrors) {
                    qCritical() << "Severe execution error occured, acquisition stopping";
                    self->stopAcquisition();
                }
            }
        }

        // If acquisition is not running, wait for events. Otherwise we loop directly.
        if (!running) {
            self->m_cond.wait(lock);
        }

        // Check for runtime requests. If there isn't any, do acquisition.
        RuntimeRequest req;
        while (self->m_requestQueue.try_pop(req)) {
            std::visit(
                [&](auto &&arg) {
                    using T = std::decay_t<decltype(arg)>;
#define MATCH(X) constexpr(std::is_same_v<T, X>)
                    /*  */ if MATCH (RequestStartAcquisition) {
                        running = true;
                        self->readFrequencyFeedbackReportIntervalFromQSettings();
                    } else if MATCH (RequestStopAcquisition) {
                        running = false;
                        emit self->acquisitionStopped();
                    } else if MATCH (RequestExit) {
                        runLoop = false;
                    } else if MATCH (RequestAddEntry) {
                        if (self->m_acquisitionEntries.contains(arg.entryId)) {
                            qCritical() << "AcquisitionHub already has entry" << arg.entryId;
                            return;
                        }
                        self->m_acquisitionEntries[arg.entryId] = {.bytecode = arg.runtimeBytecode,
                                                                   .es = {},
                                                                   .frequencyLimit = arg.acquisitionFrequencyLimit,
                                                                   .enabled = arg.enabled,
                                                                   .acquisitionCounter = 0};
                    } else if MATCH (RequestRemoveEntry) {
                        if (!self->m_acquisitionEntries.contains(arg.entryId)) {
                            qCritical() << "AcquisitionHub does not have entry" << arg.entryId;
                            return;
                        }
                        self->m_acquisitionEntries.remove(arg.entryId);
                    } else if MATCH (RequestSetEntryEnabled) {
                        if (!self->m_acquisitionEntries.contains(arg.entryId)) {
                            qCritical() << "AcquisitionHub does not have entry" << arg.entryId;
                            return;
                        }
                        if (!arg.enable && running) {
                            // Insert QNaN here to break the graph line. This is a documented valid usage of QCustomPlot
                            self->m_bufferChannel->addDataPoint(arg.entryId, now, qQNaN());
                            self->m_acquisitionEntries[arg.entryId].es.resetAll();
                        }
                        self->m_acquisitionEntries[arg.entryId].enabled = arg.enable;
                    } else if MATCH (RequestChangeEntryBytecode) {
                        if (!self->m_acquisitionEntries.contains(arg.entryId)) {
                            qCritical() << "AcquisitionHub does not have entry" << arg.entryId;
                            return;
                        }
                        self->m_acquisitionEntries[arg.entryId].bytecode = arg.runtimeBytecode;
                    } else if MATCH (RequestChangeEntryFrequencyLimit) {
                        if (!self->m_acquisitionEntries.contains(arg.entryId)) {
                            qCritical() << "AcquisitionHub does not have entry" << arg.entryId;
                            return;
                        }
                        self->m_acquisitionEntries[arg.entryId].frequencyLimit = arg.acquisitionFrequencyLimit;
                        self->m_acquisitionEntries[arg.entryId].minimumWaitDuration =
                            std::chrono::steady_clock::duration(
                                std::chrono::nanoseconds(1000000000ull / arg.acquisitionFrequencyLimit));
                    }
#undef MATCH
                },
                req);
        }
    }
}

void AcquisitionHub::startAcquisitionTimer() {
    //
}

void AcquisitionHub::stopAcquisitionTimer() {
    //
}

void AcquisitionHub::readFrequencyFeedbackReportIntervalFromQSettings() {
    QSettings settings;
    using namespace std::chrono_literals;

    auto value = settings.value("Acquisition/FrequencyFeedbackReportInterval", 200).toInt();
    m_frequencyFeedbackReportInterval = 1ms * value;
}
