/*
 * Copyright (C) 2015 Roy O'Young <roy2220@outlook.com>.
 */


#include "Scheduler.h"

#include <stdlib.h>

#if defined USE_VALGRIND
#include <valgrind/valgrind.h>
#endif

#include "Utility.h"


#define FIBER_SIZE ((size_t)65536)


struct Fiber
{
    struct ListItem listItem;
    char *stack;
    size_t stackSize;
#if defined USE_VALGRIND
    int stackID;
#endif
    jmp_buf *context;
    void (*function)(uintptr_t);
    uintptr_t argument;
};


static NORETURN void Scheduler_SwitchToFiber(struct Scheduler *, struct Fiber *);
static NORETURN void Scheduler_FiberStart(struct Scheduler *, struct Fiber *);
static NORETURN void Scheduler_SwitchTo(struct Scheduler *);

static struct Fiber *Fiber_Allocate(void);
static void Fiber_Free(struct Fiber *);


void
Scheduler_Initialize(struct Scheduler *self)
{
    assert(self != NULL);
    self->activeFiber = NULL;
    List_Initialize(&self->readyFiberListHead);
    List_Initialize(&self->deadFiberListHead);
    self->fiberCount = 0;
}


void
Scheduler_Finalize(const struct Scheduler *self)
{
    assert(self != NULL && self->activeFiber == NULL);
    struct ListItem *fiberListItem = List_GetBack(&self->readyFiberListHead);

    while (fiberListItem != &self->readyFiberListHead) {
        struct Fiber *fiber = CONTAINER_OF(fiberListItem, struct Fiber, listItem);
        fiberListItem = ListItem_GetPrev(fiberListItem);
        Fiber_Free(fiber);
    }
}


bool
Scheduler_AddFiber(struct Scheduler *self, void (*function)(uintptr_t), uintptr_t argument)
{
    assert(self != NULL);
    assert(function != NULL);
    struct Fiber *fiber;

    if (List_IsEmpty(&self->deadFiberListHead)) {
        fiber = Fiber_Allocate();

        if (fiber == NULL) {
            return false;
        }
    } else {
        fiber = CONTAINER_OF(List_GetBack(&self->deadFiberListHead), struct Fiber, listItem);
        ListItem_Remove(&fiber->listItem);
    }

    fiber->context = NULL;
    fiber->function = function;
    fiber->argument = argument;
    List_InsertBack(&self->readyFiberListHead, &fiber->listItem);
    ++self->fiberCount;
    return true;
}


bool
Scheduler_AddAndRunFiber(struct Scheduler *self, void (*function)(uintptr_t), uintptr_t argument)
{
    assert(self != NULL && self->activeFiber != NULL);

    if (!Scheduler_AddFiber(self, function, argument)) {
        return false;
    }

    jmp_buf context;

    if (setjmp(context) != 0) {
        return true;
    }

    self->activeFiber->context = &context;
    List_InsertFront(&self->readyFiberListHead, &self->activeFiber->listItem);
    struct Fiber *fiber = CONTAINER_OF(List_GetBack(&self->readyFiberListHead), struct Fiber
                                       , listItem);
    ListItem_Remove(&fiber->listItem);
    Scheduler_SwitchToFiber(self, fiber);
}


void
Scheduler_YieldCurrentFiber(struct Scheduler *self)
{
    assert(self != NULL && self->activeFiber != NULL);

    if (List_IsEmpty(&self->readyFiberListHead)) {
        return;
    }

    jmp_buf context;

    if (setjmp(context) != 0) {
        return;
    }

    self->activeFiber->context = &context;
    List_InsertBack(&self->readyFiberListHead, &self->activeFiber->listItem);
    struct Fiber *fiber = CONTAINER_OF(List_GetFront(&self->readyFiberListHead), struct Fiber
                                       , listItem);
    ListItem_Remove(&fiber->listItem);
    Scheduler_SwitchToFiber(self, fiber);
}


void
Scheduler_SuspendCurrentFiber(struct Scheduler *self)
{
    assert(self != NULL && self->activeFiber != NULL);

    jmp_buf context;

    if (setjmp(context) != 0) {
        return;
    }

    self->activeFiber->context = &context;

    if (List_IsEmpty(&self->readyFiberListHead)) {
        Scheduler_SwitchTo(self);
    } else {
        struct Fiber *fiber = CONTAINER_OF(List_GetFront(&self->readyFiberListHead), struct Fiber
                                           , listItem);
        ListItem_Remove(&fiber->listItem);
        Scheduler_SwitchToFiber(self, fiber);
    }
}


void
Scheduler_ResumeFiber(struct Scheduler *self, struct Fiber *fiber)
{
    assert(self != NULL && self->activeFiber != fiber);
    assert(fiber != NULL);
    List_InsertBack(&self->readyFiberListHead, &fiber->listItem);
}


void
Scheduler_UnresumeFiber(struct Scheduler *self, struct Fiber *fiber)
{
    assert(self != NULL && self->activeFiber != fiber);
    assert(fiber != NULL);
    ListItem_Remove(&fiber->listItem);
}


NORETURN void
Scheduler_ExitCurrentFiber(struct Scheduler *self)
{
    assert(self != NULL && self->activeFiber != NULL);
    List_InsertBack(&self->deadFiberListHead, &self->activeFiber->listItem);
    --self->fiberCount;

    if (List_IsEmpty(&self->readyFiberListHead)) {
        Scheduler_SwitchTo(self);
    } else {
        struct Fiber *fiber = CONTAINER_OF(List_GetFront(&self->readyFiberListHead), struct Fiber
                                           , listItem);
        ListItem_Remove(&fiber->listItem);
        Scheduler_SwitchToFiber(self, fiber);
    }
}


void
Scheduler_Tick(struct Scheduler *self)
{
    assert(self != NULL && self->activeFiber == NULL);

    if (List_IsEmpty(&self->readyFiberListHead)) {
        return;
    }

    jmp_buf context;

    if (setjmp(context) == 0) {
        self->context = &context;
        struct Fiber *fiber = CONTAINER_OF(List_GetFront(&self->readyFiberListHead), struct Fiber
                                           , listItem);
        ListItem_Remove(&fiber->listItem);
        Scheduler_SwitchToFiber(self, fiber);
    } else {
        struct ListItem *fiberListItem = List_GetBack(&self->deadFiberListHead);

        if (fiberListItem == &self->deadFiberListHead) {
            return;
        }

        do {
            struct Fiber *fiber = CONTAINER_OF(fiberListItem, struct Fiber, listItem);
            fiberListItem = ListItem_GetPrev(fiberListItem);
            Fiber_Free(fiber);
        } while (fiberListItem != &self->deadFiberListHead);

        List_Initialize(&self->deadFiberListHead);
    }
}


static NORETURN void
Scheduler_SwitchToFiber(struct Scheduler *self, struct Fiber *fiber)
{
    self->activeFiber = fiber;

    if (fiber->context == NULL) {
        __asm__ __volatile__ (
#if defined __i386__
            "movl\t$0, %%ebp\n\t"
            "movl\t%0, %%esp\n\t"
            "pushl\t%1\n\t"
            "pushl\t%2\n\t"
            "pushl\t$0\n\t"
            "jmpl\t*%3"
            :
            : "r"(fiber->stack + fiber->stackSize), "r"(self), "r"(fiber), "r"(Scheduler_FiberStart)
#elif defined __x86_64__
            "movq\t$0, %%rbp\n\t"
            "movq\t%0, %%rsp\n\t"
            "pushq\t$0\n\t"
            "jmpq\t*%3"
            :
            : "r"(fiber->stack + fiber->stackSize), "D"(self), "S"(fiber), "r"(Scheduler_FiberStart)
#else
#error architecture not supported
#endif
        );

        __builtin_unreachable();
    } else {
        longjmp(*fiber->context, 1);
    }
}


static NORETURN void
Scheduler_FiberStart(struct Scheduler *self, struct Fiber *fiber)
{
    fiber->function(fiber->argument);
    Scheduler_ExitCurrentFiber(self);
}


static NORETURN void
Scheduler_SwitchTo(struct Scheduler *self)
{
    self->activeFiber = NULL;
    longjmp(*self->context, 1);
}


static struct Fiber *
Fiber_Allocate(void)
{
    char *region = malloc(FIBER_SIZE);

    if (region == NULL) {
        return NULL;
    }

#if defined __i386__ || defined __x86_64__
    struct Fiber *self = (struct Fiber *)(region + FIBER_SIZE - sizeof *self);
    self->stack = region;
    self->stackSize = FIBER_SIZE - sizeof *self;
#else
#error architecture not supported
#endif
#if defined USE_VALGRIND
    self->stackID = VALGRIND_STACK_REGISTER(self->stack, self->stack + self->stackSize);
#endif
    return self;
}


static void
Fiber_Free(struct Fiber *self)
{
#if defined USE_VALGRIND
    VALGRIND_STACK_DEREGISTER(self->stackID);
#endif
#if defined __i386__ || defined __x86_64__
    char *region = (char *)self + sizeof *self - FIBER_SIZE;
#else
#error architecture not supported
#endif
    free(region);
}
