#pragma once


#include <stdint.h>

#include "Base.h"
#include "Heap.h"


struct Async;


struct Timer
{
    struct Heap timeoutHeap;
};


struct Timeout
{
    struct HeapNode heapNode;
    uint64_t dueTime;
    any_t data;
    void (*callback)(any_t);
};


void Timer_Initialize(struct Timer *);
void Timer_Finalize(struct Timer *);
bool Timer_SetTimeout(struct Timer *, struct Timeout *, int, any_t, void (*)(any_t));
void Timer_ClearTimeout(struct Timer *, const struct Timeout *);
int Timer_CalculateWaitTime(const struct Timer *);
bool Timer_Tick(struct Timer *, struct Async *);
