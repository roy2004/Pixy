#include "Scheduler.h"

#include <stdlib.h>


#define FIBER_SIZE ((size_t)65536)


struct Fiber
{
    struct ListItem listItem;
    void *stack;
    jmp_buf *context;
    void (*coroutine)(any_t);
    any_t argument;
};


static void Scheduler_RunFiber(struct Scheduler *, struct Fiber *);
static void Scheduler_FiberStart(struct Scheduler *, struct Fiber *);
static void Scheduler_Return(struct Scheduler *);

static struct Fiber *AllocateFiber(void);
static void FreeFiber(struct Fiber *);


void
Scheduler_Initialize(struct Scheduler *self)
{
    assert(self != NULL);
    self->runningFiber = NULL;
    List_Initialize(&self->readyFiberListHead);
    List_Initialize(&self->deadFiberListHead);
    self->fiberCount = 0;
}


void
Scheduler_Finalize(const struct Scheduler *self)
{
    assert(self != NULL && self->runningFiber == NULL);
    struct ListItem *fiberListItem = List_GetBack(&self->readyFiberListHead);

    while (fiberListItem != &self->readyFiberListHead) {
        struct Fiber *fiber = CONTAINER_OF(fiberListItem, struct Fiber, listItem);
        fiberListItem = ListItem_GetPrev(fiberListItem);
        FreeFiber(fiber);
    }
}


bool
Scheduler_CallCoroutine(struct Scheduler *self, void (*coroutine)(any_t), any_t argument)
{
    assert(self != NULL);
    assert(coroutine != NULL);
    struct Fiber *fiber;

    if (List_IsEmpty(&self->deadFiberListHead)) {
        fiber = AllocateFiber();

        if (fiber == NULL) {
            return false;
        }
    } else {
        fiber = CONTAINER_OF(List_GetBack(&self->deadFiberListHead), struct Fiber, listItem);
        ListItem_Remove(&fiber->listItem);
    }

    fiber->context = NULL;
    fiber->coroutine = coroutine;
    fiber->argument = argument;
    List_InsertBack(&self->readyFiberListHead, &fiber->listItem);
    ++self->fiberCount;
    return true;
}


void
Scheduler_YieldCurrentFiber(struct Scheduler *self)
{
    assert(self != NULL && self->runningFiber != NULL);

    if (List_IsEmpty(&self->readyFiberListHead)) {
        return;
    }

    jmp_buf context;

    if (setjmp(context) != 0) {
        return;
    }

    self->runningFiber->context = &context;
    List_InsertBack(&self->readyFiberListHead, &self->runningFiber->listItem);
    struct Fiber *fiber = CONTAINER_OF(List_GetFront(&self->readyFiberListHead), struct Fiber
                                       , listItem);
    ListItem_Remove(&fiber->listItem);
    Scheduler_RunFiber(self, fiber);
}


void
Scheduler_SuspendCurrentFiber(struct Scheduler *self)
{
    assert(self != NULL && self->runningFiber != NULL);

    jmp_buf context;

    if (setjmp(context) != 0) {
        return;
    }

    self->runningFiber->context = &context;

    if (List_IsEmpty(&self->readyFiberListHead)) {
        Scheduler_Return(self);
    } else {
        struct Fiber *fiber = CONTAINER_OF(List_GetFront(&self->readyFiberListHead), struct Fiber
                                           , listItem);
        ListItem_Remove(&fiber->listItem);
        Scheduler_RunFiber(self, fiber);
    }
}


void
Scheduler_ResumeFiber(struct Scheduler *self, struct Fiber *fiber)
{
    assert(self != NULL);
    List_InsertBack(&self->readyFiberListHead, &fiber->listItem);
}


void
Scheduler_ExitCurrentFiber(struct Scheduler *self)
{
    assert(self != NULL && self->runningFiber != NULL);
    List_InsertBack(&self->deadFiberListHead, &self->runningFiber->listItem);
    --self->fiberCount;

    if (List_IsEmpty(&self->readyFiberListHead)) {
        Scheduler_Return(self);
    } else {
        struct Fiber *fiber = CONTAINER_OF(List_GetFront(&self->readyFiberListHead), struct Fiber
                                           , listItem);
        ListItem_Remove(&fiber->listItem);
        Scheduler_RunFiber(self, fiber);
    }
}


void
Scheduler_Tick(struct Scheduler *self)
{
    assert(self != NULL && self->runningFiber == NULL);

    if (List_IsEmpty(&self->readyFiberListHead)) {
        return;
    }

    jmp_buf context;

    if (setjmp(context) == 0) {
        self->context = &context;
        struct Fiber *fiber = CONTAINER_OF(List_GetFront(&self->readyFiberListHead), struct Fiber
                                           , listItem);
        ListItem_Remove(&fiber->listItem);
        Scheduler_RunFiber(self, fiber);
    } else {
        struct ListItem *fiberListItem = List_GetBack(&self->deadFiberListHead);

        if (fiberListItem == &self->deadFiberListHead) {
            return;
        }

        do {
            struct Fiber *fiber = CONTAINER_OF(fiberListItem, struct Fiber, listItem);
            fiberListItem = ListItem_GetPrev(fiberListItem);
            FreeFiber(fiber);
        } while (fiberListItem != &self->deadFiberListHead);

        List_Initialize(&self->deadFiberListHead);
    }
}


static void
Scheduler_RunFiber(struct Scheduler *self, struct Fiber *fiber)
{
    self->runningFiber = fiber;

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
            : "r"(fiber->stack), "r"(self), "r"(fiber), "r"(Scheduler_FiberStart)
            : "memory"
#elif defined __x86_64__
            "movq\t$0, %%rbp\n\t"
            "movq\t%0, %%rsp\n\t"
            "pushq\t$0\n\t"
            "jmpq\t*%3"
            :
            : "r"(fiber->stack), "D"(self), "S"(fiber), "r"(Scheduler_FiberStart)
            : "memory"
#else
#   error architecture not supported
#endif
        );
    } else {
        longjmp(*fiber->context, 1);
    }
}


static void
Scheduler_FiberStart(struct Scheduler *self, struct Fiber *fiber)
{
    fiber->coroutine(fiber->argument);
    Scheduler_ExitCurrentFiber(self);
}


static void
Scheduler_Return(struct Scheduler *self)
{
    self->runningFiber = NULL;
    longjmp(*self->context, 1);
}


static struct Fiber *
AllocateFiber(void)
{
    struct Fiber *fiber = malloc(FIBER_SIZE);

    if (fiber == NULL) {
        return NULL;
    }

#if defined __i386__ || defined __x86_64__
    fiber = (struct Fiber *)((unsigned char *)fiber + FIBER_SIZE - sizeof *fiber);
    fiber->stack = fiber;
#else
#   error architecture not supported
#endif
    return fiber;
}


static void
FreeFiber(struct Fiber *fiber)
{
#if defined __i386__ || defined __x86_64__
    fiber = (struct Fiber *)((unsigned char *)fiber + sizeof *fiber - FIBER_SIZE);
#else
#   error architecture not supported
#endif
    free(fiber);
}
