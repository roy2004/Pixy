#pragma once


#include <assert.h>

#include "Base.h"


struct FIFO
{
    unsigned char *buffer;
    size_t bufferSize;
    ptrdiff_t i;
    ptrdiff_t j;
};


static inline void *FIFO_GetData(const struct FIFO *);
static inline size_t FIFO_GetDataSize(const struct FIFO *);
static inline void FIFO_ClearData(struct FIFO *);

void FIFO_Initialize(struct FIFO *);
void FIFO_Finalize(const struct FIFO *);
bool FIFO_ShrinkToFit(struct FIFO *);
bool FIFO_Write(struct FIFO *, const void *, size_t);
size_t FIFO_Read(struct FIFO *, void *, size_t);


static inline void *
FIFO_GetData(const struct FIFO *self)
{
    assert(self != NULL);
    return self->buffer + self->i;
}


static inline size_t
FIFO_GetDataSize(const struct FIFO *self)
{
    assert(self != NULL);
    return self->j - self->i;
}


static inline void
FIFO_ClearData(struct FIFO *self)
{
    assert(self != NULL);
    self->i = 0;
    self->j = 0;
}
