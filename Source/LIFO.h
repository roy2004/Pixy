#pragma once


#include <assert.h>

#include "Base.h"


struct LIFO
{
    unsigned char *buffer;
    size_t bufferSize;
    ptrdiff_t i;
};


static inline void *LIFO_GetData(const struct LIFO *);
static inline size_t LIFO_GetDataSize(const struct LIFO *);
static inline void LIFO_ClearData(struct LIFO *);

void LIFO_Initialize(struct LIFO *);
void LIFO_Finalize(const struct LIFO *);
bool LIFO_ShrinkToFit(struct LIFO *);
bool LIFO_Write(struct LIFO *, const void *, size_t);
size_t LIFO_Read(struct LIFO *, void *, size_t);


static inline void *
LIFO_GetData(const struct LIFO *self)
{
    assert(self != NULL);
    return self->buffer;
}


static inline size_t
LIFO_GetDataSize(const struct LIFO *self)
{
    assert(self != NULL);
    return self->i;
}


static inline void
LIFO_ClearData(struct LIFO *self)
{
    assert(self != NULL);
    self->i = 0;
}
