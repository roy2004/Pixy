#pragma once


#include <assert.h>

#include "Base.h"


struct Heap
{
    struct HeapNode ***segmentTable;
    int segmentTableLength;
    int numberOfSegments;
    int numberOfSlots;
    int numberOfNodes;
};


struct HeapNode
{
    int slotNumber;
};


static inline struct HeapNode *Heap_GetTop(const struct Heap *);

void Heap_Initialize(struct Heap *);
void Heap_Finalize(const struct Heap *);
bool Heap_ShrinkToFit(struct Heap *);
bool Heap_InsertNode(struct Heap *, struct HeapNode *, int (*)(const struct HeapNode *
                                                               , const struct HeapNode *));
void Heap_AdjustNode(struct Heap *, struct HeapNode *, int (*)(const struct HeapNode *
                                                               , const struct HeapNode *));
void Heap_RemoveNode(struct Heap *, const struct HeapNode *, int (*)(const struct HeapNode *
                                                                     , const struct HeapNode *));


static inline struct HeapNode *
Heap_GetTop(const struct Heap *self)
{
    assert(self != NULL);

    if (self->numberOfNodes == 0) {
        return NULL;
    }

    return self->segmentTable[0][0];
}
