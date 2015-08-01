#include "Heap.h"

#include <stdlib.h>
#include <limits.h>


#define HEAP_SEGMENT_LENGTH 256u


static bool Heap_IncreaseSegments(struct Heap *);
static bool Heap_ExpandSegmentTable(struct Heap *);
static struct HeapNode **Heap_LocateSlot(const struct Heap *, int);
static void Heap_SiftNodeUp(struct Heap *, struct HeapNode **, int (*)(const struct HeapNode *
                                                                       , const struct HeapNode *));
static void Heap_SiftNodeDown(struct Heap *, struct HeapNode **
                              , int (*)(const struct HeapNode *, const struct HeapNode *));

static unsigned int NextPowerOfTwo(unsigned int);


void
Heap_Initialize(struct Heap *self)
{
    assert(self != NULL);
    self->segmentTable = NULL;
    self->segmentTableLength = 0;
    self->numberOfSegments = 0;
    self->numberOfSlots = 0;
    self->numberOfNodes = 0;
}


void
Heap_Finalize(const struct Heap *self)
{
    assert(self != NULL);

    if (self->numberOfSegments == 0) {
        return;
    }

    int i = self->numberOfSegments - 1;

    do {
        free(self->segmentTable[i]);
    } while (--i >= 0);

    free(self->segmentTable);
}


bool
Heap_ShrinkToFit(struct Heap *self)
{
    assert(self != NULL);
    int numberOfSegments = (self->numberOfNodes + HEAP_SEGMENT_LENGTH - 1) / HEAP_SEGMENT_LENGTH;

    if (self->numberOfSegments > numberOfSegments) {
        int i = self->numberOfSegments - 1;

        do {
            free(self->segmentTable[i]);
        } while (--i >= numberOfSegments);

        self->numberOfSegments = numberOfSegments;
        self->numberOfSlots = numberOfSegments * HEAP_SEGMENT_LENGTH;
    }

    int segmentTableLength = NextPowerOfTwo(numberOfSegments);

    if (self->segmentTableLength == segmentTableLength) {
        return true;
    }

    if (segmentTableLength == 0) {
        free(self->segmentTable);
        self->segmentTable = NULL;
        self->segmentTableLength = 0;
    } else {
        struct HeapNode ***segmentTable = realloc(self->segmentTable, segmentTableLength
                                                                      * sizeof *segmentTable);

        if (segmentTable == NULL) {
            return false;
        }

        self->segmentTable = segmentTable;
        self->segmentTableLength = segmentTableLength;
    }

    return true;
}


bool
Heap_InsertNode(struct Heap *self, struct HeapNode *node
                , int (*nodeComparer)(const struct HeapNode *, const struct HeapNode *))
{
    assert(self != NULL);
    assert(node != NULL);
    assert(nodeComparer != NULL);

    if (self->numberOfNodes == self->numberOfSlots) {
        if (!Heap_IncreaseSegments(self)) {
            return false;
        }
    }

    struct HeapNode **slot = Heap_LocateSlot(self, self->numberOfNodes);
    (*slot = node)->slotNumber = self->numberOfNodes++;
    Heap_SiftNodeUp(self, slot, nodeComparer);
    return true;
}


void
Heap_AdjustNode(struct Heap *self, struct HeapNode *node
                , int (*nodeComparer)(const struct HeapNode *, const struct HeapNode *))
{
    assert(self != NULL);
    assert(node != NULL);
    assert(nodeComparer != NULL);

    struct HeapNode **slot = Heap_LocateSlot(self, node->slotNumber);
    Heap_SiftNodeUp(self, slot, nodeComparer);

    if (*slot != node) {
        return;
    }

    Heap_SiftNodeDown(self, slot, nodeComparer);
}


void
Heap_RemoveNode(struct Heap *self, const struct HeapNode *node
                , int (*nodeComparer)(const struct HeapNode *, const struct HeapNode *))
{
    assert(self != NULL);
    assert(node != NULL);
    assert(nodeComparer != NULL);

    struct HeapNode **slot = Heap_LocateSlot(self, node->slotNumber);
    (*slot = *Heap_LocateSlot(self, --self->numberOfNodes))->slotNumber = node->slotNumber;
    int delta = nodeComparer(*slot, node);

    if (delta == 0) {
        return;
    }

    if (delta < 0) {
        Heap_SiftNodeUp(self, slot, nodeComparer);
    } else {
        Heap_SiftNodeDown(self, slot, nodeComparer);
    }
}


static bool
Heap_IncreaseSegments(struct Heap *self)
{
    if (self->numberOfSegments == self->segmentTableLength) {
        if (!Heap_ExpandSegmentTable(self)) {
            return false;
        }
    }

    struct HeapNode **segment = malloc(HEAP_SEGMENT_LENGTH * sizeof *segment);

    if (segment == NULL) {
        return false;
    }

    self->segmentTable[self->numberOfSegments++] = segment;
    self->numberOfSlots += HEAP_SEGMENT_LENGTH;
    return true;
}


static bool
Heap_ExpandSegmentTable(struct Heap *self)
{
    int segmentTableLength = self->segmentTableLength == 0 ? 1 : 2u * self->segmentTableLength;
    struct HeapNode ***segmentTable = realloc(self->segmentTable, segmentTableLength
                                                                  * sizeof *segmentTable);

    if (segmentTable == NULL) {
        return false;
    }

    self->segmentTable = segmentTable;
    self->segmentTableLength = segmentTableLength;
    return true;
}


static struct HeapNode **
Heap_LocateSlot(const struct Heap *self, int slotNumber)
{
    return &self->segmentTable[slotNumber / HEAP_SEGMENT_LENGTH][slotNumber % HEAP_SEGMENT_LENGTH];
}


static void
Heap_SiftNodeUp(struct Heap *self, struct HeapNode **slotX
                , int (*nodeComparer)(const struct HeapNode *, const struct HeapNode *))
{
    struct HeapNode *node = *slotX;
    int x = node->slotNumber;

    while (x >= 1) {
        int y = (x - 1) / 2u;
        struct HeapNode **slotY = Heap_LocateSlot(self, y);

        if (nodeComparer(node, *slotY) >= 0) {
            break;
        }

        (*slotX = *slotY)->slotNumber = x;
        slotX = slotY;
        x = y;
    }

    (*slotX = node)->slotNumber = x;
}


static void
Heap_SiftNodeDown(struct Heap *self, struct HeapNode **slotX
                  , int (*nodeComparer)(const struct HeapNode *, const struct HeapNode *))
{
    struct HeapNode *node = *slotX;
    int x = node->slotNumber;

    for (;;) {
        int y = 2u * x + 1;

        if (y >= self->numberOfNodes) {
            break;
        }

        struct HeapNode **slotY = Heap_LocateSlot(self, y);
        int z = y + 1;

        if (z < self->numberOfNodes) {
            struct HeapNode **slotZ = Heap_LocateSlot(self, z);

            if (nodeComparer(*slotZ, *slotY) < 0) {
                slotY = slotZ;
                y = z;
            }
        }

        if (nodeComparer(node, *slotY) <= 0) {
            break;
        }

        (*slotX = *slotY)->slotNumber = x;
        slotX = slotY;
        x = y;
    }

    (*slotX = node)->slotNumber = x;
}


static unsigned int
NextPowerOfTwo(unsigned int number)
{
    --number;
    int k;

    for (k = 1; k < (int)sizeof number * CHAR_BIT; k *= 2u) {
        number |= number >> k;
    }

    return ++number;
}
