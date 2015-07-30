/*
$ cc 1.c -lpixy && ./a.out

Output:
    A says 1
    B says 1
    A says 2
    B says 2
    A says 3
    B says 3
*/


#include <stdio.h>

#include <Pixy/Runtime.h>


static void Coroutine(any_t);


int
Main(int argc, char **argv)
{
    Call(Coroutine, (any_t)'A');
    Call(Coroutine, (any_t)'B');
    return 0;
}


static void
Coroutine(any_t argument)
{
    char who = (char)argument;
    printf("%c says 1\n", who);
    Yield();
    printf("%c says 2\n", who);
    Yield();
    printf("%c says 3\n", who);
    Yield();
}
