/*
 * Single contention point. Multi producers/multi consumers queue.
 * Extends:
 *  - Scheduling new arrived coroutines
 *  triggers spilling half of overloaded thread local queue
 *  - Scheduling suspended coroutines
 * Shrinks:
 *  - Worker grab tasks from here when not enough
 *
 *                               Dataflow
 * threadlocal overflow ---\                     /--> threadlocal steals
 *      ...                 ---> global queue ---       ...
 * threadlocal overflow ---/                     \--> threadlocal steals
 */

#include <pthread.h>

#include "list.h"
#include "task.h"

#ifndef SCHED_GLOBAL_H
#define SCHED_GLOBAL_H

// types
typedef struct GlobalQueue GlobalQueue;

struct GlobalQueue {
    IntrusiveList *tasks;// intrusive list of workers
    pthread_mutex_t mutex; // mutex - single contention point
    int test;
};

// methods
GlobalQueue* GlobalQueueNew();

void GlobalQueueDelete(GlobalQueue* self);

/**
 * Entrypoint for:
 *  - Suspended coroutines
 */
void GlobalQueuePushOne(GlobalQueue* self, SchedulerTask* item);

/**
 * GlobalQueueSteal used by Worker to get more tasks.
 * Safely appends tasks to local queue:
 *  at least 1, at most min(size of global queue / workers count, local queue capacity)
 */
void GlobalQueuePushMany(GlobalQueue* self, ArrayView* local);

void* GlobalQueuePop(GlobalQueue* self);

size_t GlobalQueuePopMany(GlobalQueue* self, ArrayView* to, size_t nWorkers);

#endif //SCHED_GLOBAL_H
