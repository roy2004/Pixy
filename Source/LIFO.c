#include "LIFO.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>


static size_t NextPowerOfTwo(size_t);


void
LIFO_Initialize(struct LIFO *self)
{
    assert(self != NULL);
    self->base = NULL;
    self->baseSize = 0;
    self->rwIndex = 0;
}


void LIFO_Finalize(const struct LIFO *self)
{
    assert(self != NULL);
    free(self->base);
}


int
LIFO_ShrinkToFit(struct LIFO *self)
{
    assert(self != NULL);
    size_t baseSize = NextPowerOfTwo(self->rwIndex);

    if (self->baseSize == baseSize) {
        return 0;
    }

    if (baseSize == 0) {
        free(self->base);
        self->base = NULL;
        self->baseSize = 0;
    } else {
        char *base = realloc(self->base, baseSize);

        if (base == NULL) {
            return -1;
        }

        self->base = base;
        self->baseSize = baseSize;
    }

    return 0;
}


int
LIFO_Write(struct LIFO *self, const void *data, size_t dataSize)
{
    assert(self != NULL);

    if (self->baseSize < self->rwIndex + dataSize) {
        size_t baseSize = NextPowerOfTwo(self->rwIndex + dataSize);
        char *base = realloc(self->base, baseSize);

        if (base == NULL) {
            return -1;
        }

        self->base = base;
        self->baseSize = baseSize;
    }

    if (data != NULL) {
        memcpy(self->base + self->rwIndex, data, dataSize);
    }

    self->rwIndex += dataSize;
    return 0;
}


size_t
LIFO_Read(struct LIFO *self, void *buffer, size_t bufferSize)
{
    assert(self != NULL);
    size_t dataSize = self->rwIndex;

    if (dataSize > bufferSize) {
        dataSize = bufferSize;
    }

    if (buffer != NULL) {
        memcpy(buffer, self->base + self->rwIndex - dataSize, dataSize);
    }

    self->rwIndex -= dataSize;
    return dataSize;
}


static size_t
NextPowerOfTwo(size_t number)
{
    --number;
    int k;

    for (k = 1; k < (int)sizeof number * CHAR_BIT; k *= 2u) {
        number |= number >> k;
    }

    ++number;
    return number;
}
