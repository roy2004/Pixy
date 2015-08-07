#include "Runtime.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/uio.h>
#include <netdb.h>

#include <stddef.h>
#include <errno.h>

#include "Scheduler.h"
#include "IOPoller.h"
#include "Timer.h"
#include "ThreadPool.h"


static void SleepCallback(uintptr_t);
static void ReadVCallback1(uintptr_t);
static void WriteVCallback1(uintptr_t);
static void ReadVCallback2(uintptr_t);
static void WriteVCallback2(uintptr_t);
static void Accept4Callback1(uintptr_t);
static void Accept4Callback2(uintptr_t);
static void ConnectCallback1(uintptr_t);
static void ConnectCallback2(uintptr_t);
static void RecvMsgCallback1(uintptr_t);
static void RecvMsgCallback2(uintptr_t);
static void SendMsgCallback1(uintptr_t);
static void SendMsgCallback2(uintptr_t);
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
    struct iovec vector = {
        .iov_base = buffer,
        .iov_len = bufferSize
    };

    return ReadV(fd, &vector, 1, timeout);
}


ssize_t
Write(int fd, const void *data, size_t dataSize, int timeout)
{
    struct iovec vector = {
        .iov_base = (void *)data,
        .iov_len = dataSize
    };

    return WriteV(fd, &vector, 1, timeout);
}


ssize_t
ReadV(int fd, const struct iovec *vector, int vectorLength, int timeout)
{
    ssize_t numberOfBytes;

    do {
        numberOfBytes = readv(fd, vector, vectorLength);
    } while (numberOfBytes < 0 && errno == EINTR);

    if (numberOfBytes >= 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
        return numberOfBytes;
    }

    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        const struct iovec *vector;
        int vectorLength;
        ssize_t numberOfBytes;
        int errorNumber;
    } context = {
        .fiber = Scheduler_GetCurrentFiber(&Scheduler),
        .fd = fd,
        .vector = vector,
        .vectorLength = vectorLength
    };

    if (IOPoller_SetWatch(&IOPoller, &context.ioWatch, fd, IOReadable, (uintptr_t)&context
                          , ReadVCallback1) < 0) {
        return -1;
    }

    if (Timer_SetTimeout(&Timer, &context.timeout, timeout, (uintptr_t)&context, ReadVCallback2)
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
WriteV(int fd, const struct iovec *vector, int vectorLength, int timeout)
{
    ssize_t numberOfBytes;

    do {
        numberOfBytes = writev(fd, vector, vectorLength);
    } while (numberOfBytes < 0 && errno == EINTR);

    if (numberOfBytes >= 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
        return numberOfBytes;
    }

    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        const struct iovec *vector;
        int vectorLength;
        ssize_t numberOfBytes;
        int errorNumber;
    } context = {
        .fiber = Scheduler_GetCurrentFiber(&Scheduler),
        .fd = fd,
        .vector = vector,
        .vectorLength = vectorLength
    };

    if (IOPoller_SetWatch(&IOPoller, &context.ioWatch, fd, IOWritable, (uintptr_t)&context
                          , WriteVCallback1) < 0) {
        return -1;
    }

    if (Timer_SetTimeout(&Timer, &context.timeout, timeout, (uintptr_t)&context, WriteVCallback2)
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
    struct iovec vector = {
        .iov_base = buffer,
        .iov_len = bufferSize
    };

    struct msghdr message = {
        .msg_name = NULL,
        .msg_namelen = 0,
        .msg_iov = &vector,
        .msg_iovlen = 1,
        .msg_control = NULL,
        .msg_controllen = 0,
        .msg_flags = 0
    };

    return RecvMsg(fd, &message, flags, timeout);
}


ssize_t
Send(int fd, const void *data, size_t dataSize, int flags, int timeout)
{
    struct iovec vector = {
        .iov_base = (void *)data,
        .iov_len = dataSize
    };

    struct msghdr message = {
        .msg_name = NULL,
        .msg_namelen = 0,
        .msg_iov = &vector,
        .msg_iovlen = 1,
        .msg_control = NULL,
        .msg_controllen = 0,
        .msg_flags = 0
    };

    return SendMsg(fd, &message, flags, timeout);
}


ssize_t
RecvFrom(int fd, void *buffer, size_t bufferSize, int flags, struct sockaddr *address
         , socklen_t *addressLength, int timeout)
{
    struct iovec vector = {
        .iov_base = buffer,
        .iov_len = bufferSize
    };

    struct msghdr message = {
        .msg_name = address,
        .msg_namelen = *addressLength,
        .msg_iov = &vector,
        .msg_iovlen = 1,
        .msg_control = NULL,
        .msg_controllen = 0,
        .msg_flags = 0
    };

    ssize_t numberOfBytes = RecvMsg(fd, &message, flags, timeout);
    *addressLength = message.msg_namelen;
    return numberOfBytes;
}


ssize_t
SendTo(int fd, const void *data, size_t dataSize, int flags, const struct sockaddr *address
       , socklen_t addressLength, int timeout)
{
    struct iovec vector = {
        .iov_base = (void *)data,
        .iov_len = dataSize
    };

    struct msghdr message = {
        .msg_name = (struct sockaddr *)address,
        .msg_namelen = addressLength,
        .msg_iov = &vector,
        .msg_iovlen = 1,
        .msg_control = NULL,
        .msg_controllen = 0,
        .msg_flags = 0
    };

    return SendMsg(fd, &message, flags, timeout);
}


ssize_t
RecvMsg(int fd, struct msghdr *message, int flags, int timeout)
{
    ssize_t numberOfBytes;

    do {
        numberOfBytes = recvmsg(fd, message, flags);
    } while (numberOfBytes < 0 && errno == EINTR);

    if (numberOfBytes >= 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
        return numberOfBytes;
    }

    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        struct msghdr *message;
        int flags;
        ssize_t numberOfBytes;
        int errorNumber;
    } context = {
        .fiber = Scheduler_GetCurrentFiber(&Scheduler),
        .fd = fd,
        .message = message,
        .flags = flags
    };

    if (IOPoller_SetWatch(&IOPoller, &context.ioWatch, fd, IOReadable, (uintptr_t)&context
                          , RecvMsgCallback1) < 0) {
        return -1;
    }

    if (Timer_SetTimeout(&Timer, &context.timeout, timeout, (uintptr_t)&context, RecvMsgCallback2)
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
SendMsg(int fd, const struct msghdr *message, int flags, int timeout)
{
    ssize_t numberOfBytes;

    do {
        numberOfBytes = sendmsg(fd, message, flags);
    } while (numberOfBytes < 0 && errno == EINTR);

    if (numberOfBytes >= 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
        return numberOfBytes;
    }

    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        const struct msghdr *message;
        int flags;
        ssize_t numberOfBytes;
        int errorNumber;
    } context = {
        .fiber = Scheduler_GetCurrentFiber(&Scheduler),
        .fd = fd,
        .message = message,
        .flags = flags
    };

    if (IOPoller_SetWatch(&IOPoller, &context.ioWatch, fd, IOWritable, (uintptr_t)&context
                          , SendMsgCallback1) < 0) {
        return -1;
    }

    if (Timer_SetTimeout(&Timer, &context.timeout, timeout, (uintptr_t)&context, SendMsgCallback2)
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
ReadVCallback1(uintptr_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        const struct iovec *vector;
        int vectorLength;
        ssize_t numberOfBytes;
        int errorNumber;
    } *context = (void *)argument;

    ssize_t numberOfBytes;

    do {
        numberOfBytes = readv(context->fd, context->vector, context->vectorLength);
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
ReadVCallback2(uintptr_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        const struct iovec *vector;
        int vectorLength;
        ssize_t numberOfBytes;
        int errorNumber;
    } *context = (void *)argument;

    context->numberOfBytes = -1;
    context->errorNumber = EINTR;
    IOPoller_ClearWatch(&IOPoller, &context->ioWatch);
    Scheduler_ResumeFiber(&Scheduler, context->fiber);
}


static void
WriteVCallback1(uintptr_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        const struct iovec *vector;
        int vectorLength;
        ssize_t numberOfBytes;
        int errorNumber;
    } *context = (void *)argument;

    ssize_t numberOfBytes;

    do {
        numberOfBytes = writev(context->fd, context->vector, context->vectorLength);
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
WriteVCallback2(uintptr_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        const struct iovec *vector;
        int vectorLength;
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
RecvMsgCallback1(uintptr_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        struct msghdr *message;
        int flags;
        ssize_t numberOfBytes;
        int errorNumber;
    } *context = (void *)argument;

    ssize_t numberOfBytes;

    do {
        numberOfBytes = recvmsg(context->fd, context->message, context->flags);
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
RecvMsgCallback2(uintptr_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        struct msghdr *message;
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
SendMsgCallback1(uintptr_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        const struct msghdr *message;
        int flags;
        ssize_t numberOfBytes;
        int errorNumber;
    } *context = (void *)argument;

    ssize_t numberOfBytes;

    do {
        numberOfBytes = sendmsg(context->fd, context->message, context->flags);
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
SendMsgCallback2(uintptr_t argument)
{
    struct {
        struct IOWatch ioWatch;
        struct Timeout timeout;
        struct Fiber *fiber;
        int fd;
        const struct msghdr *message;
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
