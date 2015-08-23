/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#pragma once


#include <stdint.h>
#include <stddef.h>
#include <assert.h>

#include "Vector.h"


struct Async
{
    struct Vector callVector;
    struct __AsyncCall *calls;
    int maxNumberOfCalls;
    int numberOfCalls;
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

    if (self->numberOfCalls == self->maxNumberOfCalls) {
        if ((self->maxNumberOfCalls == 0 ? Vector_SetLength(&self->callVector, 1024)
             : Vector_Expand(&self->callVector)) < 0) {
            return -1;
        }

        self->calls = Vector_GetElements(&self->callVector);
        self->maxNumberOfCalls = Vector_GetLength(&self->callVector);
    }

    self->calls[self->numberOfCalls++] = (struct __AsyncCall) {
        .function = function,
        .argument = argument
    };

    return 0;
}
