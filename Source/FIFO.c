#include "FIFO.h"

#include <string.h>
#include <stdlib.h>
#include <limits.h>


static size_t NextPowerOfTwo(size_t);


void
FIFO_Initialize(struct FIFO *self)
{
    assert(self != NULL);
    self->base = NULL;
    self->baseSize = 0;
    self->rIndex = 0;
    self->wIndex = 0;
}


void
FIFO_Finalize(const struct FIFO *self)
{
    assert(self != NULL);
    free(self->base);
}


int
FIFO_ShrinkToFit(struct FIFO *self)
{
    assert(self != NULL);

    if (self->rIndex >= 1) {
        memmove(self->base, self->base + self->rIndex, self->wIndex - self->rIndex);
        self->wIndex -= self->rIndex;
        self->rIndex = 0;
    }

    size_t baseSize = NextPowerOfTwo(self->wIndex);

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
FIFO_Write(struct FIFO *self, const void *data, size_t dataSize)
{
    assert(self != NULL);

    if (self->baseSize < self->wIndex + dataSize) {
        size_t baseSize = NextPowerOfTwo(self->wIndex + dataSize);
        char *base = realloc(self->base, baseSize);

        if (base == NULL) {
            return -1;
        }

        self->base = base;
        self->baseSize = baseSize;
    }

    if (data != NULL) {
        memcpy(self->base + self->wIndex, data, dataSize);
    }

    self->wIndex += dataSize;
    return 0;
}


size_t
FIFO_Read(struct FIFO *self, void *buffer, size_t bufferSize)
{
    assert(self != NULL);
    size_t dataSize = self->wIndex - self->rIndex;

    if (dataSize > bufferSize) {
        dataSize = bufferSize;
    }

    if (buffer != NULL) {
        memcpy(buffer, self->base + self->rIndex, dataSize);
    }

    self->rIndex += dataSize;

    if (self->rIndex >= self->wIndex - self->rIndex) {
        memcpy(self->base, self->base + self->rIndex, self->wIndex - self->rIndex);
        self->wIndex -= self->rIndex;
        self->rIndex = 0;
    }

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
