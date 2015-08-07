#pragma once


#include <stddef.h>
#include <assert.h>


struct FIFO
{
    char *base;
    size_t baseSize;
    ptrdiff_t rIndex;
    ptrdiff_t wIndex;
};


static inline const void *FIFO_GetData(const struct FIFO *);
static inline size_t FIFO_GetDataSize(const struct FIFO *);
static inline void *FIFO_GetBuffer(const struct FIFO *);
static inline size_t FIFO_GetBufferSize(const struct FIFO *);

void FIFO_Initialize(struct FIFO *);
void FIFO_Finalize(const struct FIFO *);
int FIFO_ShrinkToFit(struct FIFO *);
int FIFO_Write(struct FIFO *, const void *, size_t);
size_t FIFO_Read(struct FIFO *, void *, size_t);


static inline const void *
FIFO_GetData(const struct FIFO *self)
{
    assert(self != NULL);
    return self->base + self->rIndex;
}


static inline size_t
FIFO_GetDataSize(const struct FIFO *self)
{
    assert(self != NULL);
    return self->wIndex - self->rIndex;
}


static inline void *
FIFO_GetBuffer(const struct FIFO *self)
{
    assert(self != NULL);
    return self->base + self->wIndex;
}


static inline size_t
FIFO_GetBufferSize(const struct FIFO *self)
{
    assert(self != NULL);
    return self->baseSize - self->wIndex;
}
