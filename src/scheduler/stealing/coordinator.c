#include "coordinator.h"

Coordinator* CoordinatorNew(size_t nThreads) {
    Coordinator* coordinator = (Coordinator*)calloc(1, sizeof(Coordinator));

    coordinator->threads = nThreads;

    pthread_mutex_init(&coordinator->mutex, NULL);

    return coordinator;
}

void CoordinatorNotifyNewTask(Coordinator* self) {
    if (CoordinatorHasAvailableIdles(self)) {
        CoordinatorWakeOne(self);
    }
}


bool CoordinatorStartSpinning(Coordinator* self) {
    uint64_t tupleCopy = atomic_load(&self->idleAndSpinning);

    while (true) {
        if ((getSpinning(tupleCopy) + 1) * 2 > self->threads) {
            return false;
        }

        uint64_t oneMoreSpinning = tupleCopy + oneSpinning;

        if (atomic_compare_exchange_weak(&self->idleAndSpinning, &tupleCopy, oneMoreSpinning)) {
            return true;
        }
    }
}

/**
 * Returns true if last spinning stopped
*/
bool CoordinatorStopSpinning(Coordinator* self) {
    uint64_t old = atomic_fetch_sub(&self->idleAndSpinning, oneSpinning);
    return getSpinning(old) == 1;
}

/**
 * Routine to anounce worker as idle
*/
void CoordinatorRegisterWorkerAsIdle(Coordinator* self, Worker* willIdle) {
    pthread_mutex_lock(&self->mutex);

    IntrusiveLinkedListPushBack(&self->idleWorkers, willIdle);

    coordinatorIncreaseIdleCount(self);

    pthread_mutex_unlock(&self->mutex);
}

void CoordinatorRegisterWorkerAsActive(Coordinator* self, Worker* activated) {
    pthread_mutex_lock(&self->mutex);

    IntrusiveNode* tag = (IntrusiveNode*)activated;

    if (IntrusiveNodeIsLinked(tag)) {
        IntrusiveNodeUnlink(tag);
        coordinatorDecreaseIdleCount(self);
    }

    pthread_mutex_unlock(&self->mutex);
}

bool CoordinatorHasAvailableIdles(Coordinator* self) {
    uint64_t cpy = atomic_load(&self->idleAndSpinning);

    return getIdle(cpy) > 0 && getSpinning(cpy) == 0;
}

/**
 * Wake and remove one worker from idle list safely
*/
void CoordinatorWakeOne(Coordinator *self) {
    pthread_mutex_lock(&self->mutex);

    Worker* worker = IntrusiveLinkedListPopBack(&self->idleWorkers);

    if (worker) {
        WorkerWake(worker);

        coordinatorDecreaseIdleCount(self);
    }

    pthread_mutex_unlock(&self->mutex);
}

void CoordinatorWakeAll(Coordinator *self) {
    pthread_mutex_lock(&self->mutex);

    Worker* worker;

    while (!IntrusiveLinkedListIsEmpty(&self->idleWorkers)) {
        worker = IntrusiveLinkedListPopBack(&self->idleWorkers);
        WorkerWake(worker);
        coordinatorDecreaseIdleCount(self);
    }

    pthread_mutex_unlock(&self->mutex);
}

void CoordinatorStopSignal(Coordinator* self) {
    atomic_store(&self->stoped, true);
    CoordinatorWakeAll(self);
}

bool CoordinatorIsStopSignaled(Coordinator* self) {
    return atomic_load(&self->stoped);
}

void coordinatorIncreaseIdleCount(Coordinator *self) {
    atomic_fetch_add(&self->idleAndSpinning, oneIdle);
}

void coordinatorDecreaseIdleCount(Coordinator *self) {
    atomic_fetch_sub(&self->idleAndSpinning, oneIdle);
}