#include "LIFO.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>


static size_t NextPowerOfTwo(size_t);


void
LIFO_Initialize(struct LIFO *self)
{
    assert(self != NULL);
    self->buffer = NULL;
    self->bufferSize = 0;
    self->i = 0;
}


void LIFO_Finalize(const struct LIFO *self)
{
    assert(self != NULL);
    free(self->buffer);
}


bool
LIFO_ShrinkToFit(struct LIFO *self)
{
    assert(self != NULL);
    size_t bufferSize = NextPowerOfTwo(self->i);

    if (self->bufferSize == bufferSize) {
        return true;
    }

    if (bufferSize == 0) {
        free(self->buffer);
        self->buffer = NULL;
        self->bufferSize = 0;
    } else {
        unsigned char *buffer = realloc(self->buffer, bufferSize);

        if (buffer == NULL) {
            return false;
        }

        self->buffer = buffer;
        self->bufferSize = bufferSize;
    }

    return true;
}


bool
LIFO_Write(struct LIFO *self, const void *data, size_t dataSize)
{
    assert(self != NULL);
    assert(data != NULL || dataSize == 0);

    if (self->bufferSize < self->i + dataSize) {
        size_t bufferSize = NextPowerOfTwo(self->i + dataSize);
        unsigned char *buffer = realloc(self->buffer, bufferSize);

        if (buffer == NULL) {
            return false;
        }

        self->buffer = buffer;
        self->bufferSize = bufferSize;
    }

    memcpy(self->buffer + self->i, data, dataSize);
    self->i += dataSize;
    return true;
}


size_t
LIFO_Read(struct LIFO *self, void *buffer, size_t bufferSize)
{
    assert(self != NULL);
    assert(buffer != NULL || bufferSize == 0);
    size_t dataSize = self->i;

    if (dataSize > bufferSize) {
        dataSize = bufferSize;
    }

    memcpy(buffer, self->buffer + self->i - dataSize, dataSize);
    self->i -= dataSize;
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

    return ++number;
}
