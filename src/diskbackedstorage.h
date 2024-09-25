
#pragma once

#include "result.h"
#include <QByteArray>
#include <QFile>
#include <QSet>
#include <vector>



class DiskBackedStorage {
public:
    ~DiskBackedStorage();

    enum class Error {
        DataStorageFull,
        CacheMemoryMappingFail,
    };

    QString fileName() const { return m_backingStore.fileName(); }
    void setFileName(QString name) { m_backingStore.setFileName(name); }
    bool open(QIODevice::OpenMode mode) { return m_backingStore.open(mode); }

    void setBlockCountLimit(uint64_t blockCount) { m_backingFileBlockCountLimit = blockCount; }

    uint8_t *storageMapping() { return m_storageMapping; }

    /**
     * @brief Get the internal storage block size.
     *
     * @return uint64_t Storage block size in bytes.
     */
    uint64_t getStorageBlockSize() { return m_blockSize; }

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
