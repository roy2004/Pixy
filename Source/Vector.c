#include "Vector.h"

#include <stdlib.h>
#include <limits.h>


static size_t NextPowerOfTwo(size_t);


void
Vector_Initialize(struct Vector *self, size_t elementSize)
{
    assert(self != NULL);
    self->elements = NULL;
    self->size = 0;
    self->elementSize = elementSize;
}


void
Vector_Finalize(const struct Vector *self)
{
    assert(self != NULL);
    free(self->elements);
}


int
Vector_SetLength(struct Vector *self, ptrdiff_t length)
{
    assert(self != NULL);
    assert(length >= 0);
    size_t size = NextPowerOfTwo(length * self->elementSize);

    if (self->size == size) {
        return 0;
    }

    if (size == 0) {
        free(self->elements);
        self->elements = NULL;
        self->size = 0;
        return 0;
    }

    void *elements = realloc(self->elements, size);

    if (elements == NULL) {
        return -1;
    }

    self->elements = elements;
    self->size = size;
    return 0;
}


int
Vector_Expand(struct Vector *self)
{
    assert(self != NULL && self->size != 0);
    size_t size = 2 * self->size;
    void *elements = realloc(self->elements, size);

    if (elements == NULL) {
        return -1;
    }

    self->elements = elements;
    self->size = size;
    return 0;
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
