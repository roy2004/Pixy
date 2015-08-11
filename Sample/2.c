/*
$ cc 2.c -lpixy -lpthread && ./a.out

Output:
    Hello!
    Hello!
    Hello!
    Hello!
    Hello!
*/


#include <stdio.h>

#include <Pixy/Runtime.h>
#include <Pixy/IO.h>


static void Writer(uintptr_t);
static void Reader(uintptr_t);


int
Main(int argc, char **argv)
{
    int fds[2];
    Pipe2(fds, 0);
    Call(Reader, fds[0]);
    Call(Writer, fds[1]);
    return 0;
}


static void
Reader(uintptr_t argument)
{
    int fd = argument;
    char buffer[100];

    while (Read(fd, buffer, sizeof buffer, -1) >= 1) {
        puts(buffer);
    }

    Close(fd);
}


static void
Writer(uintptr_t argument)
{
    int fd = argument;
    char message[] = "Hello!";
    int i;

    for (i = 0; i < 5; ++i) {
        Write(fd, message, sizeof message, -1);
        Sleep(1000);
    }

    Close(fd);
}
