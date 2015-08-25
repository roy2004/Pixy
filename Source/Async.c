/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#include "Async.h"


void
Async_Initialize(struct Async *self)
{
    assert(self != NULL);
    Vector_Initialize(&self->callVector, sizeof(struct __AsyncCall));
    self->numberOfCalls = 0;
}


void
Async_Finalize(const struct Async *self)
{
    assert(self != NULL);
    Vector_Finalize(&self->callVector);
}


void
Async_DispatchCalls(struct Async *self)
{
    assert(self != NULL);
    struct __AsyncCall *calls = Vector_GetElements(&self->callVector);
    int n = self->numberOfCalls;
    int i;

    for (i = 0; i < n; ++i) {
        calls[i].function(calls[i].argument);
    }

    self->numberOfCalls = 0;
}
