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


#include <stdint.h>
#include <stdio.h>

#include <Pixy/Runtime.h>


static void Foo(uintptr_t);


int
FiberMain(int argc, char **argv)
{
    AddFiber(Foo, 'A');
    AddFiber(Foo, 'B');
    return 0;
}


static void
Foo(uintptr_t argument)
{
    char who = argument;
    printf("%c - 1\n", who);
    YieldCurrentFiber();
    printf("%c - 2\n", who);
    YieldCurrentFiber();
    printf("%c - 3\n", who);
    YieldCurrentFiber();
    puts("Wait 2 seconds...");
    SleepCurrentFiber(2000);
    puts("Done!");
}
