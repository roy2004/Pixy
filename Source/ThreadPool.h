#pragma once


#include <pthread.h>

#include "Base.h"
#include "List.h"
#include "IOPoller.h"


#define __NUMBER_OF_WORKERS 4


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
    void (*function)(any_t);
    any_t argument;
    any_t data;
    void (*callback)(any_t);
};


bool ThreadPool_Initialize(struct ThreadPool *, struct IOPoller *);
void ThreadPool_Finalize(struct ThreadPool *);
void ThreadPool_Start(struct ThreadPool *);
void ThreadPool_Stop(struct ThreadPool *);
void ThreadPool_PostWork(struct ThreadPool *, struct Work *, void (*)(any_t), any_t, any_t
                         , void (*)(any_t));
