/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#pragma once


#include <setjmp.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "List.h"


struct Fiber;


struct Scheduler
{
    jmp_buf *context;
    struct Fiber *runningFiber;
    struct ListItem readyFiberListHead;
    struct ListItem deadFiberListHead;
    int fiberCount;
};


static inline struct Fiber *Scheduler_GetCurrentFiber(const struct Scheduler *);
static inline int Scheduler_GetFiberCount(const struct Scheduler *);

void Scheduler_Initialize(struct Scheduler *);
void Scheduler_Finalize(const struct Scheduler *);
int Scheduler_CallCoroutine(struct Scheduler *, void (*)(uintptr_t), uintptr_t);
void Scheduler_YieldCurrentFiber(struct Scheduler *);
void Scheduler_SuspendCurrentFiber(struct Scheduler *);
void Scheduler_ResumeFiber(struct Scheduler *, struct Fiber *);
void Scheduler_ExitCurrentFiber(struct Scheduler *);
void Scheduler_Tick(struct Scheduler *);


static inline struct Fiber *
Scheduler_GetCurrentFiber(const struct Scheduler *self)
{
    assert(self != NULL && self->runningFiber != NULL);
    return self->runningFiber;
}


static inline int
Scheduler_GetFiberCount(const struct Scheduler *self)
{
    assert(self != NULL);
    return self->fiberCount;
}
