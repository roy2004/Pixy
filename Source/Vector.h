/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#pragma once


#include <stddef.h>
#include <stdbool.h>
#include <assert.h>


struct Vector
{
    size_t elementSize;
    void *elements;
    size_t size;
    ptrdiff_t length;
};


static inline void *Vector_GetElements(const struct Vector *);
static inline ptrdiff_t Vector_GetLength(const struct Vector *);

void Vector_Initialize(struct Vector *, size_t);
void Vector_Finalize(const struct Vector *);
bool Vector_SetLength(struct Vector *, ptrdiff_t, bool);
bool Vector_Expand(struct Vector *, bool);


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
    return self->length;
}
