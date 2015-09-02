/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#pragma once


#include <stdbool.h>


#if defined __cplusplus
extern "C" {
#endif

struct Semaphore
{
    int value;
    int minValue;
    int maxValue;
    void *downWaiterList[2];
    void *upWaiterList[2];
};


bool Semaphore_Initialize(struct Semaphore *self, int value, int minValue, int maxValue);
void Semaphore_Down(struct Semaphore *self);
void Semaphore_Up(struct Semaphore *self);

#if defined __cplusplus
} // extern "C"
#endif
