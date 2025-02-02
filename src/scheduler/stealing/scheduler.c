#include "scheduler/stealing/scheduler.h"
#include "bouncer.h"

Scheduler* SchedulerNew(size_t nWorkers, void(*runner)(void*)) {
    Scheduler* sched = (Scheduler*)calloc(1, sizeof(Scheduler));

    sched->nThreads = nWorkers;

    sched->coordinator = CoordinatorNew(nWorkers);
    sched->globalTasks = GlobalQueueNew();
    sched->workersCount = WaitGroupNew();
    sched->workers = WorkerPack(sched, nWorkers, runner);

    AddrMapInit(&sched->clientToWorker);

    // pthread_key_create(&sched->threadLocalWorker, NULL);

    return sched;
}

/**
 * Start workers execution concurrently
*/
void SchedulerStart(Scheduler* self) {
    for (size_t i = 0; i < self->nThreads; i++) {
        WorkerStart(self->workers + i);
    }
}

/**
 * Submit new task. Current restriction: only one Scheduler allowed
*/
void SchedulerSubmit(Scheduler* self, SchedulerTask* task) {
    uintptr_t clientPtr = (uintptr_t)task->task;  // PgSocket*

    // clients request routed to his worker
    Worker* dst = AddrMapGetWorker(&self->clientToWorker, clientPtr);
    if (dst) {
        ThreadLocalQueueTryPush(&dst->localQueue, task);
        return;
    }

    GlobalQueuePushOne(self->globalTasks, task);
    CoordinatorNotifyNewTask(self->coordinator);
}

void SchedulerWaitIdle(Scheduler* self) {
    printf("waiting all done...\n");
    fflush(stdout);
    WaitGroupWaitIdle(self->workersCount);
}

/**
 * Wait for all workers done. Signal workers stop picking new tasks.
*/
void SchedulerStop(Scheduler* self) {
    CoordinatorStopSignal(self->coordinator);

    for (size_t i = 0; i < self->nThreads; i++) {
        WorkerJoin(&self->workers[i]);
    }
}


Worker* SchedulerGetWorker(Scheduler* self, size_t index) {
    assert(index < self->nThreads);

    return self->workers + index;
}