#include "Runtime.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "Scheduler.h"
#include "IOPoller.h"
#include "Timer.h"
#include "Async.h"


static void SleepCallback(any_t);
static void ReadCallback1(any_t);
static void ReadCallback2(any_t);
static void WriteCallback1(any_t);
static void WriteCallback2(any_t);
static void Accept4Callback1(any_t);
static void Accept4Callback2(any_t);
static void ConnectCallback1(any_t);
static void ConnectCallback2(any_t);


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


int
Pipe2(int *fds, int flags)
{
    return pipe2(fds, flags | O_NONBLOCK);
}


ssize_t
Read(int fd, void *buffer, size_t bufferSize, int timeout)
{
    ssize_t numberOfBytes;

    do {
        numberOfBytes = read(fd, buffer, bufferSize);
    } while (numberOfBytes < 0 && errno == EINTR);

    if (numberOfBytes >= 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
        return numberOfBytes;
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
    numberOfBytes = context.numberOfBytes;
    errno = context.errorNumber;
    return numberOfBytes;
}


ssize_t
Write(int fd, const void *data, size_t dataSize, int timeout)
{
    ssize_t numberOfBytes;

    do {
        numberOfBytes = write(fd, data, dataSize);
    } while (numberOfBytes < 0 && errno == EINTR);

    if (numberOfBytes >= 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
        return numberOfBytes;
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
    numberOfBytes = context.numberOfBytes;
    errno = context.errorNumber;
    return numberOfBytes;
}


ssize_t
ReadFully(int fd, void *buffer, size_t bufferSize, int timeout)
{
    size_t i = 0;

    do {
        ssize_t n = Read(fd, (unsigned char *)buffer + i, bufferSize - i, timeout);

        if (n < 0) {
            return -(i + 1);
        }

        i += n;
    } while (i < bufferSize);

    return bufferSize;
}


ssize_t
WriteFully(int fd, const void *data, size_t dataSize, int timeout)
{
    size_t i = 0;

    do {
        ssize_t n = Write(fd, (const unsigned char *)data + i, dataSize - i, timeout);

        if (n < 0) {
            return -(i + 1);
        }

        i += n;
    } while (i < dataSize);

    return dataSize;
}


int
Socket(int domain, int type, int protocol)
{
    return socket(domain, type | SOCK_NONBLOCK, protocol);
}


int
Accept4(int fd, struct sockaddr *address, socklen_t *addressLength, int flags, int timeout)
{
    int subFD;

    do {
        subFD = accept4(fd, address, addressLength, flags | O_NONBLOCK);
    } while (subFD < 0 && errno == EINTR);

    if (subFD >= 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
        return subFD;
    }

    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        struct sockaddr *address;
        socklen_t *addressLength;
        int flags;
        int subFD;
        int errorNumber;
    } context = {
        .fiber = Scheduler_GetCurrentFiber(&Scheduler),
        .fd = fd,
        .address = address,
        .addressLength = addressLength,
        .flags = flags
    };

    if (!IOPoller_SetWatch(&IOPoller, &context.ioWatch, fd, IOReadable, &context
                           , Accept4Callback1)) {
        return -1;
    }

    if (!Timer_SetTimeout(&Timer, &context.timeout, timeout, &context, Accept4Callback2)) {
        IOPoller_ClearWatch(&IOPoller, &context.ioWatch);
        return -1;
    }

    Scheduler_SuspendCurrentFiber(&Scheduler);
    subFD = context.subFD;
    errno = context.errorNumber;
    return subFD;
}


int
Connect(int fd, const struct sockaddr *address, socklen_t addressLength, int timeout)
{
    int result = connect(fd, address, addressLength);

    if (result >= 0 || (errno != EINTR && errno != EINPROGRESS)) {
        return result;
    }

    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        int result;
        int errorNumber;
    } context = {
        .fiber = Scheduler_GetCurrentFiber(&Scheduler),
        .fd = fd
    };

    if (!IOPoller_SetWatch(&IOPoller, &context.ioWatch, fd, IOWritable, &context
                           , ConnectCallback1)) {
        return -1;
    }

    if (!Timer_SetTimeout(&Timer, &context.timeout, timeout, &context, ConnectCallback2)) {
        IOPoller_ClearWatch(&IOPoller, &context.ioWatch);
        return -1;
    }

    Scheduler_SuspendCurrentFiber(&Scheduler);
    result = context.result;
    errno = context.errorNumber;
    return result;
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

    ssize_t numberOfBytes;

    do {
        numberOfBytes = read(context->fd, context->buffer, context->bufferSize);
    } while (numberOfBytes < 0 && errno == EINTR);

    if (numberOfBytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return;
    }

    context->numberOfBytes = numberOfBytes;
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
    context->errorNumber = EINTR;
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

    ssize_t numberOfBytes;

    do {
        numberOfBytes = write(context->fd, context->data, context->dataSize);
    } while (numberOfBytes < 0 && errno == EINTR);

    if (numberOfBytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return;
    }

    context->numberOfBytes = numberOfBytes;
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
    context->errorNumber = EINTR;
    IOPoller_ClearWatch(&IOPoller, &context->ioWatch);
    Scheduler_ResumeFiber(&Scheduler, context->fiber);
}


static void
Accept4Callback1(any_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        struct sockaddr *address;
        socklen_t *addressLength;
        int flags;
        int subFD;
        int errorNumber;
    } *context = (void *)argument;

    int subFD;

    do {
        subFD = accept4(context->fd, context->address, context->addressLength, context->flags
                                                                               | O_NONBLOCK);
    } while (subFD < 0 && errno == EINTR);

    if (subFD < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        return;
    }

    context->subFD = subFD;
    context->errorNumber = errno;
    IOPoller_ClearWatch(&IOPoller, &context->ioWatch);
    Timer_ClearTimeout(&Timer, &context->timeout);
    Scheduler_ResumeFiber(&Scheduler, context->fiber);
}


static void
Accept4Callback2(any_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        struct sockaddr *address;
        socklen_t *addressLength;
        int flags;
        int subFD;
        int errorNumber;
    } *context = (void *)argument;

    context->subFD = -1;
    context->errorNumber = EINTR;
    IOPoller_ClearWatch(&IOPoller, &context->ioWatch);
    Scheduler_ResumeFiber(&Scheduler, context->fiber);
}


static void
ConnectCallback1(any_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        int result;
        int errorNumber;
    } *context = (void *)argument;

    socklen_t optionLength = sizeof context->errorNumber;

    if (getsockopt(context->fd, SOL_SOCKET, SO_ERROR, &context->errorNumber, &optionLength) >= 0) {
        context->result = context->errorNumber == 0 ? 0 : -1;
    } else {
        context->result = -1;
        context->errorNumber = errno;
    }

    IOPoller_ClearWatch(&IOPoller, &context->ioWatch);
    Timer_ClearTimeout(&Timer, &context->timeout);
    Scheduler_ResumeFiber(&Scheduler, context->fiber);
}


static void
ConnectCallback2(any_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        int result;
        int errorNumber;
    } *context = (void *)argument;

    context->result = -1;
    context->errorNumber = EINTR;
    IOPoller_ClearWatch(&IOPoller, &context->ioWatch);
    Scheduler_ResumeFiber(&Scheduler, context->fiber);
}
