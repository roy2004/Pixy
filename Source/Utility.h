/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#pragma once


#define LENGTH_OF(ARRAY) \
    (sizeof (ARRAY) / sizeof *(ARRAY))

#define OFFSET_OF(TYPE, FIELD) \
    ((char *)&((TYPE *)0)->FIELD - (char *)0)

#define CONTAINER_OF(ADDRESS, TYPE, FIELD) \
    ((TYPE *)((char *)(ADDRESS) - OFFSET_OF(TYPE, FIELD)))

#define COMPARE(A, B) \
    (((A) > (B)) - ((A) < (B)))

#define STRINGIZE(TEXT) \
    __STRINGIZE(TEXT)

#define __STRINGIZE(TEXT) \
    #TEXT
