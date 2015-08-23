/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#pragma once


#include <stddef.h>
#include <assert.h>


struct Vector
{
    void *elements;
    size_t size;
    size_t elementSize;
};


static inline void *Vector_GetElements(const struct Vector *);
static inline ptrdiff_t Vector_GetLength(const struct Vector *);

void Vector_Initialize(struct Vector *, size_t);
void Vector_Finalize(const struct Vector *);
int Vector_SetLength(struct Vector *, ptrdiff_t);
int Vector_Expand(struct Vector *);


static inline void *
Vector_GetElements(const struct Vector *self)
{
    assert(self != NULL);
    return self->elements;
}


static inline ptrdiff_t
Vector_GetLength(const struct Vector *self)
{
    assert(self != NULL);
    return self->size / self->elementSize;
}
