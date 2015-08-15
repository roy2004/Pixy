/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#include "Mailbox.h"

#include <stdlib.h>

#include "Scheduler.h"
#include "Utility.h"


struct __MailboxReader
{
    struct __MailboxReader *prev;
    struct Fiber *fiber;
    struct Mail *mail;
};


struct __MailboxWriter
{
    struct __MailboxWriter *prev;
    struct Fiber *fiber;
    struct Mail mail;
};


struct Scheduler Scheduler;


void
Mailbox_Initialize(struct Mailbox *self)
{
    if (self == NULL) {
        abort();
    }

    self->lastReader = NULL;
    self->lastWriter = NULL;
}


void
Mailbox_PutMail(struct Mailbox *self, uintptr_t message, ptrdiff_t messageLength)
{
    if (self == NULL) {
        abort();
    }

    struct __MailboxWriter writer = {
        .fiber = Scheduler_GetCurrentFiber(&Scheduler),

        .mail = {
            .message = message,
            .messageLength = messageLength
        }
    };

    struct __MailboxReader *reader = self->lastReader;

    if (reader != NULL) {
        self->lastReader = reader->prev;
        reader->mail = &writer.mail;
        Scheduler_ResumeFiber(&Scheduler, reader->fiber);
    } else {
        writer.prev = self->lastWriter;
        self->lastWriter = &writer;
    }

    Scheduler_SuspendCurrentFiber(&Scheduler);
}


struct Mail *
Mailbox_GetMail(struct Mailbox *self)
{
    if (self == NULL) {
        abort();
    }

    struct __MailboxWriter *writer = self->lastWriter;

    if (writer != NULL) {
        self->lastWriter = writer->prev;
        return &writer->mail;
    }

    struct __MailboxReader reader = {
        .fiber = Scheduler_GetCurrentFiber(&Scheduler)
    };

    reader.prev = self->lastReader;
    self->lastReader = &reader;
    Scheduler_SuspendCurrentFiber(&Scheduler);
    return reader.mail;
}


struct Mail *
Mailbox_TryGetMail(struct Mailbox *self)
{
    if (self == NULL) {
        abort();
    }

    struct __MailboxWriter *writer = self->lastWriter;

    if (writer == NULL) {
        return NULL;
    }

    self->lastWriter = writer->prev;
    return &writer->mail;
}


void
Mail_Delete(const struct Mail *self)
{
    if (self == NULL) {
        abort();
    }

    Scheduler_ResumeFiber(&Scheduler, CONTAINER_OF(self, const struct __MailboxWriter, mail)
                                      ->fiber);
}
