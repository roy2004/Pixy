#pragma once


#include <sys/types.h>
#include <sys/socket.h>

#include "Base.h"


bool Call(void (*coroutine)(any_t), any_t argument);
void Yield(void);
void Exit(void);
bool Sleep(int duration);

int Pipe2(int *fds, int flags);
ssize_t Read(int fd, void *buffer, size_t bufferSize, int timeout);
ssize_t Write(int fd, const void *data, size_t dataSize, int timeout);
ssize_t ReadFully(int fd, void *buffer, size_t bufferSize, int timeout);
ssize_t WriteFully(int fd, const void *data, size_t dataSize, int timeout);

int Socket(int domain, int type, int protocol);
int Accept4(int fd, struct sockaddr *address, socklen_t *addressLength, int flags, int timeout);
int Connect(int fd, const struct sockaddr *address, socklen_t addressLength, int timeout);
ssize_t Recv(int fd, void *buffer, size_t bufferSize, int flags, int timeout);
ssize_t Send(int fd, const void *data, size_t dataSize, int flags, int timeout);
ssize_t RecvFully(int fd, void *buffer, size_t bufferSize, int flags, int timeout);
ssize_t SendFully(int fd, const void *data, size_t dataSize, int flags, int timeout);
