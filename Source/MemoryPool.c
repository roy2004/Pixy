/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#include "MemoryPool.h"

#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>

#include "Utility.h"


#define MEMORY_CHUNK_SIZE ((size_t)65536)


struct MemoryChunk
{
    struct ListItem listItem;
    void **freeSlot;
    int numberOfFreeSlots;
};


static int MemoryPool_IncreaseChunks(struct MemoryPool *);

static struct MemoryChunk *LocateMemoryChunk(const void *);


static const size_t MemoryChunkPaddingSize = MEMORY_CHUNK_SIZE - sizeof(struct MemoryChunk);


void
MemoryPool_Initialize(struct MemoryPool *self, size_t blockSize)
{
    assert(self != NULL);
    assert(blockSize <= MemoryChunkPaddingSize);

    if (blockSize < sizeof(void *)) {
        blockSize = sizeof(void *);
    }

    self->blockSize = blockSize;
    self->numberOfSlotsPerChunk = MemoryChunkPaddingSize / blockSize;
    List_Initialize(&self->usableChunkListHead);
    List_Initialize(&self->unusableChunkListHead);
}


void
MemoryPool_Finalize(const struct MemoryPool *self)
{
    assert(self != NULL);
    struct ListItem *chunkListItem = List_GetBack(&self->usableChunkListHead);

    while (chunkListItem != &self->usableChunkListHead) {
        struct MemoryChunk *chunk = CONTAINER_OF(chunkListItem, struct MemoryChunk, listItem);
        chunkListItem = ListItem_GetPrev(chunkListItem);
        free(chunk);
    }

    chunkListItem = List_GetBack(&self->unusableChunkListHead);

    while (chunkListItem != &self->unusableChunkListHead) {
        struct MemoryChunk *chunk = CONTAINER_OF(chunkListItem, struct MemoryChunk, listItem);
        chunkListItem = ListItem_GetPrev(chunkListItem);
        free(chunk);
    }
}


void
MemoryPool_ShrinkToFit(struct MemoryPool *self)
{
    assert(self != NULL);
    struct ListItem *chunkListItem = List_GetFront(&self->usableChunkListHead);

    while (chunkListItem != &self->usableChunkListHead) {
        struct MemoryChunk *chunk = CONTAINER_OF(chunkListItem, struct MemoryChunk, listItem);

        if (chunk->numberOfFreeSlots < self->numberOfSlotsPerChunk) {
            break;
        }

        chunkListItem = ListItem_GetNext(chunkListItem);
        ListItem_Remove(&chunk->listItem);
        free(chunk);
    }
}


void *
MemoryPool_AllocateBlock(struct MemoryPool *self)
{
    assert(self != NULL);

    if (List_IsEmpty(&self->usableChunkListHead)) {
        if (MemoryPool_IncreaseChunks(self) < 0) {
            return NULL;
        }
    }

    struct MemoryChunk *chunk = CONTAINER_OF(List_GetBack(&self->usableChunkListHead)
                                             , struct MemoryChunk, listItem);
    void **slot = chunk->freeSlot;
    chunk->freeSlot = *slot;

    if (--chunk->numberOfFreeSlots == 0) {
        ListItem_Remove(&chunk->listItem);
        List_InsertBack(&self->unusableChunkListHead, &chunk->listItem);
    }

    return slot;
}


void
MemoryPool_FreeBlock(struct MemoryPool *self, void *block)
{
    assert(self != NULL);
    assert(block != NULL);
    struct MemoryChunk *chunk = LocateMemoryChunk(block);
    void **slot = block;
    *slot = chunk->freeSlot;
    chunk->freeSlot = slot;

    if (++chunk->numberOfFreeSlots == 1) {
        ListItem_Remove(&chunk->listItem);
        List_InsertBack(&self->usableChunkListHead, &chunk->listItem);
        return;
    }

    if (chunk->numberOfFreeSlots < self->numberOfSlotsPerChunk) {
        return;
    }

    struct ListItem *chunkListItemPrev = ListItem_GetPrev(&chunk->listItem);

    if (chunkListItemPrev == &self->usableChunkListHead) {
        return;
    }

    struct MemoryChunk *chunkPrev = CONTAINER_OF(chunkListItemPrev, struct MemoryChunk, listItem);

    if (chunkPrev->numberOfFreeSlots == self->numberOfSlotsPerChunk) {
        return;
    }

    ListItem_Remove(&chunk->listItem);
    List_InsertFront(&self->usableChunkListHead, &chunk->listItem);
}


static int
MemoryPool_IncreaseChunks(struct MemoryPool *self)
{
    struct MemoryChunk *chunk;
    int errorNumber = posix_memalign((void **)&chunk, MEMORY_CHUNK_SIZE, MEMORY_CHUNK_SIZE);

    if (errorNumber != 0) {
        assert(errorNumber != EINVAL);
        return -1;
    }

    void *block = (char *)chunk + MEMORY_CHUNK_SIZE - self->blockSize;
    void **slot = block;
    chunk->freeSlot = slot;

    for (;;) {
        block = (char *)block - self->blockSize;

        if (block < (void *)(chunk + 1)) {
            break;
        }

        *slot = block;
        slot = block;
    }

    *slot = NULL;
    chunk->numberOfFreeSlots = self->numberOfSlotsPerChunk;
    List_InsertFront(&self->usableChunkListHead, &chunk->listItem);
    return 0;
}


static struct MemoryChunk *
LocateMemoryChunk(const void *memoryBlock)
{
    return (struct MemoryChunk *)((uintptr_t)memoryBlock & ~(MEMORY_CHUNK_SIZE - 1));
}
