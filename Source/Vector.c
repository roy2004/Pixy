/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#include "Vector.h"

#include <stdlib.h>
#include <string.h>
#include <limits.h>


static size_t NextPowerOfTwo(size_t);


void
Vector_Initialize(struct Vector *self, size_t elementSize)
{
    assert(self != NULL);
    self->elementSize = elementSize;
    self->elements = NULL;
    self->size = 0;
    self->length = 0;
}


void
Vector_Finalize(const struct Vector *self)
{
    assert(self != NULL);
    free(self->elements);
}


bool
Vector_SetLength(struct Vector *self, ptrdiff_t length, bool zeroNewElements)
{
    assert(self != NULL);
    assert(length >= 0);
    size_t size = NextPowerOfTwo(length * self->elementSize);

    if (self->size == size) {
        return true;
    }

    if (size == 0) {
        free(self->elements);
        self->elements = NULL;
        self->size = 0;
        self->length = 0;
        return true;
    }

    void *elements = realloc(self->elements, size);

    if (elements == NULL) {
        return false;
    }

    if (zeroNewElements && size > self->size) {
        memset((char *)elements + self->size, 0, size - self->size);
    }

    self->elements = elements;
    self->size = size;
    self->length = size / self->elementSize;
    return true;
}


bool
Vector_Expand(struct Vector *self, bool zeroNewElements)
{
    assert(self != NULL && self->size != 0);
    size_t size = 2 * self->size;
    void *elements = realloc(self->elements, size);

    if (elements == NULL) {
        return false;
    }

    if (zeroNewElements) {
        memset((char *)elements + self->size, 0, size - self->size);
    }

    self->elements = elements;
    self->size = size;
    self->length = size / self->elementSize;
    return true;
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
