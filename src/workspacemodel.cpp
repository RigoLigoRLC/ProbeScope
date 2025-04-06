
#include "workspacemodel.h"
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
#include <limits>


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
    // TODO: refresh watch entries' bytecode

    if (result.isOk()) {
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
        return Err(Error::WatchExpressionParseFailed);
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
        .exprBytecode = parseResult.unwrap(),
        .staticOptimizedBytecode = parseResult.unwrap(), // FIXME: Static optimization!!!
        .runtimeBytecode = parseResult.unwrap()  // FIXME: Should be same as static optimized or evaluated on first run
    };

    emit requestAssignGraphOnPlotArea(entryId, destAreaId[0]);

    // TODO: add to watch entry model

    return Err(Error::NoError);
}

Result<void, WorkspaceModel::Error> WorkspaceModel::removeWatchEntry(uint64_t entryId) {
    if (!m_watchEntries.contains(entryId)) {
        qCritical() << "Watch Entry ID" << entryId << "does not exists and cannot remove it";
        return Err(Error::InvalidWatchEntryIndex);
    }

    auto entry = m_watchEntries.take(entryId);

    // Remove from plot areas
    for (auto i : entry.associatedPlotAreas) {
        emit requestUnassignGraphOnPlotArea(entryId, i);
    }

    // TODO: remove from watch entry model
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
        default: return Ok(QVariant());
    }
}

Result<void, WorkspaceModel::Error>
    WorkspaceModel::setWatchEntryGraphProperty(size_t entryId, WatchEntryModel::Columns prop, QVariant data) {
    if (!m_watchEntries.contains(entryId)) {
        qCritical() << "Watch entry ID" << entryId << "does not exists and cannot set property";
        return Err(Error::InvalidWatchEntryIndex);
    }

    // FIXME: fill in function!!!

    return Ok();
}

/***************************************** INTERNAL UTILS *****************************************/
