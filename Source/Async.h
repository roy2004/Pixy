/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#pragma once


#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "FIFO.h"


struct Async
{
    struct FIFO callFIFO;
};


struct __AsyncCall
{
    void (*function)(uintptr_t);
    uintptr_t argument;
};


static inline int Async_AddCall(struct Async *, void (*)(uintptr_t), uintptr_t);

void Async_Initialize(struct Async *);
void Async_Finalize(const struct Async *);
void Async_DispatchCalls(struct Async *);


static inline int
Async_AddCall(struct Async *self, void (*function)(uintptr_t), uintptr_t argument)
{
    assert(self != NULL);
    assert(function != NULL);

    struct __AsyncCall call = {
        .function = function,
        .argument = argument
    };

    return FIFO_Write(&self->callFIFO, &call, sizeof call);
}
