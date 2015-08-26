/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#pragma once


#define LENGTH_OF(array) \
    (sizeof (array) / sizeof *(array))

#define OFFSET_OF(type, field) \
    ((char *)&((type *)0)->field - (char *)0)

#define CONTAINER_OF(address, type, field) \
    ((type *)((char *)(address) - OFFSET_OF(type, field)))

#define COMPARE(a, b) \
    (((a) > (b)) - ((a) < (b)))

#define STRINGIZE(text) \
    __STRINGIZE2(text)

#define __STRINGIZE2(text) \
    #text

#define STATIC_ASSERT(expression)          \
    do {                                   \
        char dummy[(expression) ? 1 : -1]; \
        (void)dummy;                       \
    } while (0)
