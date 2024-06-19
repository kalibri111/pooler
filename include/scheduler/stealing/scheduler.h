/**
 * Scalable work-stealing scheduler
 */
#ifndef SCHED_SCHEDULER_H
#define SCHED_SCHEDULER_H

#include "scheduler/stealing/queues/global.h"
#include "scheduler/stealing/queues/threadlocal.h"
#include "scheduler/stealing/coordinator.h"
#include "scheduler/stealing/worker.h"
#include "scheduler/utils/waitgroup.h"
#include "scheduler/utils/addr_map.h"
#include "scheduler/utils/addr_map.h"

typedef struct Scheduler {
    // global queue with contention
    // local queues relies to compliment worker
    GlobalQueue* globalTasks;

    // stealing logic
    Coordinator* coordinator;
    size_t nThreads;
    Worker* workers;
    WaitGroup* workersCount;
    AddrMap clientToWorker;

    /* Need to check submitter to determine local or external push */
    // pthread_key_t threadLocalWorker;  // current execution worker
} Scheduler;

typedef void(*Runner)(void*);

extern Scheduler* scheduler;

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
