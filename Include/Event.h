/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#pragma once


#if defined __cplusplus
extern "C" {
#endif

struct __EventWaiter;


struct Event
{
    struct __EventWaiter *lastWaiter;
};


void Event_Initialize(struct Event *);
void Event_Trigger(struct Event *);
void Event_WaitFor(struct Event *);

#if defined __cplusplus
} // extern "C"
#endif
