#pragma once


#include <assert.h>

#include "Base.h"


struct LIFO
{
    unsigned char *base;
    size_t baseSize;
    ptrdiff_t rwIndex;
};


static inline const void *LIFO_GetData(const struct LIFO *);
static inline size_t LIFO_GetDataSize(const struct LIFO *);
static inline void *LIFO_GetBuffer(const struct LIFO *);
static inline size_t LIFO_GetBufferSize(const struct LIFO *);

void LIFO_Initialize(struct LIFO *);
void LIFO_Finalize(const struct LIFO *);
bool LIFO_ShrinkToFit(struct LIFO *);
bool LIFO_Write(struct LIFO *, const void *, size_t);
size_t LIFO_Read(struct LIFO *, void *, size_t);


static inline void *
LIFO_GetData(const struct LIFO *self)
{
    assert(self != NULL);
    return self->base;
}


static inline size_t
LIFO_GetDataSize(const struct LIFO *self)
{
    assert(self != NULL);
    return self->rwIndex;
}


static inline void *
LIFO_GetBuffer(const struct LIFO *self)
{
    assert(self != NULL);
    return self->base + self->rwIndex;
}


static inline size_t
LIFO_GetBufferSize(const struct LIFO *self)
{
    assert(self != NULL);
    return self->baseSize - self->rwIndex;
}
