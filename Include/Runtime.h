#pragma once


#include <sys/types.h>

#include "Base.h"


bool
Call(
    void (*coroutine)(any_t),
    any_t argument
);

void
Yield(void);

bool
Sleep(
    int duration
);

ssize_t
Read(
    int fd,
    void *buffer,
    size_t bufferSize,
    int timeout
);

ssize_t
Write(
    int fd,
    const void *data,
    size_t dataSize,
    int timeout
);

ssize_t
ReadFully(
    int fd,
    void *buffer,
    size_t bufferSize,
    int timeout
);

ssize_t
WriteFully(
    int fd,
    const void *data,
    size_t dataSize,
    int timeout
);
