/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#pragma once


#include <stdint.h>
#include <stdbool.h>

#include "Vector.h"
#include "MemoryPool.h"
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
    struct Vector eventVector;
    struct MemoryPool eventMemoryPool;
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
bool IOPoller_SetWatch(struct IOPoller *, struct IOWatch *, int, enum IOCondition, uintptr_t
                       , void (*)(uintptr_t));
void IOPoller_ClearWatch(struct IOPoller *, const struct IOWatch *);
void IOPoller_ClearWatches(struct IOPoller *, int);
bool IOPoller_Tick(struct IOPoller *, int, struct Async *);
