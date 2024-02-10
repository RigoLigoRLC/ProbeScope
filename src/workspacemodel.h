
#pragma once

#include "result.h"
#include "symbolbackend.h"
#include <QColor>
#include <QFile>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QTreeWidgetItem>

/**
 * @brief WorkspaceModel does all the bookkeeping and backend data storing for the UI (so all the complicated logics
 * happen here.) When the UI does a anything it will call the API of WorkspaceModel, and only modify UI state when the
 * API call succeeded. WorkspaceModel also sends signals to UI when certain thing is going to be updated, for example,
 * when the plots has new data points, it will tell the UI to update it, and UI will have to fetch data from
 * WorkspaceModel.
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
        DataStorageFull,
        CacheMemoryMappingFail,
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
     *
     * @param path File path of symbol file
     * @return On success: nothing. On fail: error code of SymbolBackend.
     */
    Result<void, SymbolBackend::Error> loadSymbolFile(QString path);

    /**
     * @brief Get the Symbol Backend object, when the UI part appropriately needs it
     *
     * @return SymbolBackend const*
     */
    SymbolBackend *const getSymbolBackend() { return m_symbolBackend; }

    /**
     * @brief Builds the symbol tree for the current loaded symbol file.
     *
     * @return On success: A list of root tree items. On fail: error code of SymbolBackend.
     */
    Result<QList<QTreeWidgetItem *>, SymbolBackend::Error> buildSymbolTree();

    /**
     * @brief Get the internal storage block size.
     *
     * @return uint64_t Storage block size in bytes.
     */
    uint64_t getStorageBlockSize() { return m_blockSize; }

    /**
     * @brief Request adding a new plot area.
     * WorkspaceModel doesn't care about the detailed UI properties; it only manages individual plot areas' IDs.
     *
     * @return On success: the new plot area's ID. On fail: error code.
     */
    Result<int, Error> addPlotArea();

    /**
     * @brief Request removing a plot area.
     * This will unlink the plots still on the plot area.
     *
     * @return On success: nothing. On fail: error code.
     */
    Result<void, Error> removePlotArea(int areaId);

    /**
     * @brief Request adding a new watch entry.
     *
     * @param expression The expression of the desired variable to watch.
     * @return On success: the assigned watch entry configuration and unique ID. On fail: error code.
     */
    Result<QPair<uint64_t, WatchEntryConfigurations>, Error> addWatchEntry(QString expression);

    /**
     * @brief Remove a watch entry entirely. Would trigger a signal to notify the UI to remove it from associated plot
     * area. This would also remove all the data already recorded for this watch entry, and free all its occupying
     * blocks.
     *
     * @param entryId unique ID of watch entry
     * @return On success: nothing. On fail: error code.
     */
    Result<void, Error> removeWatchEntry(uint64_t entryId);

    void createTest();

private:
    // Private utilities

    /**
     * @brief Clear backing store. Can be inconsistent with other components.
     */
    void clearStorage();

    /**
     * @brief Tries to grow the cache file to specified block count long. This would refresh the file mapping.
     * @return Whether grow succeeded. When grow fails, the file mapping could also become invalid. The acquisition must
     * immediately stop and restore file mapping; otherwise the application would need a hard reset.
     */
    bool growToBlocksLong(uint64_t blockCount);

    inline uint8_t *blockFromSequenceNumber(uint64_t blockSeqNumber);
    inline uint64_t sequenceNumberFromBlock(uint8_t *block);

    Result<uint64_t, Error> allocateBlock();
    void freeBlock(uint64_t blockSeqNumber);
    void printFreeList();

private:
    bool m_isWorkspaceDirty = false; ///< Dirty flag, changed when anything modifies workspace and cleared on saving.

    SymbolBackend *m_symbolBackend; ///< Symbol backend.

    constexpr static uint64_t m_blockSize = 1048576; ///< Storage block size.

    /**
     * @brief Useful information of each storage block. This only contains the most important properties for data
     * logging, but if we need recoverability in the future, this can be expanded to realize that.
     */
    struct StorageBlockHeader {
        uint64_t watchEntryId;          ///< ID of the watch entry that supposed to occupy this block
        uint64_t blockSequenceNumber;   ///< Sequence number among all the blocks a watch entry (backwards reference)
        uint64_t loggedDataPointsCount; ///< How many data points has already been logged into this block
    };
    struct UnallocatedBlockHeader {
        uint64_t unallocatedId;   ///< ID for unallocated (maximum of u64)
        uint64_t nextUnallocated; ///< Next unallocated block sequence number
        uint64_t numberOfBlocks;  ///< How many contigious blocks, including this one, are free
    };

    /**
     * @brief Self incrementing ID for watch entries, used solely to index entries between GUI and model
     */
    uint64_t m_watchEntryId = 0;

    /**
     * @brief A set of unique IDs for plot areas. The top is not self incremented and the first free entry is searched
     * for every time a new plot area is going to be created (because I would consider there'd be a lot more plot
     * entries than plot areas). When plot area is closed, it is also removed from this set, maintaining the
     * consistency.
     */
    QSet<int> m_availablePlotAreas;

    /**
     * @brief The backing storage area for the huge amount of data points. It is mapped to memory with Qt API. On
     * Windows it is possible to use pagefile-backed-sections for better performance, but we're not implementing it
     * here. User can limit the total size of this backing file. Data is stored in block like structures in the mapped
     * memory area, and the block occupation state is managed with a accompanying bitmap. Each watch entry will also
     * remember the block sequence numbers they occupied.
     *
     * We define the blocks to be 1MB for now. Most users' systems don't even use pages this large yet. Each watch entry
     * have to at least occupy one block, and fill its data points into it. When a block is full, new block will be
     * requested. When reaching the file size limit, the acquisition will be forcibly aborted.
     */
    QFile m_backingStore;

    uint8_t *m_storageMapping;
    uint64_t m_freeListFirstBlockSeq;

    uint64_t m_backingFileBlockCountLimit; ///< Only written to at initialization
    uint64_t m_occupiedBlockCount = 0;     ///< Maintained dynamically
    uint64_t m_backingFileBlockCount;      ///< Dynamically grows

    /**
     * @brief Represents the occupied blocks in the storage.
     */
    std::vector<bool> m_storageOccupationBitmap;
};
