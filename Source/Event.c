/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#include "Event.h"

#include <stddef.h>

#include "Scheduler.h"


struct __EventWaiter
{
    struct __EventWaiter *prev;
    struct Fiber *fiber;
};


struct Scheduler Scheduler;


void
Event_Initialize(struct Event *self)
{
    if (self == NULL) {
        return;
    }

    self->lastWaiter = NULL;
}


void
Event_Trigger(struct Event *self)
{
    if (self == NULL) {
        return;
    }

    struct __EventWaiter *waiter = self->lastWaiter;

    if (waiter == NULL) {
        return;
    }

    do {
        Scheduler_ResumeFiber(&Scheduler, waiter->fiber);
        waiter = waiter->prev;
    } while (waiter != NULL);

    self->lastWaiter = NULL;
}


void
Event_WaitFor(struct Event *self)
{
    if (self == NULL) {
        return;
    }

    struct __EventWaiter waiter = {
        .fiber = Scheduler_GetCurrentFiber(&Scheduler)
    };

    waiter.prev = self->lastWaiter;
    self->lastWaiter = &waiter;
    Scheduler_SuspendCurrentFiber(&Scheduler);
}
