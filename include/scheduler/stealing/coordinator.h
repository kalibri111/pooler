/**
 * Spinning thread stealing now
 */

#include <stdatomic.h>
#include <stdint.h>
#include <pthread.h>

#include "list.h"
#include "worker.h"

#ifndef SCHED_COORDINATOR_H
#define SCHED_COORDINATOR_H

/**
 * Storing two int32 as one uint64 atomic
 * saves store/load calls twice
*/
#define getIdle(source) (uint32_t)((source) >> 32)
#define getSpinning(source) (uint32_t)((source)& (((uint64_t)1 << 32) - 1))
#define oneIdle (uint64_t)1 << 32
#define oneSpinning 1


typedef struct Coordinator {
    _Atomic (uint64_t) idleAndSpinning;  // tuple of (uint32_t idle, uint32_t spinning)
    _Atomic (bool) stoped;
    pthread_mutex_t mutex;
    IntrusiveLinkedList idleWorkers;
    size_t threads;
} Coordinator;

Coordinator* CoordinatorNew(size_t nThreads);

void CoordinatorNotifyNewTask(Coordinator* self);

bool CoordinatorStartSpinning(Coordinator* self);

/**
 * Returns true if last spinning stopped
*/
bool CoordinatorStopSpinning(Coordinator* self);

/**
 * Routine to anounce worker as idle
*/
void CoordinatorRegisterWorkerAsIdle(Coordinator* self, Worker* willIdle);

void CoordinatorRegisterWorkerAsActive(Coordinator* self, Worker* activated);

bool CoordinatorHasAvailableIdles(Coordinator* self);

/**
 * Wake and remove one worker from idle list safely
*/
void CoordinatorWakeOne(Coordinator *self);

void CoordinatorWakeAll(Coordinator *self);

void CoordinatorStopSignal(Coordinator* self);

bool CoordinatorIsStopSignaled(Coordinator* self);

void coordinatorIncreaseIdleCount(Coordinator *self);

void coordinatorDecreaseIdleCount(Coordinator *self);

#endif //SCHED_COORDINATOR_H
