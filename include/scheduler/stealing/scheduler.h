/**
 * Scalable work-stealing scheduler
 */

#include "queues/global.h"
#include "queues/threadlocal.h"
#include "coordinator.h"
#include "worker.h"
#include "waitgroup.h"

#ifndef SCHED_SCHEDULER_H
#define SCHED_SCHEDULER_H

typedef struct Scheduler {
    // global queue with contention
    // local queues relies to compliment worker
    GlobalQueue* globalTasks;

    // stealing logic
    Coordinator* coordinator;
    size_t nThreads;
    Worker* workers;
    WaitGroup* workersCount;

    /* Need to check submitter to determine local or external push */
    // pthread_key_t threadLocalWorker;  // current execution worker
} Scheduler;

typedef void(*Runner)(void*);

Scheduler* SchedulerNew(size_t nWorkers, Runner);

/**
 * Start workers execution concurrently
*/
void SchedulerStart(Scheduler* self);

/**
 * Submit new task. Current restriction: only one Scheduler allowed
*/
void SchedulerSubmit(Scheduler* self, SchedulerTask* task);

void SchedulerWaitIdle(Scheduler* self);
/**
 * Wait for all workers done. Signal workers stop picking new tasks.
*/
void SchedulerStop(Scheduler* self);

Worker* SchedulerGetWorker(Scheduler* self, size_t index);

// void SchedulerInWorker(Scheduler* self, Worker* current) {
//     pthread_setspecific(self->threadLocalWorker, current);
// }

#endif //SCHED_SCHEDULER_H
