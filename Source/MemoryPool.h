#pragma once


#include "Base.h"
#include "List.h"


struct MemoryPool
{
    size_t blockSize;
    int numberOfSlotsPerChunk;
    struct ListItem usableChunkListHead;
    struct ListItem unusableChunkListHead;
};


void MemoryPool_Initialize(struct MemoryPool *, size_t);
void MemoryPool_Finalize(const struct MemoryPool *);
void MemoryPool_ShrinkToFit(struct MemoryPool *);
void *MemoryPool_AllocateBlock(struct MemoryPool *);
void MemoryPool_FreeBlock(struct MemoryPool *, void *);
