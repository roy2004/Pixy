/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#pragma once


#include "Utility.h"


#if defined __i386__ || defined __x86_64__
#define ATOMIC_EXCHANGE(x, y)              \
    STATIC_ASSERT(sizeof(x) == sizeof(y)); \
    __asm__ __volatile__ ("lock xchg\t%1, %0" : "+m"(x), "+r"(y))
    // x <-> y

#define ATOMIC_ADD(x, y)                   \
    STATIC_ASSERT(sizeof(x) == sizeof(y)); \
    __asm__ __volatile__ ("lock xadd\t%1, %0" : "+m"(x), "+r"(y))
    // x <-> y
    // x <- x + y

#define ATOMIC_COMPARE_EXCHANGE(x, y, z)                             \
    STATIC_ASSERT(sizeof(x) == sizeof(y) && sizeof(y) == sizeof(z)); \
    __asm__ __volatile__ ("lock cmpxchg\t%2, %0" : "+m"(x), "+a"(y) : "r"(z))
    // if x = y
    //   x <- z
    // else
    //   y <- x
#else
#error architecture not supported
#endif
