/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#pragma once


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>

#include "Vector.h"


struct Async
{
    struct Vector callVector;
    int numberOfCalls;
};


struct __AsyncCall
{
    void (*function)(uintptr_t);
    uintptr_t argument;
};


static inline bool Async_AddCall(struct Async *, void (*)(uintptr_t), uintptr_t);

void Async_Initialize(struct Async *);
void Async_Finalize(const struct Async *);
void Async_DispatchCalls(struct Async *);


static inline bool
Async_AddCall(struct Async *self, void (*function)(uintptr_t), uintptr_t argument)
{
    assert(self != NULL);
    assert(function != NULL);

    if (self->numberOfCalls == Vector_GetLength(&self->callVector)) {
        if (!Vector_SetLength(&self->callVector, self->numberOfCalls + 1, false)) {
            return false;
        }
    }

    struct __AsyncCall *calls = Vector_GetElements(&self->callVector);

    calls[self->numberOfCalls++] = (struct __AsyncCall) {
        .function = function,
        .argument = argument
    };

    return true;
}
