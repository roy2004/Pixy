#pragma once


#include <assert.h>

#include "Base.h"
#include "FIFO.h"


struct Async
{
    struct FIFO callFIFO;
};


struct __AsyncCall
{
    void (*function)(any_t);
    any_t argument;
};


static inline bool Async_AddCall(struct Async *, void (*)(any_t), any_t);

void Async_Initialize(struct Async *);
void Async_Finalize(const struct Async *);
void Async_DispatchCalls(struct Async *);


static inline bool
Async_AddCall(struct Async *self, void (*function)(any_t), any_t argument)
{
    assert(self != NULL);
    assert(function != NULL);

    struct __AsyncCall call = {
        .function = function,
        .argument = argument
    };

    return FIFO_Write(&self->callFIFO, &call, sizeof call);
}
