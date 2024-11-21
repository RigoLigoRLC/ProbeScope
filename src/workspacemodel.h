
#pragma once

#include "diskbackedstorage.h"
#include "result.h"
#include "symbolbackend.h"
#include <QColor>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QTreeWidgetItem>


class ProbeLibHost;

/**
 * @brief WorkspaceModel does all the bookkeeping and backend data storing for the UI (so all the complicated logics
 * happen here.) When the UI does a anything it will call the API of WorkspaceModel, and only modify UI state when
 * the API call succeeded. WorkspaceModel also sends signals to UI when certain thing is going to be updated, for
 * example, when the plots has new data points, it will tell the UI to update it, and UI will have to fetch data
 * from WorkspaceModel.
 */

class WorkspaceModel final : public QObject {
    Q_OBJECT
public:
    WorkspaceModel(QObject *parent = nullptr);
    ~WorkspaceModel();

    enum class Error {
        NoError,
        InvalidWatchExpression,
        InvalidWatchEntryIndex,
        InvalidPlotAreaId,
    };

    struct WatchEntryConfigurations {
        uint64_t entryId;

        // Acquisition properties
        QString expression;
        int acquisitionFrequencyLimit;

        // Data processing properties
        double coefficient;

        // Plot painting properties
        int associatedPlotAreaId;
        QColor plotColor;
        int plotThickness;
        Qt::PenStyle plotStyle;
    };

    /**
     * @brief Load a specified symbol file.
     * This causes all watch entry expressions discarded whether it succeeded or not.
     * @param path File path of symbol file
     * @return On success: nothing. On fail: error code of SymbolBackend.
     */
    Result<void, SymbolBackend::Error> loadSymbolFile(QString path);

    /**
     * @brief Returns path of symbol file. Returns empty string when no file was loaded or last symbol file load failed.
     */
    QString getSymbolFilePath() const { return m_symbolBackend->getSymbolFilePath().unwrapOr(QString()); }

    /**
     * @brief Get the Symbol Backend object, when the UI part appropriately needs it
     * @return SymbolBackend const*
     */
    SymbolBackend *const getSymbolBackend() const { return m_symbolBackend; }

    /**
     * @brief Get the ProbeLibHost object, when the UI part appropriately needs it
     * @return ProbeLibHost const*
     */
    ProbeLibHost *const getProbeLibHost() const { return m_probeLibHost; }

    /**
     * @brief Builds the symbol tree for the current loaded symbol file.
     * @return On success: A list of root tree items. On fail: error code of SymbolBackend.
     */
    Result<QList<QTreeWidgetItem *>, SymbolBackend::Error> buildSymbolTree();


    /**
     * @brief Request adding a new plot area.
     * WorkspaceModel doesn't care about the detailed UI properties; it only manages individual plot areas' IDs.
     * @return On success: the new plot area's ID. On fail: error code.
     */
    Result<int, Error> addPlotArea();

    /**
     * @brief Request removing a plot area.
     * This will unlink the plots still on the plot area.
     * @return On success: nothing. On fail: error code.
     */
    Result<void, Error> removePlotArea(int areaId);

    /**
     * @brief Request adding a new watch entry.
     * @param expression The expression of the desired variable to watch.
     * @return On success: the assigned watch entry configuration and unique ID. On fail: error code.
     */
    Result<QPair<uint64_t, WatchEntryConfigurations>, Error> addWatchEntry(QString expression);

    /**
     * @brief Remove a watch entry entirely. Would trigger a signal to notify the UI to remove it from associated plot
     * area. This would also remove all the data already recorded for this watch entry, and free all its occupying
     * blocks.
     * @param entryId unique ID of watch entry
     * @return On success: nothing. On fail: error code.
     */
    Result<void, Error> removeWatchEntry(uint64_t entryId);

    void createTest();

private:
    bool m_isWorkspaceDirty = false; ///< Dirty flag, changed when anything modifies workspace and cleared on saving.

    SymbolBackend *m_symbolBackend; ///< Symbol backend.
    ProbeLibHost *m_probeLibHost;   ///< The object that does all communication with debug probes.

    DiskBackedStorage m_backingStore; ///< Disk backed storage for data logging.
};
