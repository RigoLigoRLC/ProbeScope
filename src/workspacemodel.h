
#pragma once

#include "acquisitionbuffer.h"
#include "acquisitionbufferchannel.h"
#include "acquisitionhub.h"
#include "atomic_queue/atomic_queue.h"
#include "diskbackedstorage.h"
#include "expressionevaluator/bytecode.h"
#include "models/watchentrymodel.h"
#include "qcustomplot.h"
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
        SaveFileCannotOpen,
        WatchExpressionParseFailed,
        InvalidWatchEntryIndex,
        InvalidPlotAreaId,
        InvalidWatchEntryProperty,
        InvalidWatchEntryPropertyValue,
    };
    Q_ENUM(Error);

    using PlotAreas = QSet<size_t>;
    struct WatchEntry {
        // Acquisition properties
        QString expression;
        QString displayName;
        int acquisitionFrequencyLimit;

        // Data processing properties
        double coefficient;

        // Plot painting properties
        PlotAreas associatedPlotAreas;
        QColor plotColor;
        int plotThickness;
        Qt::PenStyle plotStyle;
        // TODO: support scatter graph?

        // Plot data (can be shared between multiple plots)
        QSharedPointer<QCPGraphDataContainer> data;

        // Expression evaluation misc
        std::optional<ExpressionEvaluator::Bytecode> exprBytecode;            ///< Raw bytecode from parser
        std::optional<ExpressionEvaluator::Bytecode> staticOptimizedBytecode; ///< Bytecode optimized based on symbols
        std::optional<ExpressionEvaluator::Bytecode> runtimeBytecode; ///< Bytecode after resolving single_eval_block
        QString errorMessage;                                         ///< Error message returned on parse/optimization
    };

    /**
     * @brief Load a specified symbol file.
     * This causes all watch entry expressions discarded whether it succeeded or not.
     * @param path File path of symbol file
     * @return On success: nothing. On fail: error code of SymbolBackend.
     */
    Result<void, SymbolBackend::Error> loadSymbolFile(QString path);

    /**
     * @brief Returns if there's a symbol file currently loaded. May be used to determine UI state.
     */
    bool isSymbolFileLoaded() { return m_symbolBackend->isSymbolFileLoaded(); }

    /**
     * @brief Returns path of symbol file. Returns empty string when no file was loaded or last symbol file load failed.
     */
    QString getSymbolFilePath() const { return m_symbolBackend->getSymbolFilePath().unwrapOr(QString()); }

    /**
     * @brief Get the Symbol Backend object, when the UI part appropriately needs it
     * @return SymbolBackend const*
     */
    SymbolBackend *const getSymbolBackend() const { return m_symbolBackend.get(); }

    /**
     * @brief Get the ProbeLibHost object, when the UI part appropriately needs it
     * @return ProbeLibHost const*
     */
    ProbeLibHost *const getProbeLibHost() const { return m_probeLibHost.get(); }

    /**
     * @brief Get the acquisition buffer channel interface object. This object exists as a channel between acquisition
     * thread and workspace model.
     * @return IAcquisitionBufferChannel* const
     */
    IAcquisitionBufferChannel::p const getAcquisitionBufferChannel() { return m_acquisitionBuffer; }

    /**
     * @brief Get the Watch entry Qt model wrapper, when the UI part appropriately needs it
     * @return WatchEntryModel*
     */
    WatchEntryModel *getWatchEntryModel() const { return m_watchEntryModel.get(); }

    /**
     * @brief Builds the symbol tree for the current loaded symbol file.
     * @return On success: A list of root tree items. On fail: error code of SymbolBackend.
     */
    Result<QList<QTreeWidgetItem *>, SymbolBackend::Error> buildSymbolTree();

    /**
     * @brief Anyone who whats a new plot area must call this function, it will request the main window add a new plot
     * area. It in turn sends a signal to the UI with an ID (WorkspaceModel manages the plot area ID) and let the UI
     * create the plot area. Because WorkspaceModel lives on the main thread, the signal is synchronous and by the time
     * the emit call has ended the plot area is immediately available.
     * When project load requires a new plot area, this function is called internally;
     * When user wants a new plot area, UI code will call this function and it signals back to UI with an exact ID.
     * @return On success: the new plot area's ID. On fail: error code.
     */
    Result<size_t, Error> addPlotArea();

    /**
     * @brief Request removing a plot area.
     * This will unlink the plots still on the plot area.
     * @return On success: nothing. On fail: error code.
     */
    Result<void, Error> removePlotArea(size_t areaId);

    /**
     * @brief The UI will set the active plot area once an area gets a focus.
     * @param areaId  Plot area ID.
     * @return On success: nothing. On fail: error code.
     */
    Result<void, Error> setActivePlotArea(size_t areaId);

    /**
     * @brief Request adding a new watch entry. This is a request from UI, and UI changes will be reflected back with
     * other signals.
     * @param expression The expression of the desired variable to watch.
     * @param areaId An optional plot area ID from the UI, if the user specified which area to add it to.
     * @return On success: the assigned watch entry ID. On fail: error code.
     */
    Result<size_t, Error> addWatchEntry(QString expression, std::optional<size_t> areaId);

    /**
     * @brief Remove a watch entry entirely. Would trigger a signal to notify the UI to remove it from associated plot
     * area. This would also remove all the data already recorded for this watch entry, and free all its occupying
     * blocks.
     * @param entryId unique ID of watch entry
     * @param fromUi If this removal request comes from Watch Entry Panel, set this to true. This prevents the workspace
     * model from doing an extraneous row removal needed to perform when the removal request was internal to workspace.
     * @return On success: nothing. On fail: error code.
     */
    Result<void, Error> removeWatchEntry(uint64_t entryId, bool fromUi = false);

    /**
     * @brief Get the graph data container associated with a watch entry. This is used by UI when it's requested to add
     * a graph.
     * @param entryId Watch entry ID.
     * @return On success: data container. On fail: error code.
     */
    Result<QSharedPointer<QCPGraphDataContainer>, Error> getWatchEntryDataContainer(size_t entryId);

    /**
     * @brief Get a specific property of a watch entry's graph. WatchEntryModel and UI side both uses this.
     * @param entryId Watch entry ID.
     * @param prop Editable property on the watch entry table UI.
     * @return On success: data. On fail: error code.
     */
    Result<QVariant, Error> getWatchEntryGraphProperty(size_t entryId, WatchEntryModel::Columns prop);

    /**
     * @brief Set a specific property of a watch entry's graph. WatchEntryModel uses this, UI is notified afterwards.
     * @param entryId Watch entry ID.
     * @param prop Editable property on the watch entry table UI.
     * @param data Data value.
     * @return On success: nothing. On fail: error code.
     */
    Result<void, Error> setWatchEntryGraphProperty(size_t entryId, WatchEntryModel::Columns prop, QVariant data);

    /**
     * @brief This function is called periodically by UI when acquisition is active, used to notify the backend to pull
     * buffered data from the buffer channels and append them to each watch entry's graph data container.
     * @return Whether any data has been successfully fetched. This is used for the UI to determine when to stop the
     * refresh timer after acquisition has been requested to stop.
     */
    Q_SLOT bool pullBufferedAcquisitionData();

    /**
     * @brief This function is called by the UI or anywhere else that is able to start the acquisition to notify the
     * workspace model that an acquisition session should begin.
     */
    void notifyAcquisitionStarted();

    /**
     * @brief This function is called by the UI or anywhere else that is able to start the acquisition to notify the
     * workspace model that the acquisition session should stop.
     */
    void notifyAcquisitionStopped();

    /**
     * @brief Get the acquisition status from the acquisition hub.
     */
    bool isAcquisitionActive() { return m_acquisitionHub->isAcquisitionActive(); }

    /**
     * @brief Save acquisition data to CSV file into the file name provided.
     *
     * @param fileName destination CSV file name.
     * @return On success: nothing. On error: an error code.
     */
    Result<void, Error> saveAcquisitionData(QString fileName);

private:
    size_t getNextPlotAreaId() { return m_maxPlotAreaId++; }
    size_t getNextWatchEntryId() { return m_maxWatchEntryId++; }
    QColor getPlotColorBasedOnEntryId(size_t id) { return m_defaultPlotColors[id % m_defaultPlotColors.size()]; }

    // updateAcquisition: if true, then will send a bytecode change request. This should be set to true when trying to
    // hot edit the expression after it's been added to acquisition hub
    void refreshExpressionBytecodes(bool updateAcquisition = false);
    bool refreshExpressionBytecodes(size_t entryId, bool updateAcquisition = false);

private slots:
    void sltAcquisitionFrequencyFeedbackArrived(size_t entryId);

private:
    bool m_isWorkspaceDirty = false; ///< Dirty flag, changed when anything modifies workspace and cleared on saving.
    size_t m_maxWatchEntryId = 0;  ///< The self incrementing ID for assigning watch entry IDs. Cleared on project load.
    size_t m_maxPlotAreaId = 0;    ///< The self incrementing ID for assigning plot area IDs. Cleared on project load.
    size_t m_activePlotAreaId = 0; ///< The current active plot area. New plots are assigned if no area is specified.

    QMap<size_t, WatchEntry> m_watchEntries; ///< All watch entries.
    QSet<size_t> m_plotAreaIds;              ///< Bookkeeping of plot area IDs (Not much of use for now)

    std::unique_ptr<SymbolBackend> m_symbolBackend;     ///< Symbol backend.
    std::unique_ptr<ProbeLibHost> m_probeLibHost;       ///< The object that does all communication with debug probes.
    std::unique_ptr<WatchEntryModel> m_watchEntryModel; ///< Qt Model interface to access watch entry data
    std::unique_ptr<AcquisitionHub> m_acquisitionHub;

    AcquisitionBuffer::Timepoint m_acquisitionStartTime; /// The timepoint when acquisition started
    std::vector<QColor> m_defaultPlotColors;             ///< Default plot colors assigned based on watch entry ID

    std::shared_ptr<AcquisitionBuffer> m_acquisitionBuffer; ///< In-memory data buffer between acquisition thread and UI
    DiskBackedStorage m_backingStore;                       ///< Disk backed storage for data logging.

signals:
    void requestAddPlotArea(size_t areaId);
    void requestRemovePlotArea(size_t areaId);
    void requestAssignGraphOnPlotArea(size_t entryId, size_t areaId);
    void requestUnassignGraphOnPlotArea(size_t entryId, size_t areaId);

    ///@brief This signal is emitted when plotting related properties are changed so that UI can update the plot.
    void plotPropertyChanged(size_t entryId, WatchEntryModel::Columns prop, QVariant data);

    void feedbackAcquisitionStopped();
};

Q_DECLARE_METATYPE(WorkspaceModel::PlotAreas);
