/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#pragma once


#include <pthread.h>

#include <stdint.h>
#include <stdbool.h>

#include "List.h"
#include "IOPoller.h"


#define __NUMBER_OF_WORKERS 2


struct ThreadPool
{
    int fds[2];
    struct IOPoller *ioPoller;
    struct IOWatch ioWatch;
    struct ListItem workListHead;
    struct ListItem stopSignal;
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    pthread_t threads[__NUMBER_OF_WORKERS];
};


struct Work
{
    struct ListItem listItem;
    void (*function)(uintptr_t);
    uintptr_t argument;
    uintptr_t data;
    void (*callback)(uintptr_t);
};


bool ThreadPool_Initialize(struct ThreadPool *, struct IOPoller *);
void ThreadPool_Finalize(struct ThreadPool *);
void ThreadPool_Start(struct ThreadPool *);
void ThreadPool_Stop(struct ThreadPool *);
void ThreadPool_PostWork(struct ThreadPool *, struct Work *, void (*)(uintptr_t), uintptr_t
                         , uintptr_t, void (*)(uintptr_t));
