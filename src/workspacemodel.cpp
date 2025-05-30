
#include "workspacemodel.h"
#include "expressionevaluator/optimizer.h"
#include "expressionevaluator/parser.h"
#include "probelibhost.h"
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <cstdint>

// #define BLOCK_ALLOC_DEBUG_MSG

WorkspaceModel::WorkspaceModel(QObject *parent) : QObject(parent) {
    QSettings settings;

    // Initialize cache file mapping
    auto backingFileName = settings.value("CacheFile/Path").toString();
    m_backingStore.setFileName(backingFileName);

    QFileInfo fileInfo(backingFileName);
    if (backingFileName.isEmpty() || fileInfo.isDir() || !fileInfo.isWritable() ||
        !m_backingStore.open(QFile::ReadWrite)) {
        // User defined file is not usable, try default temp path
        auto tempDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        fileInfo.setFile(tempDir);
        if (!fileInfo.dir().exists()) {
            QDir().mkpath(tempDir);
        }
        m_backingStore.setFileName(QDir::toNativeSeparators(tempDir + QDir::separator() + "probescope_cache"));
        if (!m_backingStore.open(QFile::ReadWrite)) {
            do {
                auto choice = QMessageBox::warning(
                    nullptr, tr("Cannot open cache file"),
                    tr("Cache file %1 cannot be opened with read/write access.\nDo you wish to choose "
                       "another place for cache file?")
                        .arg(m_backingStore.fileName()),
                    QMessageBox::Yes | QMessageBox::Abort, QMessageBox::Yes);

                switch (choice) {
                    default:
                    case QMessageBox::Abort: exit(1); return;

                    case QMessageBox::Yes: {
                        auto chosenFile = QFileDialog::getSaveFileName(
                            nullptr, tr("Select where to save cache file..."), fileInfo.canonicalPath());

                        if (chosenFile.isEmpty())
                            continue;

                        m_backingStore.setFileName(chosenFile);
                        break; // Drops to the loop criteria of while
                    }
                }
            } while (!m_backingStore.open(QFile::ReadWrite));
        }
    }

    bool isOk = false;
    auto mappingBlockLimit = settings.value("CacheFile/BlockCountLimit").toULongLong(&isOk);
    if (!isOk) {
        mappingBlockLimit = 32; // Failsafe option
    }
    m_backingStore.setBlockCountLimit(mappingBlockLimit);

    // Reuse clear routine as initialization here
    m_backingStore.clearStorage();

    if (!m_backingStore.storageMapping()) {
        QMessageBox::critical(nullptr, tr("Cannot map cache file"),
                              tr("Cache file cannot be mapped to system memory. ProbeScope will now quit."));
        exit(1);
    }

    // Create symbol backend.
    m_symbolBackend = std::make_unique<SymbolBackend>(this);

    // Create probe lib host.
    m_probeLibHost = std::make_unique<ProbeLibHost>(this);

    // Create watch entries Qt model wrapper
    m_watchEntryModel = std::make_unique<WatchEntryModel>(this, this);

    // Create acquisition buffer
    m_acquisitionBuffer = std::make_shared<AcquisitionBuffer>();
    connect(m_acquisitionBuffer.get(), &AcquisitionBuffer::frequencyFeedbackArrived, this,
            &WorkspaceModel::sltAcquisitionFrequencyFeedbackArrived, Qt::QueuedConnection);

    // Create acquisition hub
    m_acquisitionHub = std::make_unique<AcquisitionHub>(m_probeLibHost.get(), this);
    m_acquisitionHub->setAcquisitionBufferChannel(getAcquisitionBufferChannel());
    connect(m_acquisitionHub.get(), &AcquisitionHub::acquisitionStopped, this,
            &WorkspaceModel::feedbackAcquisitionStopped, Qt::QueuedConnection);

    // TODO: Make default colors adjustable
    // Colors arbitrarily picked from https://gist.github.com/afcotroneo/716a864e9f7ba1bde4d2100313ad9f75/
    //["#ea5545", "#f46a9b", "#ef9b20", "#edbf33", "#ede15b", "#bdcf32", "#87bc45", "#27aeef", "#b33dc6"]
    m_defaultPlotColors.emplace_back("#ea5545");
    m_defaultPlotColors.emplace_back("#f46a9b");
    m_defaultPlotColors.emplace_back("#ef9b20");
    m_defaultPlotColors.emplace_back("#edbf33");
    m_defaultPlotColors.emplace_back("#ede15b");
    m_defaultPlotColors.emplace_back("#bdcf32");
    m_defaultPlotColors.emplace_back("#87bc45");
    m_defaultPlotColors.emplace_back("#27aeef");
    m_defaultPlotColors.emplace_back("#b33dc6");
}

WorkspaceModel::~WorkspaceModel() {}

Result<void, SymbolBackend::Error> WorkspaceModel::loadSymbolFile(QString path) {
    auto result = m_symbolBackend->switchSymbolFile(path);

    // TODO: discard watch entries when symbol file differs

    // Refresh watch entries' bytecodes
    refreshExpressionBytecodes(true);

    if (result.isOk()) {
        // Initialize plot area if we don't have any
        if (m_plotAreaIds.isEmpty()) {
            addPlotArea();
        }
        return Ok();
    } else {
        return Err(result.unwrapErr());
    }
}

Result<size_t, WorkspaceModel::Error> WorkspaceModel::addPlotArea() {
    auto id = m_maxPlotAreaId++;
    m_plotAreaIds.insert(id);
    emit requestAddPlotArea(id);
    return Ok(id);
}

Result<void, WorkspaceModel::Error> WorkspaceModel::removePlotArea(size_t areaId) {
    if (!m_plotAreaIds.contains(areaId)) {
        return Err(Error::InvalidPlotAreaId);
    }
    emit requestRemovePlotArea(areaId);
    return Ok();
}

Result<void, WorkspaceModel::Error> WorkspaceModel::setActivePlotArea(size_t areaId) {
    if (!m_plotAreaIds.contains(areaId)) {
        return Err(Error::InvalidPlotAreaId);
    }
    m_activePlotAreaId = areaId;
    return Ok();
}

Result<size_t, WorkspaceModel::Error> WorkspaceModel::addWatchEntry(QString expression, std::optional<size_t> areaId) {
    // To satisfy stupid QSet initializer
    size_t destAreaId[2];

    if (areaId.has_value() && m_plotAreaIds.contains(areaId.value())) {
        destAreaId[0] = areaId.value();
    } else if (m_plotAreaIds.contains(m_activePlotAreaId)) {
        destAreaId[0] = m_activePlotAreaId;
        if (areaId.has_value()) {
            qWarning() << "Invalid plot area ID specified:" << areaId.value() << "falling back to active area";
        }
    } else {
        qCritical() << "Plot area ID unspecified/invalid AND active area ID invalid; this is a bug";
        return Err(Error::InvalidPlotAreaId);
    }

    auto parseResult = ExpressionEvaluator::Parser::parseToBytecode(expression);
    if (parseResult.isErr()) {
        (qWarning() << "Parse of expression" << expression << "failed:").noquote() << parseResult.unwrapErr();
    }


    // TODO: Make default config adjustable
    auto entryId = getNextWatchEntryId();
    m_watchEntries[entryId] = WatchEntry{
        .expression = expression,
        .displayName = tr("Graph %1").arg(entryId),
        .acquisitionFrequencyLimit = 0, // TODO: UNUSED
        .coefficient = 1,
        .associatedPlotAreas = {destAreaId, destAreaId + 1},
        .plotColor = getPlotColorBasedOnEntryId(entryId),
        .plotThickness = 1,
        .plotStyle = Qt::SolidLine,
        .data = QSharedPointer<QCPGraphDataContainer>(new QCPGraphDataContainer),
        .exprBytecode = (parseResult.isOk() ? std::optional(parseResult.unwrap())
                                            : std::optional<ExpressionEvaluator::Bytecode>{{}}
          ),
        .staticOptimizedBytecode = std::nullopt,
        .runtimeBytecode = std::nullopt
    };
    auto &entry = m_watchEntries[entryId];
    refreshExpressionBytecodes(entryId);

    emit requestAssignGraphOnPlotArea(entryId, destAreaId[0]);

    // Add to watch entry model
    m_watchEntryModel->addRowForEntry(entryId);

    // Add to acquisition buffer channels
    m_acquisitionBuffer->addChannel(entryId);

    // Add to acquisition hub. If an entry doesn't have a valid runtime bytecode, it's disabled at first.
    m_acquisitionHub->addWatchEntry(entryId, entry.runtimeBytecode.has_value(),
                                    entry.runtimeBytecode.has_value() ? entry.runtimeBytecode.value()
                                                                      : ExpressionEvaluator::Bytecode(),
                                    entry.acquisitionFrequencyLimit);

    return Err(Error::NoError);
}

Result<void, WorkspaceModel::Error> WorkspaceModel::removeWatchEntry(uint64_t entryId, bool fromUi) {
    if (!m_watchEntries.contains(entryId)) {
        qCritical() << "Watch Entry ID" << entryId << "does not exists and cannot remove it";
        return Err(Error::InvalidWatchEntryIndex);
    }

    auto entry = m_watchEntries.take(entryId);

    // Remove from plot areas
    for (auto i : entry.associatedPlotAreas) {
        emit requestUnassignGraphOnPlotArea(entryId, i);
    }

    // Remove from watch entry model
    if (!fromUi) {
        m_watchEntryModel->removeEntry(entryId);
    }

    // Remove from acquisition hub
    m_acquisitionHub->removeWatchEntry(entryId);

    return Ok();
}

Result<QSharedPointer<QCPGraphDataContainer>, WorkspaceModel::Error>
    WorkspaceModel::getWatchEntryDataContainer(size_t entryId) {
    if (!m_watchEntries.contains(entryId)) {
        qCritical() << "Watch entry ID" << entryId << "does not exists and cannot get data container";
        return Err(Error::InvalidWatchEntryIndex);
    }

    return Ok(m_watchEntries[entryId].data);
}

Result<QVariant, WorkspaceModel::Error> WorkspaceModel::getWatchEntryGraphProperty(size_t entryId,
                                                                                   WatchEntryModel::Columns prop) {
    if (!m_watchEntries.contains(entryId)) {
        qCritical() << "Watch entry ID" << entryId << "does not exists and cannot get property";
        return Err(Error::InvalidWatchEntryIndex);
    }

    auto &entry = m_watchEntries[entryId];
    switch (prop) {
        case WatchEntryModel::Color: return Ok(QVariant(entry.plotColor));
        case WatchEntryModel::DisplayName: return Ok(QVariant(entry.displayName));
        case WatchEntryModel::Expression: return Ok(QVariant(entry.expression));
        case WatchEntryModel::PlotAreas: return Ok(QVariant::fromValue(entry.associatedPlotAreas));
        case WatchEntryModel::Thickness: return Ok(QVariant(entry.plotThickness));
        case WatchEntryModel::LineStyle: return Ok(QVariant::fromValue(entry.plotStyle));
        case WatchEntryModel::FrequencyLimit: return Ok(QVariant(entry.acquisitionFrequencyLimit));
        case WatchEntryModel::FrequencyFeedback:
            return Ok(QVariant(m_acquisitionBuffer->getChannelFrequencyFeedback(entryId)));
        case WatchEntryModel::ExpressionOkay:
            return Ok(QVariant(entry.exprBytecode.has_value() && entry.runtimeBytecode.has_value()));
        default: return Ok(QVariant());
    }
}

Result<void, WorkspaceModel::Error>
    WorkspaceModel::setWatchEntryGraphProperty(size_t entryId, WatchEntryModel::Columns prop, QVariant data) {
    if (!m_watchEntries.contains(entryId)) {
        qCritical() << "Watch entry ID" << entryId << "does not exists and cannot set property";
        return Err(Error::InvalidWatchEntryIndex);
    }

    auto needsEmitPropChangedSignal = [&]() -> Result<bool, Error> {
        auto &entry = m_watchEntries[entryId];
        switch (prop) {
            case WatchEntryModel::Color:
                if (!data.canView(QMetaType(QMetaType::QColor))) {
                    return Err(Error::InvalidWatchEntryPropertyValue);
                }
                entry.plotColor = data.value<QColor>();
                return Ok(true);
            case WatchEntryModel::DisplayName: entry.displayName = data.toString(); return Ok(true);
            case WatchEntryModel::Expression: {
                // FIXME: this parse code appeared twice. Abstract it away
                auto expression = data.toString();
                entry.expression = expression;
                auto parseResult = ExpressionEvaluator::Parser::parseToBytecode(expression);
                if (parseResult.isErr()) {
                    (qWarning() << "Parse of expression" << expression << "failed:").noquote()
                        << parseResult.unwrapErr();

                    entry.exprBytecode = std::nullopt;
                    entry.staticOptimizedBytecode = std::nullopt;
                    entry.runtimeBytecode = std::nullopt;
                    m_acquisitionHub->setEntryEnabled(entryId, false); // You ded
                    // TODO: Pop error message
                    return Ok(false); // We still return Ok here because we want the model to emit dataChanged
                }
                entry.exprBytecode = {parseResult.unwrap()};
                refreshExpressionBytecodes(entryId, true);
                return Ok(false);
            }
            case WatchEntryModel::PlotAreas: {
                auto &&newSet = data.value<PlotAreas>();
                auto &oldSet = entry.associatedPlotAreas;
                auto additions = newSet - oldSet;
                auto removals = oldSet - newSet;
                for (auto i : additions) {
                    emit requestAssignGraphOnPlotArea(entryId, i);
                }
                for (auto i : removals) {
                    emit requestUnassignGraphOnPlotArea(entryId, i);
                }
                entry.associatedPlotAreas = newSet;
                return Ok(false);
            }
            case WatchEntryModel::Thickness:
                if (!data.canView(QMetaType(QMetaType::Int))) {
                    return Err(Error::InvalidWatchEntryPropertyValue);
                }
                entry.plotThickness = data.toInt();
                return Ok(true);
            case WatchEntryModel::LineStyle:
                if (!data.canView(QMetaType(qMetaTypeId<Qt::PenStyle>()))) {
                    return Err(Error::InvalidWatchEntryPropertyValue);
                }
                entry.plotStyle = data.value<Qt::PenStyle>();
                return Ok(true);
            case WatchEntryModel::FrequencyLimit:
                if (!data.canView(QMetaType(QMetaType::Int))) {
                    return Err(Error::InvalidWatchEntryPropertyValue);
                }
                entry.acquisitionFrequencyLimit = data.toInt();
                return Ok(true);
            case WatchEntryModel::MaxColumns:
            case WatchEntryModel::FrequencyFeedback:
            case WatchEntryModel::ExpressionOkay: return Err(Error::InvalidWatchEntryProperty);
        }
        return Err(Error::InvalidWatchEntryProperty);
    }();

    if (needsEmitPropChangedSignal.isErr()) {
        return Err(needsEmitPropChangedSignal.unwrapErr());
    } else if (needsEmitPropChangedSignal.unwrap()) {
        emit plotPropertyChanged(entryId, prop, data);
    }

    return Ok();
}

bool WorkspaceModel::pullBufferedAcquisitionData() {
    size_t count = 0;
    for (auto it = m_watchEntries.cbegin(); it != m_watchEntries.cend(); ++it) {
        m_acquisitionBuffer->drainChannel(it.key(), [&](AcquisitionBuffer::Timepoint t, AcquisitionBuffer::Value v) {
            ++count;
            it->data->add({AcquisitionBuffer::timepointToMillisecond(m_acquisitionStartTime, t),
                           AcquisitionBuffer::valueToDouble(v)});
        });
    }
    // qDebug() << "Processed" << count << "sample points";
    return count != 0;
}

void WorkspaceModel::notifyAcquisitionStarted() {
    // FIXME: When workspace still has data, notify the user whether to discard or save them
    foreach (auto &i, m_watchEntries) {
        i.data->clear();
    }
    m_acquisitionStartTime = AcquisitionBuffer::Clock::now();
    m_acquisitionHub->startAcquisition();
}

void WorkspaceModel::notifyAcquisitionStopped() {
    m_acquisitionHub->stopAcquisition();

    // Let all entries display their limit value again
    m_watchEntryModel->notifyFrequencyFeedbackChanged();

// DEBUG: Write data point to disk
#if 0
    auto data = m_watchEntries[0].data;
    QFile f("D:/tmp/acquisition.csv");
    f.open(QFile::WriteOnly);
    QTextStream ts(&f);
    for (auto it = data->begin(); it != data->end(); it++) {
        ts << it->mainKey() << ',' << it->mainValue() << ",\n";
    }
#endif
}

Result<void, WorkspaceModel::Error> WorkspaceModel::saveAcquisitionData(QString fileName) {
    size_t maxIndex = std::max_element(m_watchEntries.cbegin(), m_watchEntries.cend(), [](auto lhs, auto rhs) {
                          return lhs.data->size() < rhs.data->size();
                      })->data->size();

    QFile f(fileName);
    if (!f.open(QFile::WriteOnly)) {
        return Err(Error::SaveFileCannotOpen);
    }

    QTextStream ts(&f);

    for (auto var : m_watchEntries) {
        ts << "time_" + var.displayName << ",val_" + var.displayName + ',';
    }
    ts << '\n';

    for (size_t i = 0; i < maxIndex; ++i) {
        for (auto var : m_watchEntries) {
            if (i >= var.data->size()) {
                ts << ",,";
            } else {
                auto &sample = *var.data->at(i);
                ts << sample.key << ',' << sample.value << ',';
            }
        }
        ts << '\n';
    }

    ts.flush();
    f.close();
    return Ok();
}

/***************************************** INTERNAL UTILS *****************************************/

void WorkspaceModel::refreshExpressionBytecodes(bool updateAcquisition) {
    //
    for (auto [id, _] : m_watchEntries.asKeyValueRange()) {
        refreshExpressionBytecodes(id, updateAcquisition);
    }
}

bool WorkspaceModel::refreshExpressionBytecodes(size_t entryId, bool updateAcquisition) {
    //
    if (auto it = m_watchEntries.find(entryId); it == m_watchEntries.end()) {
        qCritical() << "Entry ID" << entryId << "not found!";
        return false;
    } else if (auto &entry = *it; !entry.exprBytecode.has_value()) {
        qWarning() << "Trying to refresh expression bytecode on an entry whose evaluation didn't even pass:" << entryId;
        return false;
    } else {
        auto &bytecode = entry.exprBytecode.value();
        auto optimizeResult = ExpressionEvaluator::StaticOptimize(bytecode, m_symbolBackend.get());
        if (optimizeResult.isErr()) {
            (qWarning() << "Static optimization of bytecode failed. Disassembly:\n").noquote()
                << bytecode.disassemble() << "Error message:" << optimizeResult.unwrapErr();
            entry.staticOptimizedBytecode = std::nullopt;
            entry.runtimeBytecode = std::nullopt;
            goto failAndTemporarilyDisable;
        }

        // FIXME: single eval block is not considered here at all. Implement this in the future
        entry.staticOptimizedBytecode = optimizeResult.unwrap();
        entry.runtimeBytecode = optimizeResult.unwrap();

        if (updateAcquisition) {
            m_acquisitionHub->changeWatchEntryBytecode(entryId, entry.runtimeBytecode.value());
            m_acquisitionHub->setEntryEnabled(entryId, true); // You passed
        }
        return true;
    }

failAndTemporarilyDisable:
    if (updateAcquisition) {
        m_acquisitionHub->setEntryEnabled(entryId, false);
    }
    return false;
}

void WorkspaceModel::sltAcquisitionFrequencyFeedbackArrived(size_t entryId) {
    m_watchEntryModel->notifyFrequencyFeedbackChanged(entryId);
}
