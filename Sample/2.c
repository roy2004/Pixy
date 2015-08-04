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


static void Writer(any_t);
static void Reader(any_t);


int
Main(int argc, char **argv)
{
    int fds[2];
    Pipe2(fds, 0);
    Call(Reader, (any_t)fds[0]);
    Call(Writer, (any_t)fds[1]);
    return 0;
}


static void
Reader(any_t argument)
{
    int fd = (int)argument;
    int i;

    for (;;) {
        char buffer[100];
        ssize_t numberOfBytes = Read(fd, buffer, sizeof buffer, -1);

        if (numberOfBytes <= 0) {
            break;
        }

        printf("%.*s\n", (int)numberOfBytes, buffer);
    }

    Close(fd);
}


static void
Writer(any_t argument)
{
    int fd = (int)argument;
    int i;

    for (i = 0; i < 5; ++i) {
        char message[] = "Hello!";
        Write(fd, message, sizeof message, -1);
        Sleep(1000);
    }

    Close(fd);
}
