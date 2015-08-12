/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#include "List.h"


static void List_Quicksort(struct ListItem **, struct ListItem **
                           , int (*)(const struct ListItem *, const struct ListItem *));


void
List_Sort(struct ListItem *head, int (*itemComparer)(const struct ListItem *
                                                     , const struct ListItem *))
{
    assert(head != NULL);
    assert(itemComparer != NULL);
    List_Quicksort(&head->next, &head->prev, itemComparer);
}


static void
List_Quicksort(struct ListItem **front, struct ListItem **back
               , int (*itemComparer)(const struct ListItem *, const struct ListItem *))
{
    struct ListItem *item1 = *front;
    struct ListItem *item2 = *back;

    if (item2 == item1) {
        return;
    }

    do {
        if (itemComparer(item2, item1) >= 0) {
            item2 = item2->prev;
            continue;
        }

        struct ListItem *item3 = item2;
        item2 = item2->prev;
        ListItem_Remove(item3);
        ListItem_InsertBefore(item3, item1);
    } while (item2 != item1);

    if (*front != item1) {
        List_Quicksort(front, &item1->prev, itemComparer);
    }

    if (*back != item1) {
        List_Quicksort(&item1->next, back, itemComparer);
    }
}
