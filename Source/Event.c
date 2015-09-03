/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#include "Event.h"

#include <stddef.h>

#include "List.h"
#include "Scheduler.h"
#include "Utility.h"


struct EventWaiter
{
    struct ListItem listItem;
    struct Fiber *fiber;
};


struct Scheduler Scheduler;


void
Event_Initialize(struct Event *self)
{
    if (self == NULL) {
        return;
    }

    List_Initialize(LIST_HEAD(self->waiterList));
}


void
Event_WaitFor(struct Event *self)
{
    if (self == NULL) {
        return;
    }

    struct EventWaiter waiter;
    waiter.fiber = Scheduler_GetCurrentFiber(&Scheduler);
    List_InsertBack(LIST_HEAD(self->waiterList), &waiter.listItem);
    Scheduler_SuspendCurrentFiber(&Scheduler);
}


void
Event_Trigger(struct Event *self)
{
    if (self == NULL) {
        return;
    }

    if (List_IsEmpty(LIST_HEAD(self->waiterList))) {
        return;
    }

    struct EventWaiter *waiter = CONTAINER_OF(List_GetFront(LIST_HEAD(self->waiterList))
                                              , struct EventWaiter, listItem);
    ListItem_Remove(&waiter->listItem);
    Scheduler_ResumeFiber(&Scheduler, waiter->fiber);
}
