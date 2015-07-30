#include "FIFO.h"

#include <stdlib.h>
#include <string.h>


static unsigned long NextPowerOfTwo(unsigned long);


void
FIFO_Initialize(struct FIFO *self)
{
    assert(self != NULL);
    self->buffer = NULL;
    self->bufferSize = 0;
    self->i = 0;
    self->j = 0;
}


void
FIFO_Finalize(const struct FIFO *self)
{
    assert(self != NULL);
    free(self->buffer);
}


bool
FIFO_ShrinkToFit(struct FIFO *self)
{
    assert(self != NULL);

    if (self->i >= 1) {
        memmove(self->buffer, self->buffer + self->i, self->j - self->i);
        self->j -= self->i;
        self->i = 0;
    }

    size_t bufferSize = NextPowerOfTwo(self->j);

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
FIFO_Write(struct FIFO *self, const void *data, size_t dataSize)
{
    assert(self != NULL);
    assert(data != NULL || dataSize == 0);

    if (self->bufferSize < self->j + dataSize) {
        size_t bufferSize = NextPowerOfTwo(self->j + dataSize);
        unsigned char *buffer = realloc(self->buffer, bufferSize);

        if (buffer == NULL) {
            return false;
        }

        self->buffer = buffer;
        self->bufferSize = bufferSize;
    }

    memcpy(self->buffer + self->j, data, dataSize);
    self->j += dataSize;
    return true;
}


size_t
FIFO_Read(struct FIFO *self, void *buffer, size_t bufferSize)
{
    assert(self != NULL);
    assert(buffer != NULL || bufferSize == 0);
    size_t dataSize = self->j - self->i;

    if (dataSize > bufferSize) {
        dataSize = bufferSize;
    }

    memcpy(buffer, self->buffer + self->i, dataSize);
    self->i += dataSize;

    if (self->i >= self->j - self->i) {
        memcpy(self->buffer, self->buffer + self->i, self->j - self->i);
        self->j -= self->i;
        self->i = 0;
    }

    return dataSize;
}


static unsigned long
NextPowerOfTwo(unsigned long number)
{
    --number;
#if defined __i386__
    number |= number >> 1;
    number |= number >> 2;
    number |= number >> 4;
    number |= number >> 8;
    number |= number >> 16;
#elif defined __x86_64__
    number |= number >> 1;
    number |= number >> 2;
    number |= number >> 4;
    number |= number >> 8;
    number |= number >> 16;
    number |= number >> 32;
#else
#   error architecture not supported
#endif
    ++number;
    return number;
}
