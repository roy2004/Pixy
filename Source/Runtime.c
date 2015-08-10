#include "Runtime.h"

#include <errno.h>
#include <string.h>
#include <stddef.h>

#include "Scheduler.h"
#include "IOPoller.h"
#include "Timer.h"
#include "ThreadPool.h"
#include "Async.h"
#include "Logging.h"


static void MainWrapper(uintptr_t);
static void Loop(void);
static void SleepCallback(uintptr_t);


struct Scheduler Scheduler;
struct IOPoller IOPoller;
struct Timer Timer;
struct ThreadPool ThreadPool;


int
main(int argc, char **argv)
{
    Scheduler_Initialize(&Scheduler);
    IOPoller_Initialize(&IOPoller);
    Timer_Initialize(&Timer);

    if (ThreadPool_Initialize(&ThreadPool, &IOPoller) < 0) {
        LOG_FATAL_ERROR("`ThreadPool_Initialize()` failed: %s", strerror(errno));
    }

    ThreadPool_Start(&ThreadPool);

    struct {
        int argc;
        char **argv;
        int status;
    } context = {
        .argc = argc,
        .argv = argv
    };

    Scheduler_CallCoroutine(&Scheduler, MainWrapper, (uintptr_t)&context);
    Loop();
    ThreadPool_Stop(&ThreadPool);
    ThreadPool_Finalize(&ThreadPool);
    Scheduler_Finalize(&Scheduler);
    IOPoller_Finalize(&IOPoller);
    Timer_Finalize(&Timer);
    return context.status;
}


int
Call(void (*coroutine)(uintptr_t), uintptr_t argument)
{
    if (coroutine == NULL) {
        errno = EINVAL;
        return -1;
    }

    return Scheduler_CallCoroutine(&Scheduler, coroutine, argument);
}


void
Yield(void)
{
    Scheduler_YieldCurrentFiber(&Scheduler);
}


void
Exit(void)
{
    Scheduler_ExitCurrentFiber(&Scheduler);
}


int
Sleep(int duration)
{
    struct Timeout timeout;

    if (Timer_SetTimeout(&Timer, &timeout, duration
                         , (uintptr_t)Scheduler_GetCurrentFiber(&Scheduler), SleepCallback) < 0) {
        return -1;
    }

    Scheduler_SuspendCurrentFiber(&Scheduler);
    return 0;
}


static void
MainWrapper(uintptr_t argument)
{
    struct {
        int argc;
        char **argv;
        int status;
    } *context = (void *)argument;

    context->status = Main(context->argc, context->argv);
}


static void
Loop(void)
{
    struct Async async;
    Async_Initialize(&async);

    for (;;) {
        Scheduler_Tick(&Scheduler);

        if (Scheduler_GetFiberCount(&Scheduler) == 0) {
            break;
        }

        int result;

        do {
            result = IOPoller_Tick(&IOPoller, Timer_CalculateWaitTime(&Timer), &async);
        } while (result < 0 && errno == EINTR);

        if (result < 0) {
            LOG_FATAL_ERROR("`IOPoller_Tick()` failed: %s", strerror(errno));
        }

        Async_DispatchCalls(&async);

        if (Timer_Tick(&Timer, &async) < 0) {
            LOG_FATAL_ERROR("`Timer_Tick()` failed: %s", strerror(errno));
        }

        Async_DispatchCalls(&async);
    }

    Async_Finalize(&async);
}


static void
SleepCallback(uintptr_t argument)
{
    Scheduler_ResumeFiber(&Scheduler, (struct Fiber *)argument);
}
