/*
$ cc 3.c -lpixy -lpthread && ./a.out

Output:
    1
    2
    3
    4
    5
*/


#include <stdio.h>

#include <Pixy/Runtime.h>
#include <Pixy/Mailbox.h>


static void Producer(uintptr_t);
static void Consumer(uintptr_t);


int
Main(int argc, char **argv)
{
    Call(Producer, 0);
    return 0;
}


static void
Producer(uintptr_t argument)
{
    struct Mailbox mailbox;
    Mailbox_Initialize(&mailbox);
    Call(Consumer, (uintptr_t)&mailbox);
    int i;

    for (i = 1; i <= 5; ++i) {
        Mailbox_PutMail(&mailbox, (uintptr_t)&i, 1);
    }

    Mailbox_PutMail(&mailbox, 0, 0);
}


static void
Consumer(uintptr_t argument)
{
    struct Mailbox *mailbox = (struct Mailbox *)argument;

    for (;;) {
        struct Mail *mail = Mailbox_GetMail(mailbox);

        if (mail->messageLength == 0) {
            Mail_Delete(mail);
            break;
        }

        printf("%d\n", *(int *)mail->message);
        Mail_Delete(mail);
    }
}
