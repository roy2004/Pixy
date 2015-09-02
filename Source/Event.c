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

    struct ListItem *waiterListItem = List_GetFront(LIST_HEAD(self->waiterList));

    if (waiterListItem == LIST_HEAD(self->waiterList)) {
        return;
    }

    do {
        Scheduler_ResumeFiber(&Scheduler, CONTAINER_OF(waiterListItem, struct EventWaiter, listItem)
                                          ->fiber);
        waiterListItem = ListItem_GetNext(waiterListItem);
    } while (waiterListItem != LIST_HEAD(self->waiterList));

    List_Initialize(LIST_HEAD(self->waiterList));
}
