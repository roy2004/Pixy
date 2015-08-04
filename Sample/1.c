/*
$ cc 1.c -lpixy -lpthread && ./a.out

Output:
    A - 1
    B - 1
    A - 2
    B - 2
    A - 3
    B - 3
    Wait 2 seconds...
    Wait 2 seconds...
    Done!
    Done!
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
    printf("%c - 1\n", who);
    Yield();
    printf("%c - 2\n", who);
    Yield();
    printf("%c - 3\n", who);
    Yield();
    puts("Wait 2 seconds...");
    Sleep(2000);
    puts("Done!");
}
