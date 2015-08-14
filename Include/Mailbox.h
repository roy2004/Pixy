/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#pragma once


#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

struct __MailboxReader;
struct __MailboxWriter;


struct Mailbox
{
    struct __MailboxReader *lastReader;
    struct __MailboxWriter *lastWriter;
};


struct Mail
{
    void *message;
    size_t messageSize;
};


void Mailbox_Initialize(struct Mailbox *self);
void Mailbox_PutMail(struct Mailbox *self, void *message, size_t messageSize);
struct Mail *Mailbox_GetMail(struct Mailbox *self);
struct Mail *Mailbox_TryGetMail(struct Mailbox *self);
void Mail_Delete(const struct Mail *mail);

#ifdef __cplusplus
} // extern "C"
#endif
