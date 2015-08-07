#include "Runtime.h"

#include <fcntl.h>
#include <unistd.h>
#include <netdb.h>

#include <stddef.h>
#include <errno.h>

#include "Scheduler.h"
#include "IOPoller.h"
#include "Timer.h"
#include "ThreadPool.h"


static void SleepCallback(uintptr_t);
static void ReadCallback1(uintptr_t);
static void ReadCallback2(uintptr_t);
static void WriteCallback1(uintptr_t);
static void WriteCallback2(uintptr_t);
static void Accept4Callback1(uintptr_t);
static void Accept4Callback2(uintptr_t);
static void ConnectCallback1(uintptr_t);
static void ConnectCallback2(uintptr_t);
static void RecvCallback1(uintptr_t);
static void RecvCallback2(uintptr_t);
static void SendCallback1(uintptr_t);
static void SendCallback2(uintptr_t);
static void RecvFromCallback1(uintptr_t);
static void RecvFromCallback2(uintptr_t);
static void SendToCallback1(uintptr_t);
static void SendToCallback2(uintptr_t);
static void DoWork(void (*)(uintptr_t), uintptr_t);
static void DoWorkCallback(uintptr_t);
static void GetAddrInfoWrapper(uintptr_t);


struct Scheduler Scheduler;
struct IOPoller IOPoller;
struct Timer Timer;
struct ThreadPool ThreadPool;


int
Call(void (*coroutine)(uintptr_t), uintptr_t argument)
{
    if (coroutine == NULL) {
        errno = EINVAL;
        return -1;
    }

    return Scheduler_CallCoroutine(&Scheduler, coroutine, argument);
}


void
Yield(void)
{
    Scheduler_YieldCurrentFiber(&Scheduler);
}


void
Exit(void)
{
    Scheduler_ExitCurrentFiber(&Scheduler);
}


int
Sleep(int duration)
{
    struct Timeout timeout;

    if (Timer_SetTimeout(&Timer, &timeout, duration
                         , (uintptr_t)Scheduler_GetCurrentFiber(&Scheduler), SleepCallback) < 0) {
        return -1;
    }

    Scheduler_SuspendCurrentFiber(&Scheduler);
    return 0;
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

    if (IOPoller_SetWatch(&IOPoller, &context.ioWatch, fd, IOReadable, (uintptr_t)&context
                          , ReadCallback1) < 0) {
        return -1;
    }

    if (Timer_SetTimeout(&Timer, &context.timeout, timeout, (uintptr_t)&context, ReadCallback2)
        < 0) {
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

    if (IOPoller_SetWatch(&IOPoller, &context.ioWatch, fd, IOWritable, (uintptr_t)&context
                          , WriteCallback1) < 0) {
        return -1;
    }

    if (Timer_SetTimeout(&Timer, &context.timeout, timeout, (uintptr_t)&context, WriteCallback2)
        < 0) {
        IOPoller_ClearWatch(&IOPoller, &context.ioWatch);
        return -1;
    }

    Scheduler_SuspendCurrentFiber(&Scheduler);
    numberOfBytes = context.numberOfBytes;
    errno = context.errorNumber;
    return numberOfBytes;
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

    if (IOPoller_SetWatch(&IOPoller, &context.ioWatch, fd, IOReadable, (uintptr_t)&context
                          , Accept4Callback1) < 0) {
        return -1;
    }

    if (Timer_SetTimeout(&Timer, &context.timeout, timeout, (uintptr_t)&context, Accept4Callback2)
        < 0) {
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

    if (IOPoller_SetWatch(&IOPoller, &context.ioWatch, fd, IOWritable, (uintptr_t)&context
                          , ConnectCallback1) < 0) {
        return -1;
    }

    if (Timer_SetTimeout(&Timer, &context.timeout, timeout, (uintptr_t)&context, ConnectCallback2)
        < 0) {
        IOPoller_ClearWatch(&IOPoller, &context.ioWatch);
        return -1;
    }

    Scheduler_SuspendCurrentFiber(&Scheduler);
    result = context.result;
    errno = context.errorNumber;
    return result;
}


ssize_t
Recv(int fd, void *buffer, size_t bufferSize, int flags, int timeout)
{
    ssize_t numberOfBytes;

    do {
        numberOfBytes = recv(fd, buffer, bufferSize, flags);
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
        int flags;
        ssize_t numberOfBytes;
        int errorNumber;
    } context = {
        .fiber = Scheduler_GetCurrentFiber(&Scheduler),
        .fd = fd,
        .buffer = buffer,
        .bufferSize = bufferSize,
        .flags = flags
    };

    if (IOPoller_SetWatch(&IOPoller, &context.ioWatch, fd, IOReadable, (uintptr_t)&context
                          , RecvCallback1) < 0) {
        return -1;
    }

    if (Timer_SetTimeout(&Timer, &context.timeout, timeout, (uintptr_t)&context, RecvCallback2)
        < 0) {
        IOPoller_ClearWatch(&IOPoller, &context.ioWatch);
        return -1;
    }

    Scheduler_SuspendCurrentFiber(&Scheduler);
    numberOfBytes = context.numberOfBytes;
    errno = context.errorNumber;
    return numberOfBytes;
}


ssize_t
Send(int fd, const void *data, size_t dataSize, int flags, int timeout)
{
    ssize_t numberOfBytes;

    do {
        numberOfBytes = send(fd, data, dataSize, flags);
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
        int flags;
        ssize_t numberOfBytes;
        int errorNumber;
    } context = {
        .fiber = Scheduler_GetCurrentFiber(&Scheduler),
        .fd = fd,
        .data = data,
        .dataSize = dataSize,
        .flags = flags
    };

    if (IOPoller_SetWatch(&IOPoller, &context.ioWatch, fd, IOWritable, (uintptr_t)&context
                          , SendCallback1) < 0) {
        return -1;
    }

    if (Timer_SetTimeout(&Timer, &context.timeout, timeout, (uintptr_t)&context, SendCallback2)
        < 0) {
        IOPoller_ClearWatch(&IOPoller, &context.ioWatch);
        return -1;
    }

    Scheduler_SuspendCurrentFiber(&Scheduler);
    numberOfBytes = context.numberOfBytes;
    errno = context.errorNumber;
    return numberOfBytes;
}


ssize_t
RecvFrom(int fd, void *buffer, size_t bufferSize, int flags, struct sockaddr *address
         , socklen_t *addressLength, int timeout)
{
    ssize_t numberOfBytes;

    do {
        numberOfBytes = recvfrom(fd, buffer, bufferSize, flags, address, addressLength);
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
        int flags;
        struct sockaddr *address;
        socklen_t *addressLength;
        ssize_t numberOfBytes;
        int errorNumber;
    } context = {
        .fiber = Scheduler_GetCurrentFiber(&Scheduler),
        .fd = fd,
        .buffer = buffer,
        .bufferSize = bufferSize,
        .flags = flags,
        .address = address,
        .addressLength = addressLength
    };

    if (IOPoller_SetWatch(&IOPoller, &context.ioWatch, fd, IOReadable, (uintptr_t)&context
                          , RecvFromCallback1) < 0) {
        return -1;
    }

    if (Timer_SetTimeout(&Timer, &context.timeout, timeout, (uintptr_t)&context, RecvFromCallback2)
        < 0) {
        IOPoller_ClearWatch(&IOPoller, &context.ioWatch);
        return -1;
    }

    Scheduler_SuspendCurrentFiber(&Scheduler);
    numberOfBytes = context.numberOfBytes;
    errno = context.errorNumber;
    return numberOfBytes;
}


ssize_t
SendTo(int fd, const void *data, size_t dataSize, int flags, const struct sockaddr *address
       , socklen_t addressLength, int timeout)
{
    ssize_t numberOfBytes;

    do {
        numberOfBytes = sendto(fd, data, dataSize, flags, address, addressLength);
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
        int flags;
        const struct sockaddr *address;
        socklen_t addressLength;
        ssize_t numberOfBytes;
        int errorNumber;
    } context = {
        .fiber = Scheduler_GetCurrentFiber(&Scheduler),
        .fd = fd,
        .data = data,
        .dataSize = dataSize,
        .flags = flags
    };

    if (IOPoller_SetWatch(&IOPoller, &context.ioWatch, fd, IOWritable, (uintptr_t)&context
                          , SendToCallback1) < 0) {
        return -1;
    }

    if (Timer_SetTimeout(&Timer, &context.timeout, timeout, (uintptr_t)&context, SendToCallback2)
        < 0) {
        IOPoller_ClearWatch(&IOPoller, &context.ioWatch);
        return -1;
    }

    Scheduler_SuspendCurrentFiber(&Scheduler);
    numberOfBytes = context.numberOfBytes;
    errno = context.errorNumber;
    return numberOfBytes;
}


int
Close(int fd)
{
    IOPoller_ClearWatches(&IOPoller, fd);
    int result;

    do {
        result = close(fd);
    } while (result < 0 && errno == EINTR);

    return result;
}


int
GetAddrInfo(const char *hostName, const char *serviceName, const struct addrinfo *hints
            , struct addrinfo **result)
{
    struct {
        const char *hostName;
        const char *serviceName;
        const struct addrinfo *hints;
        struct addrinfo **result;
        int errorCode;
    } context = {
        .hostName = hostName,
        .serviceName = serviceName,
        .hints = hints,
        .result = result
    };

    DoWork(GetAddrInfoWrapper, (uintptr_t)&context);
    return context.errorCode;
}


static void
SleepCallback(uintptr_t argument)
{
    Scheduler_ResumeFiber(&Scheduler, (struct Fiber *)argument);
}


static void
ReadCallback1(uintptr_t argument)
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
ReadCallback2(uintptr_t argument)
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
WriteCallback1(uintptr_t argument)
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
WriteCallback2(uintptr_t argument)
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
Accept4Callback1(uintptr_t argument)
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
Accept4Callback2(uintptr_t argument)
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
ConnectCallback1(uintptr_t argument)
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
ConnectCallback2(uintptr_t argument)
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


static void
RecvCallback1(uintptr_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        void *buffer;
        size_t bufferSize;
        int flags;
        ssize_t numberOfBytes;
        int errorNumber;
    } *context = (void *)argument;

    ssize_t numberOfBytes;

    do {
        numberOfBytes = recv(context->fd, context->buffer, context->bufferSize, context->flags);
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
RecvCallback2(uintptr_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        void *buffer;
        size_t bufferSize;
        int flags;
        ssize_t numberOfBytes;
        int errorNumber;
    } *context = (void *)argument;

    context->numberOfBytes = -1;
    context->errorNumber = EINTR;
    IOPoller_ClearWatch(&IOPoller, &context->ioWatch);
    Scheduler_ResumeFiber(&Scheduler, context->fiber);
}


static void
SendCallback1(uintptr_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        const void *data;
        size_t dataSize;
        int flags;
        ssize_t numberOfBytes;
        int errorNumber;
    } *context = (void *)argument;

    ssize_t numberOfBytes;

    do {
        numberOfBytes = send(context->fd, context->data, context->dataSize, context->flags);
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
SendCallback2(uintptr_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        const void *data;
        size_t dataSize;
        int flags;
        ssize_t numberOfBytes;
        int errorNumber;
    } *context = (void *)argument;

    context->numberOfBytes = -1;
    context->errorNumber = EINTR;
    IOPoller_ClearWatch(&IOPoller, &context->ioWatch);
    Scheduler_ResumeFiber(&Scheduler, context->fiber);
}


static void
RecvFromCallback1(uintptr_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        void *buffer;
        size_t bufferSize;
        int flags;
        struct sockaddr *address;
        socklen_t *addressLength;
        ssize_t numberOfBytes;
        int errorNumber;
    } *context = (void *)argument;

    ssize_t numberOfBytes;

    do {
        numberOfBytes = recvfrom(context->fd, context->buffer, context->bufferSize, context->flags
                                 , context->address, context->addressLength);
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
RecvFromCallback2(uintptr_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        void *buffer;
        size_t bufferSize;
        int flags;
        struct sockaddr *address;
        socklen_t *addressLength;
        ssize_t numberOfBytes;
        int errorNumber;
    } *context = (void *)argument;

    context->numberOfBytes = -1;
    context->errorNumber = EINTR;
    IOPoller_ClearWatch(&IOPoller, &context->ioWatch);
    Scheduler_ResumeFiber(&Scheduler, context->fiber);
}


static void
SendToCallback1(uintptr_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        const void *data;
        size_t dataSize;
        int flags;
        const struct sockaddr *address;
        socklen_t addressLength;
        ssize_t numberOfBytes;
        int errorNumber;
    } *context = (void *)argument;

    ssize_t numberOfBytes;

    do {
        numberOfBytes = sendto(context->fd, context->data, context->dataSize, context->flags
                               , context->address, context->addressLength);
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
SendToCallback2(uintptr_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        const void *data;
        size_t dataSize;
        int flags;
        const struct sockaddr *address;
        socklen_t addressLength;
        ssize_t numberOfBytes;
        int errorNumber;
    } *context = (void *)argument;

    context->numberOfBytes = -1;
    context->errorNumber = EINTR;
    IOPoller_ClearWatch(&IOPoller, &context->ioWatch);
    Scheduler_ResumeFiber(&Scheduler, context->fiber);
}


static void
DoWork(void (*function)(uintptr_t), uintptr_t argument)
{
    struct Work work;
    ThreadPool_PostWork(&ThreadPool, &work, function, argument
                        , (uintptr_t)Scheduler_GetCurrentFiber(&Scheduler), DoWorkCallback);
    Scheduler_SuspendCurrentFiber(&Scheduler);
}


static void
DoWorkCallback(uintptr_t argument)
{
    Scheduler_ResumeFiber(&Scheduler, (struct Fiber *)argument);
}


static void
GetAddrInfoWrapper(uintptr_t argument)
{
    struct {
        const char *hostName;
        const char *serviceName;
        const struct addrinfo *hints;
        struct addrinfo **result;
        int errorCode;
    } *context = (void *)argument;

    context->errorCode = getaddrinfo(context->hostName, context->serviceName, context->hints
                                     , context->result);
}
