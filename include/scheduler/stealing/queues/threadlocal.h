/*
 * Distributed by threads single producer/multi consumer queue.
 * Queue implemented as ring buffer of pointers to coroutine.
 * Extends:
 *  - Worker grabs tasks from global queue
 *  - Worker steals tasks from other local queues
 * Shrinks:
 *  - Other worker steals tasks
 *  - Half of queue spill to global queue due to overload
 *
 *                          Dataflow
 *  stealing
 *    or                                       /---> other local queue steals
 *  extending from global ---> threadlocal -- /             ...
 *      |                                     \---> other local queue steals
 *      | ---> spills half to global queue
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdatomic.h>

#include "task.h"

#define THREAD_LOCAL_QUEUE_CAPACITY 100

#ifndef SCHED_THREADLOCAL_H
#define SCHED_THREADLOCAL_H

typedef struct SchedulerTaskAtomicPointer {
    _Atomic(SchedulerTask*) task;
} SchedulerTaskAtomicPointer;

typedef struct ThreadLocalQueue {
    atomic_size_t head;
    atomic_size_t tail;
    SchedulerTaskAtomicPointer ringBuffer[100];
} ThreadLocalQueue;

void ThreadLocalQueueInit(ThreadLocalQueue* self);

bool ThreadLocalQueueIsEmpty(ThreadLocalQueue* self);

bool ThreadLocalQueueSnapshotIsEmpty(size_t head, size_t tail);

bool ThreadLocalQueueSnapshotIsFull(size_t head, size_t tail);

size_t ThreadLocalQueueRingIndex(size_t index);


/* Single producer push */

// Should always succeed
void ThreadLocalQueueMultiPush(ThreadLocalQueue* self, ArrayView* tasks);

// Push allowed if free space exist
bool ThreadLocalQueueTryPush(ThreadLocalQueue* self, SchedulerTask* task);

/* Multi consumer pop */

// Pop allowed if queue not empty
// lockfree try
SchedulerTask* ThreadLocalQueueTryPop(ThreadLocalQueue* self);

/**
 * Tries to move tasks from queue to fill ArrayView.
 * Returns count of moved tasks.
 */
size_t ThreadLocalQueueTryMultiPop(ThreadLocalQueue* self, /*ArrayView<SchedulerTask>*/ArrayView* to);

size_t ThreadLocalQueueSizeAtMost(ThreadLocalQueue* self);



#endif //SCHED_THREADLOCAL_H
