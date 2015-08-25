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
    __STRINGIZE(text)

#define __STRINGIZE(text) \
    #text
