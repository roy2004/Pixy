#pragma once


#include <stddef.h>
#include <assert.h>


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
int Heap_ShrinkToFit(struct Heap *);
int Heap_InsertNode(struct Heap *, struct HeapNode *, int (*)(const struct HeapNode *
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
