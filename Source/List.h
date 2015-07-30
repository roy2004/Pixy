#pragma once


#include <assert.h>

#include "Base.h"


#define FOR_EACH_LIST_ITEM_BACKWARD(LIST_ITEM, LIST_HEAD)                                       \
    assert((LIST_HEAD) != NULL);                                                                \
    for ((LIST_ITEM) = (LIST_HEAD)->prev; (LIST_ITEM) != (LIST_HEAD); (LIST_ITEM) = (LIST_ITEM) \
                                                                                    ->prev)

#define FOR_EACH_LIST_ITEM_FORWARD(LIST_ITEM, LIST_HEAD)                                        \
    assert((LIST_HEAD) != NULL);                                                                \
    for ((LIST_ITEM) = (LIST_HEAD)->next; (LIST_ITEM) != (LIST_HEAD); (LIST_ITEM) = (LIST_ITEM) \
                                                                                    ->next)


struct ListItem
{
    struct ListItem *prev;
    struct ListItem *next;
};


static inline void List_Initialize(struct ListItem *);
static inline void List_InsertBack(struct ListItem *, struct ListItem *);
static inline void List_InsertFront(struct ListItem *, struct ListItem *);
static inline bool List_IsEmpty(const struct ListItem *);
#define List_GetBack ListItem_GetPrev
#define List_GetFront ListItem_GetNext

static inline void ListItem_InsertBefore(struct ListItem *, struct ListItem *);
static inline void ListItem_InsertAfter(struct ListItem *, struct ListItem *);
static inline void ListItem_Replace(const struct ListItem *, struct ListItem *);
static inline void ListItem_Remove(const struct ListItem *);
static inline struct ListItem *ListItem_GetPrev(const struct ListItem *);
static inline struct ListItem *ListItem_GetNext(const struct ListItem *);
static inline void __ListItem_Insert(struct ListItem *, struct ListItem *, struct ListItem *);

void List_Sort(struct ListItem *, int (*)(const struct ListItem *, const struct ListItem *));


static inline void
List_Initialize(struct ListItem *head)
{
    assert(head != NULL);
    head->prev = head;
    head->next = head;
}


static inline void
List_InsertBack(struct ListItem *head, struct ListItem *back)
{
    assert(head != NULL);
    assert(back != NULL);
    __ListItem_Insert(back, head->prev, head);
}


static inline void
List_InsertFront(struct ListItem *head, struct ListItem *front)
{
    assert(head != NULL);
    assert(front != NULL);
    __ListItem_Insert(front, head, head->next);
}


static inline bool
List_IsEmpty(const struct ListItem *head)
{
    assert(head != NULL);
    return head->prev == head;
}


static inline void
ListItem_InsertBefore(struct ListItem *self, struct ListItem *other)
{
    assert(self != NULL);
    assert(other != NULL);
    __ListItem_Insert(self, other->prev, other);
}


static inline void
ListItem_InsertAfter(struct ListItem *self, struct ListItem *other)
{
    assert(self != NULL);
    assert(other != NULL);
    __ListItem_Insert(self, other, other->next);
}


static inline void
ListItem_Replace(const struct ListItem *self, struct ListItem *other)
{
    assert(self != NULL);
    assert(other != NULL);
    __ListItem_Insert(other, self->prev, self->next);
}


static inline void
ListItem_Remove(const struct ListItem *self)
{
    assert(self != NULL);
    self->prev->next = self->next;
    self->next->prev = self->prev;
}


static inline struct ListItem *
ListItem_GetPrev(const struct ListItem *self)
{
    assert(self != NULL);
    return self->prev;
}


static inline struct ListItem *
ListItem_GetNext(const struct ListItem *self)
{
    assert(self != NULL);
    return self->next;
}


static inline void
__ListItem_Insert(struct ListItem *self, struct ListItem *prev, struct ListItem *next)
{
    (self->prev = prev)->next = self;
    (self->next = next)->prev = self;
}
