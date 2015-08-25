/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#include "ThreadPool.h"

#include <fcntl.h>
#include <unistd.h>

#include <stddef.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include "Utility.h"
#include "Logging.h"


static void ThreadPool_InsertWorkListItem(struct ThreadPool *, struct ListItem *);
static void ThreadPool_Worker(struct ThreadPool *);

static void WorkerCallback(uintptr_t);
static void *ThreadStart(void *);

static void xpipe(int [2]);
static void xclose(int);
static int xfcntl(int, int, int);
static size_t xread(int, void *, size_t);
static size_t xwrite(int, const void *, size_t);
static void xpthread_mutex_init(pthread_mutex_t *, const pthread_mutexattr_t *);
static void xpthread_mutex_destroy(pthread_mutex_t *);
static void xpthread_mutex_lock(pthread_mutex_t *);
static void xpthread_mutex_unlock(pthread_mutex_t *);
static void xpthread_cond_init(pthread_cond_t *, const pthread_condattr_t *);
static void xpthread_cond_destroy(pthread_cond_t *);
static void xpthread_cond_wait(pthread_cond_t *, pthread_mutex_t *);
static void xpthread_cond_signal(pthread_cond_t *);
static void xpthread_create(pthread_t *, const pthread_attr_t *, void *(*)(void *), void *);
static void xpthread_join(pthread_t, void **);


bool
ThreadPool_Initialize(struct ThreadPool *self, struct IOPoller *ioPoller)
{
    assert(self != NULL);
    assert(ioPoller != NULL);
    int fds[2];
    xpipe(fds);
    xfcntl(fds[0], F_SETFL, xfcntl(fds[0], F_GETFL, 0) | O_NONBLOCK);
    xfcntl(fds[1], F_SETPIPE_SZ, 8192 * sizeof(struct Work *));

    if (!IOPoller_SetWatch(ioPoller, &self->ioWatch, fds[0], IOReadable, (uintptr_t)self
                           , WorkerCallback)) {
        xclose(fds[0]);
        xclose(fds[1]);
        return false;
    }

    self->fds[0] = fds[0];
    self->fds[1] = fds[1];
    self->ioPoller = ioPoller;
    List_Initialize(&self->workListHead);
    return true;
}


void
ThreadPool_Finalize(struct ThreadPool *self)
{
    assert(self != NULL);
    IOPoller_ClearWatches(self->ioPoller, self->fds[0]);
    xclose(self->fds[0]);
    xclose(self->fds[1]);
}


void
ThreadPool_Start(struct ThreadPool *self)
{
    assert(self != NULL);
    xpthread_mutex_init(&self->mutex, NULL);
    xpthread_cond_init(&self->condition, NULL);
    int i;

    for (i = 0; i < __NUMBER_OF_WORKERS; ++i) {
        xpthread_create(&self->threads[i], NULL, ThreadStart, self);
    }
}


void
ThreadPool_Stop(struct ThreadPool *self)
{
    assert(self != NULL);
    ThreadPool_InsertWorkListItem(self, &self->stopSignal);
    int i;

    for (i = 0; i < __NUMBER_OF_WORKERS; ++i) {
        xpthread_join(self->threads[i], NULL);
    }

    ListItem_Remove(&self->stopSignal);
    xpthread_mutex_destroy(&self->mutex);
    xpthread_cond_destroy(&self->condition);
}


void
ThreadPool_PostWork(struct ThreadPool *self, struct Work *work, void (*function)(uintptr_t)
                    , uintptr_t argument, uintptr_t data, void (*callback)(uintptr_t))
{
    assert(self != NULL);
    assert(work != NULL);
    assert(function != NULL);
    assert(callback != NULL);
    work->function = function;
    work->argument = argument;
    work->data = data;
    work->callback = callback;
    ThreadPool_InsertWorkListItem(self, &work->listItem);
}


static void
ThreadPool_InsertWorkListItem(struct ThreadPool *self, struct ListItem *workListItem)
{
    xpthread_mutex_lock(&self->mutex);
    List_InsertBack(&self->workListHead, workListItem);

    if (ListItem_GetPrev(workListItem) == ListItem_GetNext(workListItem)) {
        xpthread_cond_signal(&self->condition);
    }

    xpthread_mutex_unlock(&self->mutex);
}


static void
ThreadPool_Worker(struct ThreadPool *self)
{
    for (;;) {
        xpthread_mutex_lock(&self->mutex);

        while (List_IsEmpty(&self->workListHead)) {
            xpthread_cond_wait(&self->condition, &self->mutex);
        }

        struct ListItem *workListItem = List_GetFront(&self->workListHead);

        if (workListItem == &self->stopSignal) {
#if __NUMBER_OF_WORKERS >= 2
            xpthread_cond_signal(&self->condition);
#endif
            xpthread_mutex_unlock(&self->mutex);
            break;
        }

        ListItem_Remove(workListItem);

        if (!List_IsEmpty(&self->workListHead)) {
#if __NUMBER_OF_WORKERS >= 2
            xpthread_cond_signal(&self->condition);
#endif
        }

        xpthread_mutex_unlock(&self->mutex);
        struct Work *work = CONTAINER_OF(workListItem, struct Work, listItem);
        work->function(work->argument);
        xwrite(self->fds[1], &work, sizeof work);
    }
}


static void
WorkerCallback(uintptr_t argument)
{
    struct ThreadPool *threadPool = (struct ThreadPool *)argument;
    struct Work *works[8192];
    int n = xread(threadPool->fds[0], works, sizeof works) / sizeof *works;
    int i;

    for (i = 0; i < n; ++i) {
        works[i]->callback(works[i]->data);
    }
}


static void *
ThreadStart(void *argument)
{
    ThreadPool_Worker(argument);
    return NULL;
}


static void
xpipe(int fildes[2])
{
    if (pipe(fildes) < 0) {
        LOG_FATAL_ERROR("`pipe()` failed: %s", strerror(errno));
    }
}


static void
xclose(int fd)
{
    int res;

    do {
        res = close(fd);
    } while (res < 0 && errno == EINTR);

    if (res < 0) {
        LOG_ERROR("`close()` failed: %s", strerror(errno));
    }
}


static int
xfcntl(int fd, int cmd, int arg)
{
    int res;

    do {
        res = fcntl(fd, cmd, arg);
    } while (res < 0 && errno == EINTR);

    if (res < 0) {
        LOG_FATAL_ERROR("`fcntl()` failed: %s", strerror(errno));
    }

    return res;
}


static size_t
xread(int fd, void *buf, size_t count)
{
    ssize_t nbytes;

    do {
        nbytes = read(fd, buf, count);
    } while (nbytes < 0 && errno == EINTR);

    if (nbytes < 0) {
        LOG_FATAL_ERROR("`read()` failed: %s", strerror(errno));
    }

    return nbytes;
}


static size_t
xwrite(int fd, const void *buf, size_t count)
{
    ssize_t nbytes;

    do {
        nbytes = write(fd, buf, count);
    } while (nbytes < 0 && errno == EINTR);

    if (nbytes < 0) {
        LOG_FATAL_ERROR("`write()` failed: %s", strerror(errno));
    }

    return nbytes;
}


static void
xpthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr)
{
    if (pthread_mutex_init(mutex, attr) < 0) {
        LOG_FATAL_ERROR("`pthread_mutex_init()` failed: %s", strerror(errno));
    }
}


static void
xpthread_mutex_destroy(pthread_mutex_t *mutex)
{
    if (pthread_mutex_destroy(mutex) < 0) {
        LOG_FATAL_ERROR("`pthread_mutex_destroy()` failed: %s", strerror(errno));
    }
}


static void
xpthread_mutex_lock(pthread_mutex_t *mutex)
{
    if (pthread_mutex_lock(mutex) < 0) {
        LOG_FATAL_ERROR("`pthread_mutex_lock()` failed: %s", strerror(errno));
    }
}


static void
xpthread_mutex_unlock(pthread_mutex_t *mutex)
{
    if (pthread_mutex_unlock(mutex) < 0) {
        LOG_FATAL_ERROR("`pthread_mutex_unlock()` failed: %s", strerror(errno));
    }
}


static void
xpthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr)
{
    if (pthread_cond_init(cond, attr) < 0) {
        LOG_FATAL_ERROR("`pthread_cond_init()` failed: %s", strerror(errno));
    }
}


static void
xpthread_cond_destroy(pthread_cond_t *cond)
{
    if (pthread_cond_destroy(cond) < 0) {
        LOG_FATAL_ERROR("`pthread_cond_destroy()` failed: %s", strerror(errno));
    }
}


static void
xpthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    if (pthread_cond_wait(cond, mutex) < 0) {
        LOG_FATAL_ERROR("`pthread_cond_wait()` failed: %s", strerror(errno));
    }
}


static void
xpthread_cond_signal(pthread_cond_t *cond)
{
    if (pthread_cond_signal(cond) < 0) {
        LOG_FATAL_ERROR("`pthread_cond_signal()` failed: %s", strerror(errno));
    }
}


static void
xpthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void *)
                , void *arg)
{
    if (pthread_create(thread, attr, start_routine, arg) < 0) {
        LOG_FATAL_ERROR("`pthread_create()` failed: %s", strerror(errno));
    }
}


static void
xpthread_join(pthread_t thread, void **value_ptr)
{
    if (pthread_join(thread, value_ptr) < 0) {
        LOG_FATAL_ERROR("`pthread_join()` failed: %s", strerror(errno));
    }
}
