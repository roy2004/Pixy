#include "Runtime.h"

#include <errno.h>
#include <assert.h>

#include "Scheduler.h"
#include "IOPoller.h"
#include "Timer.h"
#include "Async.h"


int Main(int, char **);

static void SleepCallback(any_t);
static void MainWrapper(any_t);


struct Scheduler Scheduler;
struct IOPoller IOPoller;
struct Timer Timer;


bool
Call(void (*coroutine)(any_t), any_t argument)
{
    if (coroutine == NULL) {
        errno = EINVAL;
        return false;
    }

    return Scheduler_CallCoroutine(&Scheduler, coroutine, argument);
}


void
Yield(void)
{
    Scheduler_YieldCurrentFiber(&Scheduler);
}


bool
Sleep(int duration)
{
    struct Timeout timeout;

    if (!Timer_SetTimeout(&Timer, &timeout, duration, (any_t)Scheduler_GetCurrentFiber(&Scheduler)
                          , SleepCallback)) {
        return false;
    }

    Scheduler_SuspendCurrentFiber(&Scheduler);
    return true;
}


int
main(int argc, char **argv)
{
    Scheduler_Initialize(&Scheduler);
    IOPoller_Initialize(&IOPoller);
    Timer_Initialize(&Timer);
    struct Async async;
    Async_Initialize(&async);

    struct {
        int argc;
        char **argv;
        int status;
    } data = {
        .argc = argc,
        .argv = argv
    };

    Scheduler_CallCoroutine(&Scheduler, MainWrapper, (any_t)&data);

    for (;;) {
        Scheduler_Tick(&Scheduler);

        if (Scheduler_GetFiberCount(&Scheduler) == 0) {
            break;
        }

        bool ok;

        do {
            ok = IOPoller_Tick(&IOPoller, Timer_CalculateWaitTime(&Timer), &async);
        } while (!ok && errno == EINTR);

        if (!ok) {
            assert(errno == ENOMEM);
            // TODO
        }

        Async_DispatchCalls(&async);

        if (!Timer_Tick(&Timer, &async)) {
            assert(errno == ENOMEM);
            // TODO
        }

        Async_DispatchCalls(&async);
    }

    Async_Finalize(&async);
    Scheduler_Finalize(&Scheduler);
    IOPoller_Finalize(&IOPoller);
    Timer_Finalize(&Timer);
    return data.status;
}


static void
MainWrapper(any_t argument)
{
    struct {
        int argc;
        char **argv;
        int status;
    } *data = (void *)argument;

    data->status = Main(data->argc, data->argv);
}


static void
SleepCallback(any_t argument)
{
    Scheduler_ResumeFiber(&Scheduler, (struct Fiber *)argument);
}
