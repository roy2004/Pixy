#pragma once


#include <stdint.h>

#include "MemoryPool.h"
#include "RBTree.h"
#include "List.h"


enum IOCondition
{
    IOReadable,
    IOWritable
};


struct Async;


struct IOPoller
{
    int fd;
    struct MemoryPool eventMemoryPool;
    struct RBTree eventRBTree;
    struct ListItem dirtyEventListHead;
};


struct IOWatch
{
    struct ListItem listItem;
    enum IOCondition condition;
    uintptr_t data;
    void (*callback)(uintptr_t);
};


void IOPoller_Initialize(struct IOPoller *);
void IOPoller_Finalize(const struct IOPoller *);
int IOPoller_SetWatch(struct IOPoller *, struct IOWatch *, int, enum IOCondition, uintptr_t
                      , void (*)(uintptr_t));
void IOPoller_ClearWatch(struct IOPoller *, const struct IOWatch *);
void IOPoller_ClearWatches(struct IOPoller *, int);
int IOPoller_Tick(struct IOPoller *, int, struct Async *);
