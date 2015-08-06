#include "Async.h"


void
Async_Initialize(struct Async *self)
{
    assert(self != NULL);
    FIFO_Initialize(&self->callFIFO);
}


void
Async_Finalize(const struct Async *self)
{
    assert(self != NULL);
    FIFO_Finalize(&self->callFIFO);
}


void
Async_DispatchCalls(struct Async *self)
{
    assert(self != NULL);
    const struct __AsyncCall *calls = FIFO_GetData(&self->callFIFO);
    ptrdiff_t n = FIFO_GetDataSize(&self->callFIFO) / sizeof *calls;
    ptrdiff_t i;

    for (i = 0; i < n; ++i) {
        calls[i].function(calls[i].argument);
    }

    FIFO_Read(&self->callFIFO, NULL, -1);
}
