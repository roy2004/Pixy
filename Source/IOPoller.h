#pragma once


#include "Base.h"
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
    any_t data;
    void (*callback)(any_t);
};


void IOPoller_Initialize(struct IOPoller *);
void IOPoller_Finalize(const struct IOPoller *);
bool IOPoller_SetWatch(struct IOPoller *, struct IOWatch *, int, enum IOCondition, any_t
                       , void (*)(any_t));
void IOPoller_ClearWatch(struct IOPoller *, const struct IOWatch *);
void IOPoller_ClearWatches(struct IOPoller *, int);
bool IOPoller_Tick(struct IOPoller *, int, struct Async *);
