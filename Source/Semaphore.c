/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#include "Semaphore.h"

#include <stddef.h>
#include <errno.h>

#include "List.h"
#include "Scheduler.h"
#include "Utility.h"


struct SemaphoreWaiter
{
    struct ListItem listItem;
    struct Fiber *fiber;
};


struct Scheduler Scheduler;


bool
Semaphore_Initialize(struct Semaphore *self, int value, int minValue, int maxValue)
{
    if (self == NULL) {
        return true;
    }

    if (minValue > value || maxValue < value) {
        errno = EINVAL;
        return false;
    }

    self->value = value;
    self->minValue = minValue;
    self->maxValue = maxValue;
    List_Initialize(LIST_HEAD(self->downWaiterList));
    List_Initialize(LIST_HEAD(self->upWaiterList));
    return true;
}


void
Semaphore_Down(struct Semaphore *self)
{
    if (self == NULL) {
        return;
    }

    if (self->value == self->minValue) {
        struct SemaphoreWaiter waiter;
        waiter.fiber = Scheduler_GetCurrentFiber(&Scheduler);
        List_InsertBack(LIST_HEAD(self->downWaiterList), &waiter.listItem);
        Scheduler_SuspendCurrentFiber(&Scheduler);
        ListItem_Remove(&waiter.listItem);

        if (--self->value > self->minValue && !List_IsEmpty(LIST_HEAD(self->downWaiterList))) {
            Scheduler_ResumeFiber(&Scheduler
                                  , CONTAINER_OF(List_GetFront(LIST_HEAD(self->downWaiterList))
                                                 , struct SemaphoreWaiter, listItem)->fiber);
        }
    } else {
        if (--self->value == self->minValue && !List_IsEmpty(LIST_HEAD(self->downWaiterList))) {
            Scheduler_UnresumeFiber(&Scheduler
                                    , CONTAINER_OF(List_GetFront(LIST_HEAD(self->downWaiterList))
                                                   , struct SemaphoreWaiter, listItem)->fiber);
        }
    }

    if (self->value == self->maxValue - 1 && !List_IsEmpty(LIST_HEAD(self->upWaiterList))) {
        Scheduler_ResumeFiber(&Scheduler
                              , CONTAINER_OF(List_GetFront(LIST_HEAD(self->upWaiterList))
                                             , struct SemaphoreWaiter, listItem)->fiber);
    }
}


void
Semaphore_Up(struct Semaphore *self)
{
    if (self == NULL) {
        return;
    }

    if (self->value == self->maxValue) {
        struct SemaphoreWaiter waiter;
        waiter.fiber = Scheduler_GetCurrentFiber(&Scheduler);
        List_InsertBack(LIST_HEAD(self->upWaiterList), &waiter.listItem);
        Scheduler_SuspendCurrentFiber(&Scheduler);
        ListItem_Remove(&waiter.listItem);

        if (++self->value < self->maxValue && !List_IsEmpty(LIST_HEAD(self->upWaiterList))) {
            Scheduler_ResumeFiber(&Scheduler
                                  , CONTAINER_OF(List_GetFront(LIST_HEAD(self->upWaiterList))
                                                 , struct SemaphoreWaiter, listItem)->fiber);
        }
    } else {
        if (++self->value == self->maxValue && !List_IsEmpty(LIST_HEAD(self->upWaiterList))) {
            Scheduler_UnresumeFiber(&Scheduler
                                    , CONTAINER_OF(List_GetFront(LIST_HEAD(self->upWaiterList))
                                                   , struct SemaphoreWaiter, listItem)->fiber);
        }
    }

    if (self->value == self->minValue + 1 && !List_IsEmpty(LIST_HEAD(self->downWaiterList))) {
        Scheduler_ResumeFiber(&Scheduler
                              , CONTAINER_OF(List_GetFront(LIST_HEAD(self->downWaiterList))
                                             , struct SemaphoreWaiter, listItem)->fiber);
    }
}
