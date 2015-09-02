/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#include "IO.h"

#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/uio.h>
#include <netdb.h>

#include "Scheduler.h"
#include "IOPoller.h"
#include "Timer.h"
#include "ThreadPool.h"
#include "Logging.h"


static bool WaitForFD(int, enum IOCondition, int);
static void WaitForFDCallback1(uintptr_t);
static void WaitForFDCallback2(uintptr_t);
static void WaitForFDCallback3(uintptr_t);
static void DoWork(void (*)(uintptr_t), uintptr_t);
static void DoWorkCallback(uintptr_t);
static void GetAddrInfoWrapper(uintptr_t);
static void GetNameInfoWrapper(uintptr_t);

static void xgetsockopt(int, int, int, void *, socklen_t *);


struct Scheduler Scheduler;
struct IOPoller IOPoller;
struct Timer Timer;
struct ThreadPool ThreadPool;


int
Pipe2(int *fds, int flags)
{
    return pipe2(fds, flags | O_NONBLOCK);
}


ssize_t
Read(int fd, void *buffer, size_t bufferSize, int timeout)
{
    for (;;) {
        ssize_t numberOfBytes;

        do {
            numberOfBytes = read(fd, buffer, bufferSize);
        } while (numberOfBytes < 0 && errno == EINTR);

        if (numberOfBytes >= 0) {
            return numberOfBytes;
        }

        if ((errno != EAGAIN && errno != EWOULDBLOCK) || !WaitForFD(fd, IOReadable, timeout)) {
            return -1;
        }
    }
}


ssize_t
Write(int fd, const void *data, size_t dataSize, int timeout)
{
    for (;;) {
        ssize_t numberOfBytes;

        do {
            numberOfBytes = write(fd, data, dataSize);
        } while (numberOfBytes < 0 && errno == EINTR);

        if (numberOfBytes >= 0) {
            return numberOfBytes;
        }

        if ((errno != EAGAIN && errno != EWOULDBLOCK) || !WaitForFD(fd, IOWritable, timeout)) {
            return -1;
        }
    }
}


ssize_t
ReadV(int fd, const struct iovec *vector, int vectorLength, int timeout)
{
    for (;;) {
        ssize_t numberOfBytes;

        do {
            numberOfBytes = readv(fd, vector, vectorLength);
        } while (numberOfBytes < 0 && errno == EINTR);

        if (numberOfBytes >= 0) {
            return numberOfBytes;
        }

        if ((errno != EAGAIN && errno != EWOULDBLOCK) || !WaitForFD(fd, IOReadable, timeout)) {
            return -1;
        }
    }
}


ssize_t
WriteV(int fd, const struct iovec *vector, int vectorLength, int timeout)
{
    for (;;) {
        ssize_t numberOfBytes;

        do {
            numberOfBytes = writev(fd, vector, vectorLength);
        } while (numberOfBytes < 0 && errno == EINTR);

        if (numberOfBytes >= 0) {
            return numberOfBytes;
        }

        if ((errno != EAGAIN && errno != EWOULDBLOCK) || !WaitForFD(fd, IOWritable, timeout)) {
            return -1;
        }
    }
}


int
Socket(int domain, int type, int protocol)
{
    return socket(domain, type | SOCK_NONBLOCK, protocol);
}


int
Accept4(int fd, struct sockaddr *name, socklen_t *nameSize, int flags, int timeout)
{
    for (;;) {
        int subFD;

        do {
            subFD = accept4(fd, name, nameSize, flags | O_NONBLOCK);
        } while (subFD < 0 && errno == EINTR);

        if (subFD >= 0) {
            return subFD;
        }

        if ((errno != EAGAIN && errno != EWOULDBLOCK) || !WaitForFD(fd, IOReadable, timeout)) {
            return -1;
        }
    }
}


int
Connect(int fd, const struct sockaddr *name, socklen_t nameSize, int timeout)
{
    if (connect(fd, name, nameSize) < 0) {
        if ((errno != EINTR && errno != EINPROGRESS) || !WaitForFD(fd, IOWritable, timeout)) {
            return -1;
        }

        int errorNumber;
        socklen_t optionSize = sizeof errorNumber;
        xgetsockopt(fd, SOL_SOCKET, SO_ERROR, &errorNumber, &optionSize);

        if (errorNumber != 0) {
            errno = errorNumber;
            return -1;
        }
    }

    return 0;
}


ssize_t
Recv(int fd, void *buffer, size_t bufferSize, int flags, int timeout)
{
    for (;;) {
        ssize_t numberOfBytes;

        do {
            numberOfBytes = recv(fd, buffer, bufferSize, flags);
        } while (numberOfBytes < 0 && errno == EINTR);

        if (numberOfBytes >= 0) {
            return numberOfBytes;
        }

        if ((errno != EAGAIN && errno != EWOULDBLOCK) || !WaitForFD(fd, IOReadable, timeout)) {
            return -1;
        }
    }
}


ssize_t
Send(int fd, const void *data, size_t dataSize, int flags, int timeout)
{
    for (;;) {
        ssize_t numberOfBytes;

        do {
            numberOfBytes = send(fd, data, dataSize, flags);
        } while (numberOfBytes < 0 && errno == EINTR);

        if (numberOfBytes >= 0) {
            return numberOfBytes;
        }

        if ((errno != EAGAIN && errno != EWOULDBLOCK) || !WaitForFD(fd, IOWritable, timeout)) {
            return -1;
        }
    }
}


ssize_t
RecvFrom(int fd, void *buffer, size_t bufferSize, int flags, struct sockaddr *name
         , socklen_t *nameSize, int timeout)
{
    for (;;) {
        ssize_t numberOfBytes;

        do {
            numberOfBytes = recvfrom(fd, buffer, bufferSize, flags, name, nameSize);
        } while (numberOfBytes < 0 && errno == EINTR);

        if (numberOfBytes >= 0) {
            return numberOfBytes;
        }

        if ((errno != EAGAIN && errno != EWOULDBLOCK) || !WaitForFD(fd, IOReadable, timeout)) {
            return -1;
        }
    }
}


ssize_t
SendTo(int fd, const void *data, size_t dataSize, int flags, const struct sockaddr *name
       , socklen_t nameSize, int timeout)
{
    for (;;) {
        ssize_t numberOfBytes;

        do {
            numberOfBytes = sendto(fd, data, dataSize, flags, name, nameSize);
        } while (numberOfBytes < 0 && errno == EINTR);

        if (numberOfBytes >= 0) {
            return numberOfBytes;
        }

        if ((errno != EAGAIN && errno != EWOULDBLOCK) || !WaitForFD(fd, IOWritable, timeout)) {
            return -1;
        }
    }
}


ssize_t
RecvMsg(int fd, struct msghdr *message, int flags, int timeout)
{
    for (;;) {
        ssize_t numberOfBytes;

        do {
            numberOfBytes = recvmsg(fd, message, flags);
        } while (numberOfBytes < 0 && errno == EINTR);

        if (numberOfBytes >= 0) {
            return numberOfBytes;
        }

        if ((errno != EAGAIN && errno != EWOULDBLOCK) || !WaitForFD(fd, IOReadable, timeout)) {
            return -1;
        }
    }
}


ssize_t
SendMsg(int fd, const struct msghdr *message, int flags, int timeout)
{
    for (;;) {
        ssize_t numberOfBytes;

        do {
            numberOfBytes = sendmsg(fd, message, flags);
        } while (numberOfBytes < 0 && errno == EINTR);

        if (numberOfBytes >= 0) {
            return numberOfBytes;
        }

        if ((errno != EAGAIN && errno != EWOULDBLOCK) || !WaitForFD(fd, IOWritable, timeout)) {
            return -1;
        }
    }
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
    } context;

    context.hostName = hostName;
    context.serviceName = serviceName;
    context.hints = hints;
    context.result = result;
    DoWork(GetAddrInfoWrapper, (uintptr_t)&context);
    return context.errorCode;
}


int
GetNameInfo(const struct sockaddr *name, socklen_t nameSize, char *hostName, socklen_t hostNameSize
            , char *serviceName, socklen_t serviceNameSize, int flags)
{
    struct {
        const struct sockaddr *name;
        socklen_t nameSize;
        char *hostName;
        socklen_t hostNameSize;
        char *serviceName;
        socklen_t serviceNameSize;
        int flags;
        int errorCode;
    } context;

    context.name = name;
    context.nameSize = nameSize;
    context.hostName = hostName;
    context.hostNameSize = hostNameSize;
    context.serviceName = serviceName;
    context.serviceNameSize = serviceNameSize;
    context.flags = flags;
    DoWork(GetNameInfoWrapper, (uintptr_t)&context);
    return context.errorCode;
}


static bool
WaitForFD(int fd, enum IOCondition ioCondition, int timeout)
{
    if (timeout < 0) {
        struct {
            struct IOWatch ioWatch;
            struct Fiber *fiber;
        } context;

        context.fiber = Scheduler_GetCurrentFiber(&Scheduler);

        if (!IOPoller_SetWatch(&IOPoller, &context.ioWatch, fd, ioCondition, (uintptr_t)&context
                               , WaitForFDCallback1)) {
            return false;
        }

        Scheduler_SuspendCurrentFiber(&Scheduler);
    } else {
        struct {
            struct IOWatch ioWatch;
            struct Timeout timeout;
            struct Fiber *fiber;
            bool ok;
            int errorNumber;
        } context;

        context.fiber = Scheduler_GetCurrentFiber(&Scheduler);

        if (!IOPoller_SetWatch(&IOPoller, &context.ioWatch, fd, ioCondition, (uintptr_t)&context
                               , WaitForFDCallback2)) {
            return false;
        }

        if (!Timer_SetTimeout(&Timer, &context.timeout, timeout, (uintptr_t)&context
                              , WaitForFDCallback3)) {
            IOPoller_ClearWatch(&IOPoller, &context.ioWatch);
            return false;
        }

        Scheduler_SuspendCurrentFiber(&Scheduler);

        if (!context.ok) {
            errno = context.errorNumber;
            return false;
        }
    }

    return true;
}


static void
WaitForFDCallback1(uintptr_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Fiber *fiber;
    } *context = (void *)argument;

    IOPoller_ClearWatch(&IOPoller, &context->ioWatch);
    Scheduler_ResumeFiber(&Scheduler, context->fiber);
}


static void
WaitForFDCallback2(uintptr_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        bool ok;
        int errorNumber;
    } *context = (void *)argument;

    IOPoller_ClearWatch(&IOPoller, &context->ioWatch);
    Timer_ClearTimeout(&Timer, &context->timeout);
    context->ok = true;
    Scheduler_ResumeFiber(&Scheduler, context->fiber);
}


static void
WaitForFDCallback3(uintptr_t argument)
{
    struct {
        struct Fiber *fiber;
        struct IOWatch ioWatch;
        struct Timeout timeout;
        bool ok;
        int errorNumber;
    } *context = (void *)argument;

    IOPoller_ClearWatch(&IOPoller, &context->ioWatch);
    context->ok = false;
    context->errorNumber = EINTR;
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


static void
GetNameInfoWrapper(uintptr_t argument)
{
    struct {
        const struct sockaddr *name;
        socklen_t nameSize;
        char *hostName;
        socklen_t hostNameSize;
        char *serviceName;
        socklen_t serviceNameSize;
        int flags;
        int errorCode;
    } *context = (void *)argument;

    context->errorCode = getnameinfo(context->name, context->nameSize, context->hostName
                                     , context->hostNameSize, context->serviceName
                                     , context->serviceNameSize, context->flags);
}


static void
xgetsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen)
{
    if (getsockopt(sockfd, level, optname, optval, optlen) < 0) {
        LOG_FATAL_ERROR("`getsockopt()` failed: %s", strerror(errno));
    }
}
