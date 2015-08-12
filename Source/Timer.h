/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#pragma once


#include <stdint.h>

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
    uintptr_t data;
    void (*callback)(uintptr_t);
};


void Timer_Initialize(struct Timer *);
void Timer_Finalize(struct Timer *);
int Timer_SetTimeout(struct Timer *, struct Timeout *, int, uintptr_t, void (*)(uintptr_t));
void Timer_ClearTimeout(struct Timer *, const struct Timeout *);
int Timer_CalculateWaitTime(const struct Timer *);
int Timer_Tick(struct Timer *, struct Async *);
