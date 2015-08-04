#pragma once


#include <assert.h>

#include "Base.h"


#if defined __i386__ || defined __x86_64__
#define ATOMIC_EXCHANGE(X, Y)       \
    assert(sizeof(X) == sizeof(Y)); \
    __asm__ __volatile__ ("lock xchg\t%1, %0" : "+m"(X), "+r"(Y))
    // X <-> Y

#define ATOMIC_ADD(X, Y)            \
    assert(sizeof(X) == sizeof(Y)); \
    __asm__ __volatile__ ("lock xadd\t%1, %0" : "+m"(X), "+r"(Y))
    // X <-> Y
    // X <- X + Y

#define ATOMIC_COMPARE_EXCHANGE(X, Y, Z)                      \
    assert(sizeof(X) == sizeof(Y) && sizeof(Y) == sizeof(Z)); \
    __asm__ __volatile__ ("lock cmpxchg\t%2, %0" : "+m"(X), "+a"(Y) : "r"(Z))
    // if X = Y
    //   X <- Z
    // else
    //   Y <- X
#else
#error architecture not supported
#endif
