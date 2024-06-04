/**
 * In-thread scheduler + task runner + worker
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>

#include "queues/threadlocal.h"
#include "task.h"
#include "park.h"


#ifndef SCHED_WORKER_H
#define SCHED_WORKER_H

#define STEALING_FACTOR 7
#define STEALING_ATTEMPTS 4
#define ORDER_SHUFFLE_FACTOR 61
#define GLOBAL_BANCH_MIN 48

/* In-thread scheduler */

typedef struct InThreadScheduler {

} InThreadScheduler;

void InThreadSchedulerStop(InThreadScheduler* self);

/**
 * true if stop was requested
 */
bool InThreadSchedulerIsStopped(InThreadScheduler* self);

/**
 * Try to:
 *  - get task from local queue
 *  - get tasks chunk from global queue
 *  - steal tasks chunk from other locals
 * Spinning wait for task otherwise
 */
SchedulerTask* InThreadSchedulerNextTask(InThreadScheduler* self);

/* Runner in thread */

void ThreadRun(InThreadScheduler* scheduler);

/* Worker - thread owning object, associated with scheduler */

typedef struct Scheduler Scheduler;

typedef struct Worker {
    ThreadLocalQueue localQueue;// local queue
    pthread_t thread;  // ID returned by pthread_create()
    SchedulerTask transferBuffer[THREAD_LOCAL_QUEUE_CAPACITY];  // transfer buffer
    size_t iteration;  // iteration number
    size_t index;  // worker index
    Scheduler* host;  // host threadpool
    InThreadScheduler localScheduler;
    Park park;
    void(*runner)(void*);
} Worker;

Worker* WorkerNew(Scheduler* host, size_t index, void(*runner)(void*)); // +

void WorkerInit(Worker* self, Scheduler* host, size_t index, void(*runner)(void*));

/**
 * Try to pick task from worker, park if fails
 */
SchedulerTask* WorkerPickTask(Worker* self);

Worker* WorkerPack(Scheduler* host, size_t nWorkers, void(*runner)(void*));

/**
 * Starts new execution in separate own thread
 */
void WorkerStart(Worker* self); // +

void WorkerStop(Worker* self);

/**
 * Join associated with self thread
 */
void WorkerJoin(Worker* self); // +

/**
 * Pick and starts available tasks via Runner
 */
void WorkerWork(Worker* self); // +

void* workerPthreadWork(void* arg);

/**
 * Single producer routine for add new task
 */
void WorkerPush(Worker* self, SchedulerTask* task); // +

/**
 * Wake parked worker.
 * Worker parks in case of ... TODO
 */
void WorkerWake(Worker* self);  // +

/* private */

/**
 * Spill half of local queue to global in case of newly arrived task
 * overflows local queue.
 */
void workerSpillToGlobalQueue(Worker* self, SchedulerTask* arrived);  // +

/**
 * Pick task from queues in desired order
 */
SchedulerTask* workerTryPickTask(Worker* self);

/**
 * Lock-free stealing from other local queue to self.transferBuffer
 */
SchedulerTask* workerTryStealTask(Worker* self, size_t nTries); // +

/**
 * Synchronized grab tasks from global queue and returns new top
 */
SchedulerTask* workerTakeFromGlobal(Worker* self);

/**
 * Take task directly from global queue only in shutdown(?)
 */
SchedulerTask* workerTryPickTaskFromGlobalQueue(Worker* self);  // +

/**
 * Try to take task from:
 *  - global queue directly
 *  - steal from other locals
 */
SchedulerTask* workerTryPickTaskBeforeShutdown(Worker* self);
#endif //SCHED_WORKER_H
