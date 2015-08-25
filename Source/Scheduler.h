/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#pragma once


#include <setjmp.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "List.h"
#include "Noreturn.h"


struct Fiber;


struct Scheduler
{
    jmp_buf *context;
    struct Fiber *activeFiber;
    struct ListItem readyFiberListHead;
    struct ListItem deadFiberListHead;
    int fiberCount;
};


static inline struct Fiber *Scheduler_GetCurrentFiber(const struct Scheduler *);
static inline int Scheduler_GetFiberCount(const struct Scheduler *);

void Scheduler_Initialize(struct Scheduler *);
void Scheduler_Finalize(const struct Scheduler *);
bool Scheduler_AddFiber(struct Scheduler *, void (*)(uintptr_t), uintptr_t);
bool Scheduler_AddAndRunFiber(struct Scheduler *, void (*)(uintptr_t), uintptr_t);
void Scheduler_YieldCurrentFiber(struct Scheduler *);
void Scheduler_SuspendCurrentFiber(struct Scheduler *);
void Scheduler_ResumeFiber(struct Scheduler *, struct Fiber *);
NORETURN void Scheduler_ExitCurrentFiber(struct Scheduler *);
void Scheduler_Tick(struct Scheduler *);


static inline struct Fiber *
Scheduler_GetCurrentFiber(const struct Scheduler *self)
{
    assert(self != NULL && self->activeFiber != NULL);
    return self->activeFiber;
}


static inline int
Scheduler_GetFiberCount(const struct Scheduler *self)
{
    assert(self != NULL);
    return self->fiberCount;
}
