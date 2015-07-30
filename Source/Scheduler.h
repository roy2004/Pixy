#pragma once


#include <assert.h>
#include <setjmp.h>

#include "Base.h"
#include "List.h"


struct Fiber;


struct Scheduler
{
    jmp_buf *context;
    struct Fiber *runningFiber;
    struct ListItem readyFiberListHead;
    struct ListItem deadFiberListHead;
    unsigned int fiberCount;
};


static inline struct Fiber *Scheduler_GetCurrentFiber(const struct Scheduler *);
static inline unsigned int Scheduler_GetFiberCount(const struct Scheduler *);

void Scheduler_Initialize(struct Scheduler *);
void Scheduler_Finalize(const struct Scheduler *);
bool Scheduler_CallCoroutine(struct Scheduler *, void (*)(any_t), any_t);
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


static inline unsigned int
Scheduler_GetFiberCount(const struct Scheduler *self)
{
    assert(self != NULL);
    return self->fiberCount;
}
