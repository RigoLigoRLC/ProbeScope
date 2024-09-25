
#include "diskbackedstorage.h"
#include <QDebug>

DiskBackedStorage::~DiskBackedStorage() {
    m_backingStore.unmap(m_storageMapping);

    // Delete cache file on complete destruction
    m_backingStore.remove();
}

void DiskBackedStorage::clearStorage() {
    m_storageOccupationBitmap.clear();
    m_storageOccupationBitmap.resize(m_backingFileBlockCountLimit, false);
    m_occupiedBlockCount = 0;
    m_backingFileBlockCount = 1;

    // Just assume this works
    m_backingStore.unmap(m_storageMapping);

    // Refresh mapping
    m_backingStore.resize(m_backingFileBlockCount * m_blockSize);
    m_storageMapping = m_backingStore.map(0, m_backingFileBlockCount * m_blockSize);

    Q_ASSERT(m_storageMapping);

    auto unallocBlk = reinterpret_cast<UnallocatedBlockHeader *>(m_storageMapping);
    unallocBlk->nextUnallocated = std::numeric_limits<uint64_t>::max();
    unallocBlk->unallocatedId = std::numeric_limits<uint64_t>::max();
    unallocBlk->numberOfBlocks = m_backingFileBlockCountLimit;

    m_freeListFirstBlockSeq = 0;
}

uint8_t *DiskBackedStorage::blockFromSequenceNumber(uint64_t blockSeqNumber) {
    return (m_storageMapping + blockSeqNumber * m_blockSize);
}

uint64_t DiskBackedStorage::sequenceNumberFromBlock(uint8_t *block) {
    return (block - m_storageMapping) / m_blockSize;
}

bool DiskBackedStorage::growToBlocksLong(uint64_t blockCount) {
    if (m_backingFileBlockCount >= blockCount) {
        return true;
    }

    if (!m_backingStore.resize(blockCount * m_blockSize)) {
        return false;
    }

    m_backingStore.unmap(m_storageMapping);
    m_storageMapping = m_backingStore.map(0, blockCount * m_blockSize);

    if (!m_storageMapping) {
        return false;
    }

    m_backingFileBlockCount = blockCount;
    return true;
}

Result<uint64_t, DiskBackedStorage::Error> DiskBackedStorage::allocateBlock() {
    // Sanity check
    Q_ASSERT((m_freeListFirstBlockSeq == std::numeric_limits<uint64_t>::max()) ==
             (m_occupiedBlockCount == m_backingFileBlockCountLimit));

    // No memory case
    if (m_freeListFirstBlockSeq == std::numeric_limits<uint64_t>::max()) {
        return Err(Error::DataStorageFull);
    }

    // The block allocated
    auto allocdBlkSeq = m_freeListFirstBlockSeq;
    auto allocdBlk = reinterpret_cast<UnallocatedBlockHeader *>(blockFromSequenceNumber(allocdBlkSeq));

    // Property sanity check
    Q_ASSERT(allocdBlk->numberOfBlocks <= m_backingFileBlockCountLimit);

    // Set in bitmap
    // Sanity check
    Q_ASSERT(!m_storageOccupationBitmap[m_freeListFirstBlockSeq]);
    m_storageOccupationBitmap[m_freeListFirstBlockSeq] = true;

    if (allocdBlk->numberOfBlocks == 1) {
        // If the current range of unallocated blocks are drained, use next range in free list next time
        m_freeListFirstBlockSeq = allocdBlk->nextUnallocated;
        growToBlocksLong(m_freeListFirstBlockSeq + 1);
        allocdBlk = reinterpret_cast<UnallocatedBlockHeader *>(blockFromSequenceNumber(allocdBlkSeq));
    } else {
        // Else, move to next
        m_freeListFirstBlockSeq++;

        // Update unalloc block metadata
        growToBlocksLong(m_freeListFirstBlockSeq + 1);
        auto freeBlock = reinterpret_cast<UnallocatedBlockHeader *>(blockFromSequenceNumber(m_freeListFirstBlockSeq));
        allocdBlk = reinterpret_cast<UnallocatedBlockHeader *>(blockFromSequenceNumber(allocdBlkSeq));
        *freeBlock = *allocdBlk;
        freeBlock->numberOfBlocks--;
    }

    // Now which block is allocated is determined
    auto ret = sequenceNumberFromBlock(reinterpret_cast<uint8_t *>(allocdBlk));

    // Increment occupied block count
    m_occupiedBlockCount++;

    // // Check if needs to grow file
    // if (m_backingFileBlockCount < ret + 1) {
    //     if (!growToBlocksLong(ret + 1)) {
    //         return Err(Error::CacheMemoryMappingFail);
    //     }
    // }

    // Return block sequence number
#ifdef BLOCK_ALLOC_DEBUG_MSG
    qDebug() << "Allocated block" << ret;
    printFreeList();
#endif
    return Ok(ret);
}

void DiskBackedStorage::freeBlock(uint64_t blockSeqNumber) {
    // Check if block is valid
    if (blockSeqNumber >= m_backingFileBlockCountLimit) {
        return;
    }

    // Check if block to be freed is actually allocated
    if (!m_storageOccupationBitmap[blockSeqNumber]) {
        return;
    }

    // Clear the block from bitmap
    m_storageOccupationBitmap[blockSeqNumber] = false;

    // Decrement occupied block count
    m_occupiedBlockCount--;

    // If block to be freed is before the first in free list...
    if (blockSeqNumber < m_freeListFirstBlockSeq) {
        // ... check also if the block immediately follows our known free list head
        auto newFreeListHead = reinterpret_cast<UnallocatedBlockHeader *>(blockFromSequenceNumber(blockSeqNumber));
        if (blockSeqNumber + 1 == m_freeListFirstBlockSeq) {
            // Merge the two
            auto oldFreeListHead =
                reinterpret_cast<UnallocatedBlockHeader *>(blockFromSequenceNumber(m_freeListFirstBlockSeq));

            *newFreeListHead = *oldFreeListHead;
            newFreeListHead->numberOfBlocks++;
        } else {
            // Link the new head to the old
            newFreeListHead->unallocatedId = std::numeric_limits<uint64_t>::max();
            newFreeListHead->numberOfBlocks = 1;
            newFreeListHead->nextUnallocated = m_freeListFirstBlockSeq;
        }

        // Replace known free list head
        m_freeListFirstBlockSeq = blockSeqNumber;
    } else {
        // Block is after the current known first free block
        // We want to find the first unallocated range after the block to free,
        // but preserving the range before that one

        // Make a good start
        uint64_t currentUnallocRange = m_freeListFirstBlockSeq;
        auto currentUnallocRangeBlock =
            reinterpret_cast<UnallocatedBlockHeader *>(blockFromSequenceNumber(m_freeListFirstBlockSeq));

        // Go down the list until we discover that next unallocated range is after freed block
        // Technically, nextUnallocated couldn't be equal to blockSeqNumber (when data is consistent)
        // And when we're at the end of the free list, the nextUnallocated would be maximum of uint64,
        // also satisfying this criteria, and also, we don't care if the nextUnallocated is valid,
        // we simply put that thing into the nextUnallocated field of the just freed block
        while (currentUnallocRangeBlock->nextUnallocated < blockSeqNumber) {
            currentUnallocRange = currentUnallocRangeBlock->nextUnallocated;
            currentUnallocRangeBlock =
                reinterpret_cast<UnallocatedBlockHeader *>(blockFromSequenceNumber(blockSeqNumber));
        }

        auto freedBlock = reinterpret_cast<UnallocatedBlockHeader *>(blockFromSequenceNumber(blockSeqNumber));
        bool besidesCurrent = blockSeqNumber == currentUnallocRange + currentUnallocRangeBlock->numberOfBlocks;
        bool besidesNext = blockSeqNumber + 1 == currentUnallocRangeBlock->nextUnallocated;

        if (besidesNext && besidesCurrent) {
            // The block is the very block separates next and current free range, merge them all
            auto nextRange = reinterpret_cast<UnallocatedBlockHeader *>(
                blockFromSequenceNumber(currentUnallocRangeBlock->nextUnallocated));
            currentUnallocRangeBlock->numberOfBlocks += (1 + nextRange->numberOfBlocks);
            currentUnallocRangeBlock->nextUnallocated = nextRange->nextUnallocated;
        } else if (besidesCurrent) {
            // If it immediately follows current free range, merge it with current
            currentUnallocRangeBlock->numberOfBlocks++;
        } else if (besidesNext) {
            // If it immediately followed by next free range, then merge it with next
            auto nextRange = reinterpret_cast<UnallocatedBlockHeader *>(
                blockFromSequenceNumber(currentUnallocRangeBlock->nextUnallocated));
            *freedBlock = *nextRange;
            freedBlock->numberOfBlocks++;
        } else {
            // Like said in the loop above, just put the block in the list
            freedBlock->nextUnallocated = currentUnallocRangeBlock->nextUnallocated;
            currentUnallocRangeBlock->nextUnallocated = blockSeqNumber;

            freedBlock->unallocatedId = std::numeric_limits<uint64_t>::max();
            freedBlock->numberOfBlocks = 1;
        }
    }
    // Done
#ifdef BLOCK_ALLOC_DEBUG_MSG
    qDebug() << "Freed block" << blockSeqNumber;
    printFreeList();
#endif
}

void DiskBackedStorage::printFreeList() {
    qDebug() << "-----------printFreeList----";
    auto freeBlkSeq = m_freeListFirstBlockSeq;
    uint64_t freeCount = 0;

    while (freeBlkSeq != std::numeric_limits<uint64_t>::max()) {
        auto freeRange = reinterpret_cast<UnallocatedBlockHeader *>(blockFromSequenceNumber(freeBlkSeq));
        qDebug() << "Block" << freeBlkSeq << ", count = " << freeRange->numberOfBlocks;
        freeCount += freeRange->numberOfBlocks;
        freeBlkSeq = freeRange->nextUnallocated;
    }

    qDebug() << "Free counted:" << freeCount << ", Used:" << m_occupiedBlockCount;
    Q_ASSERT(freeCount + m_occupiedBlockCount == m_backingFileBlockCountLimit);
}
