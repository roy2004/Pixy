#include "Timer.h"

#include <stddef.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include "Utility.h"
#include "Async.h"
#include "Logging.h"


static int TimeoutHeapNode_Compare(const struct HeapNode *, const struct HeapNode *);

static uint64_t GetTime(void);

static void xclock_gettime(clockid_t, struct timespec *);


void
Timer_Initialize(struct Timer *self)
{
    assert(self != NULL);
    Heap_Initialize(&self->timeoutHeap);
}


void
Timer_Finalize(struct Timer *self)
{
    assert(self != NULL);
    Heap_Finalize(&self->timeoutHeap);
}


int
Timer_SetTimeout(struct Timer *self, struct Timeout *timeout, int delay, uintptr_t data
                 , void (*callback)(uintptr_t))
{
    assert(self != NULL);
    assert(timeout != NULL);
    assert(callback != NULL);
    timeout->dueTime = delay >= 0 ? GetTime() + delay : UINT64_MAX;
    timeout->data = data;
    timeout->callback = callback;

    if (Heap_InsertNode(&self->timeoutHeap, &timeout->heapNode, TimeoutHeapNode_Compare) < 0) {
        return -1;
    }

    return 0;
}


void
Timer_ClearTimeout(struct Timer *self, const struct Timeout *timeout)
{
    assert(self != NULL);
    assert(timeout != NULL);
    Heap_RemoveNode(&self->timeoutHeap, &timeout->heapNode, TimeoutHeapNode_Compare);
}


int
Timer_CalculateWaitTime(const struct Timer *self)
{
    assert(self != NULL);
    struct HeapNode *timeoutHeapNode = Heap_GetTop(&self->timeoutHeap);

    if (timeoutHeapNode == NULL) {
        return -1;
    }

    struct Timeout *timeout = CONTAINER_OF(timeoutHeapNode, struct Timeout, heapNode);

    if (timeout->dueTime == UINT64_MAX) {
        return -1;
    }

    uint64_t now = GetTime();

    if (timeout->dueTime <= now) {
        return 0;
    }

    return timeout->dueTime - now;
}


int
Timer_Tick(struct Timer *self, struct Async *async)
{
    assert(self != NULL);
    assert(async != NULL);
    struct HeapNode *timeoutHeapNode = Heap_GetTop(&self->timeoutHeap);

    if (timeoutHeapNode == NULL) {
        return 0;
    }

    uint64_t now = GetTime();

    do {
        struct Timeout *timeout = CONTAINER_OF(timeoutHeapNode, struct Timeout, heapNode);

        if (timeout->dueTime > now) {
            break;
        }

        if (Async_AddCall(async, timeout->callback, timeout->data) < 0) {
            return -1;
        }

        Heap_RemoveNode(&self->timeoutHeap, timeoutHeapNode, TimeoutHeapNode_Compare);
        timeoutHeapNode = Heap_GetTop(&self->timeoutHeap);
    } while (timeoutHeapNode != NULL);

    return 0;
}


static int
TimeoutHeapNode_Compare(const struct HeapNode *self, const struct HeapNode *other)
{
    return COMPARE(CONTAINER_OF(self, const struct Timeout, heapNode)->dueTime
                   , CONTAINER_OF(other, const struct Timeout, heapNode)->dueTime);
}


static uint64_t
GetTime(void)
{
    struct timespec t;
    xclock_gettime(CLOCK_MONOTONIC_COARSE, &t);
    return t.tv_sec * 1000 + t.tv_nsec / 1000000;
}


static void
xclock_gettime(clockid_t clock_id, struct timespec *tp)
{
    if (clock_gettime(clock_id, tp) < 0) {
        LOG_FATAL_ERROR("`clock_gettime()` failed: %s", strerror(errno));
    }
}
