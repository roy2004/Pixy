#include <assert.h>
#include <errno.h>
#include <string.h>

#include "Base.h"
#include "Logging.h"
#include "Scheduler.h"
#include "IOPoller.h"
#include "Timer.h"
#include "ThreadPool.h"
#include "Async.h"


int Main(int argc, char **argv);

static void MainWrapper(any_t);
static void Loop(void);


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

    if (!ThreadPool_Initialize(&ThreadPool, &IOPoller)) {
        assert(errno == ENOMEM);
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

    Scheduler_CallCoroutine(&Scheduler, MainWrapper, (any_t)&context);
    Loop();
    ThreadPool_Stop(&ThreadPool);
    ThreadPool_Finalize(&ThreadPool);
    Scheduler_Finalize(&Scheduler);
    IOPoller_Finalize(&IOPoller);
    Timer_Finalize(&Timer);
    return context.status;
}


static void
MainWrapper(any_t argument)
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
}
