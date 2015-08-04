#pragma once


#include <stdbool.h>
#include <stddef.h>


#define LENGTH_OF(ARRAY) \
    (sizeof (ARRAY) / sizeof *(ARRAY))

#define CONTAINER_OF(ADDRESS, TYPE, FIELD) \
    ((TYPE *)((unsigned char *)(ADDRESS) - offsetof(TYPE, FIELD)))

#define COMPARE(A, B) \
    (((A) > (B)) - ((A) < (B)))

#define STRINGIZE(TEXT) \
    __STRINGIZE(TEXT)

#define __STRINGIZE(TEXT) \
    #TEXT


#if defined __i386__ || defined __x86_64__
typedef unsigned long any_t;
#else
#error architecture not supported
#endif
