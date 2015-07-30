#include "Runtime.h"

#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "Scheduler.h"
#include "IOPoller.h"
#include "Timer.h"
#include "Async.h"


int Main(int, char **);

static void SleepCallback(any_t);
static void MainWrapper(any_t);
static void ReadCallback1(any_t);
static void ReadCallback2(any_t);
static void WriteCallback1(any_t);
static void WriteCallback2(any_t);


struct Scheduler Scheduler;
struct IOPoller IOPoller;
struct Timer Timer;


bool
Call(void (*coroutine)(any_t), any_t argument)
{
    if (coroutine == NULL) {
        errno = EINVAL;
        return false;
    }

    return Scheduler_CallCoroutine(&Scheduler, coroutine, argument);
}


void
Yield(void)
{
    Scheduler_YieldCurrentFiber(&Scheduler);
}


bool
Sleep(int duration)
{
    struct Timeout timeout;

    if (!Timer_SetTimeout(&Timer, &timeout, duration, (any_t)Scheduler_GetCurrentFiber(&Scheduler)
                          , SleepCallback)) {
        return false;
    }

    Scheduler_SuspendCurrentFiber(&Scheduler);
    return true;
}


ssize_t
Read(int fd, void *buffer, size_t bufferSize, int timeout)
{
    ssize_t nbytes;

    do {
        nbytes = read(fd, buffer, bufferSize);
    } while (nbytes < 0 && errno == EINTR);

    if (nbytes >= 0 || errno != EWOULDBLOCK) {
        return nbytes;
    }

    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        void *buffer;
        size_t bufferSize;
        ssize_t numberOfBytes;
        int errorNumber;
    } context = {
        .fiber = Scheduler_GetCurrentFiber(&Scheduler),
        .fd = fd,
        .buffer = buffer,
        .bufferSize = bufferSize
    };

    if (!IOPoller_SetWatch(&IOPoller, &context.ioWatch, fd, IOReadable, &context, ReadCallback1)) {
        return -1;
    }

    if (!Timer_SetTimeout(&Timer, &context.timeout, timeout, &context, ReadCallback2)) {
        IOPoller_ClearWatch(&IOPoller, &context.ioWatch);
        return -1;
    }

    Scheduler_SuspendCurrentFiber(&Scheduler);
    nbytes = context.numberOfBytes;
    errno = context.errorNumber;
    return nbytes;
}


ssize_t
Write(int fd, const void *data, size_t dataSize, int timeout)
{
    ssize_t nbytes;

    do {
        nbytes = write(fd, data, dataSize);
    } while (nbytes < 0 && errno == EINTR);

    if (nbytes >= 0 || errno != EWOULDBLOCK) {
        return nbytes;
    }

    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        const void *data;
        size_t dataSize;
        ssize_t numberOfBytes;
        int errorNumber;
    } context = {
        .fiber = Scheduler_GetCurrentFiber(&Scheduler),
        .fd = fd,
        .data = data,
        .dataSize = dataSize
    };

    if (!IOPoller_SetWatch(&IOPoller, &context.ioWatch, fd, IOWritable, &context, WriteCallback1)) {
        return -1;
    }

    if (!Timer_SetTimeout(&Timer, &context.timeout, timeout, &context, WriteCallback2)) {
        IOPoller_ClearWatch(&IOPoller, &context.ioWatch);
        return -1;
    }

    Scheduler_SuspendCurrentFiber(&Scheduler);
    nbytes = context.numberOfBytes;
    errno = context.errorNumber;
    return nbytes;
}


ssize_t
ReadFully(int fd, void *buffer, size_t bufferSize, int timeout)
{
    size_t i = 0;

    do {
        ssize_t nbytes = Read(fd, (unsigned char *)buffer + i, bufferSize - i, timeout);

        if (nbytes < 0) {
            return -(i + 1);
        }

        i += nbytes;
    } while (i < bufferSize);

    return bufferSize;
}


ssize_t
WriteFully(int fd, const void *data, size_t dataSize, int timeout)
{
    size_t i = 0;

    do {
        ssize_t nbytes = Write(fd, (unsigned char *)data + i, dataSize - i, timeout);

        if (nbytes < 0) {
            return -(i + 1);
        }

        i += nbytes;
    } while (i < dataSize);

    return dataSize;
}


int
main(int argc, char **argv)
{
    Scheduler_Initialize(&Scheduler);
    IOPoller_Initialize(&IOPoller);
    Timer_Initialize(&Timer);
    struct Async async;
    Async_Initialize(&async);

    struct {
        int argc;
        char **argv;
        int status;
    } context = {
        .argc = argc,
        .argv = argv
    };

    Scheduler_CallCoroutine(&Scheduler, MainWrapper, (any_t)&context);

    for (;;) {
        Scheduler_Tick(&Scheduler);

        if (Scheduler_GetFiberCount(&Scheduler) == 0) {
            break;
        }

        bool ok;

        do {
            ok = IOPoller_Tick(&IOPoller, Timer_CalculateWaitTime(&Timer), &async);
        } while (!ok && errno == EINTR);

        if (!ok) {
            assert(errno == ENOMEM);
            // TODO
        }

        Async_DispatchCalls(&async);

        if (!Timer_Tick(&Timer, &async)) {
            assert(errno == ENOMEM);
            // TODO
        }

        Async_DispatchCalls(&async);
    }

    Async_Finalize(&async);
    Scheduler_Finalize(&Scheduler);
    IOPoller_Finalize(&IOPoller);
    Timer_Finalize(&Timer);
    return context.status;
}


static void
MainWrapper(any_t argument)
{
    struct {
        int argc;
        char **argv;
        int status;
    } *context = (void *)argument;

    context->status = Main(context->argc, context->argv);
}


static void
SleepCallback(any_t argument)
{
    Scheduler_ResumeFiber(&Scheduler, (struct Fiber *)argument);
}


static void
ReadCallback1(any_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        void *buffer;
        size_t bufferSize;
        ssize_t numberOfBytes;
        int errorNumber;
    } *context = (void *)argument;

    ssize_t nbytes;

    do {
        nbytes = read(context->fd, context->buffer, context->bufferSize);
    } while (nbytes < 0 && errno == EINTR);

    if (nbytes < 0 && errno == EWOULDBLOCK) {
        return;
    }

    context->numberOfBytes = nbytes;
    context->errorNumber = errno;
    IOPoller_ClearWatch(&IOPoller, &context->ioWatch);
    Timer_ClearTimeout(&Timer, &context->timeout);
    Scheduler_ResumeFiber(&Scheduler, context->fiber);
}


static void
ReadCallback2(any_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        void *buffer;
        size_t bufferSize;
        ssize_t numberOfBytes;
        int errorNumber;
    } *context = (void *)argument;

    context->numberOfBytes = -1;
    context->errorNumber = ETIMEDOUT;
    IOPoller_ClearWatch(&IOPoller, &context->ioWatch);
    Scheduler_ResumeFiber(&Scheduler, context->fiber);
}


static void
WriteCallback1(any_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        const void *data;
        size_t dataSize;
        ssize_t numberOfBytes;
        int errorNumber;
    } *context = (void *)argument;

    ssize_t nbytes;

    do {
        nbytes = write(context->fd, context->data, context->dataSize);
    } while (nbytes < 0 && errno == EINTR);

    if (nbytes < 0 && errno == EWOULDBLOCK) {
        return;
    }

    context->numberOfBytes = nbytes;
    context->errorNumber = errno;
    IOPoller_ClearWatch(&IOPoller, &context->ioWatch);
    Timer_ClearTimeout(&Timer, &context->timeout);
    Scheduler_ResumeFiber(&Scheduler, context->fiber);
}


static void
WriteCallback2(any_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        const void *data;
        size_t dataSize;
        ssize_t numberOfBytes;
        int errorNumber;
    } *context = (void *)argument;

    context->numberOfBytes = -1;
    context->errorNumber = ETIMEDOUT;
    IOPoller_ClearWatch(&IOPoller, &context->ioWatch);
    Scheduler_ResumeFiber(&Scheduler, context->fiber);
}
