/*
$ cc 2.c -lpixy && ./a.out

Output:
    Wait 2 seconds...
    Done!
*/


#include <stdio.h>

#include <Pixy/Runtime.h>


int
Main(int argc, char **argv)
{
    puts("Wait 2 seconds...");
    Sleep(2000);
    puts("Done!");
    return 0;
}
