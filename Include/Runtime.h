#pragma once


#include <sys/types.h>
#include <sys/socket.h>

#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif

struct iovec;
struct addrinfo;


int Main(int argc, char **argv);
int Call(void (*coroutine)(uintptr_t), uintptr_t argument);
void Yield(void);
void Exit(void);
int Sleep(int duration);

int Pipe2(int *fds, int flags);
ssize_t Read(int fd, void *buffer, size_t bufferSize, int timeout);
ssize_t Write(int fd, const void *data, size_t dataSize, int timeout);
ssize_t ReadV(int fd, const struct iovec *vector, int vectorLength, int timeout);
ssize_t WriteV(int fd, const struct iovec *vector, int vectorLength, int timeout);

int Socket(int domain, int type, int protocol);
int Accept4(int fd, struct sockaddr *address, socklen_t *addressLength, int flags, int timeout);
int Connect(int fd, const struct sockaddr *address, socklen_t addressLength, int timeout);
ssize_t Recv(int fd, void *buffer, size_t bufferSize, int flags, int timeout);
ssize_t Send(int fd, const void *data, size_t dataSize, int flags, int timeout);
ssize_t RecvFrom(int fd, void *buffer, size_t bufferSize, int flags, struct sockaddr *address
                 , socklen_t *addressLength, int timeout);
ssize_t SendTo(int fd, const void *data, size_t dataSize, int flags, const struct sockaddr *address
               , socklen_t addressLength, int timeout);
ssize_t RecvMsg(int fd, struct msghdr *message, int flags, int timeout);
ssize_t SendMsg(int fd, const struct msghdr *message, int flags, int timeout);

int Close(int fd);

int GetAddrInfo(const char *hostName, const char *serviceName, const struct addrinfo *hints
                , struct addrinfo **result);


#ifdef __cplusplus
} // extern "C"
#endif
