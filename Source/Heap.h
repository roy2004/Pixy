/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#pragma once


#include <stddef.h>
#include <stdbool.h>
#include <assert.h>

#include "Vector.h"


struct Heap
{
    struct Vector segmentVector;
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

    struct HeapNode ***segments = Vector_GetElements(&self->segmentVector);
    return segments[0][0];
}
