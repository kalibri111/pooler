#include <stdio.h>
#include "threadlocal.h"

#define THREAD_LOCAL_QUEUE_CAPACITY 100

#ifndef SCHED_THREADLOCAL_C
#define SCHED_THREADLOCAL_C

void ThreadLocalQueueInit(ThreadLocalQueue* self) {
    atomic_store(&self->tail, 0);
    atomic_store(&self->head, 0);
}

/* Single producer push */

// Should always succeed
void ThreadLocalQueueMultiPush(ThreadLocalQueue* self, ArrayView* tasks) {

    printf("push %zu tasks to local queue\n", ArrayViewLength(tasks));
    fflush(stdout);

    size_t currentHead = atomic_load(&(self->head));
    size_t currentTail = atomic_load(&(self->tail));

    size_t spaceAwailable = THREAD_LOCAL_QUEUE_CAPACITY - (currentTail - currentHead);

    assert(ArrayViewLength(tasks) <= spaceAwailable);
    assert(ArrayViewLength(tasks) >= 0);
    
    // write after tail
    for (size_t i = 0; i < ArrayViewLength(tasks); i++) {
        atomic_store(
            &self->ringBuffer[ThreadLocalQueueRingIndex(currentTail + i)].task,
            (SchedulerTask*)ArrayViewAt(tasks, i)
        );
    }

    // move tail
//    printf("ThreadLocalQueueMultiPush store tail: %zu\n", currentTail + ArrayViewLength(tasks));
//    fflush(stdout);
    atomic_store(&self->tail, currentTail + ArrayViewLength(tasks));
}

// Push allowed if free space exist
bool ThreadLocalQueueTryPush(ThreadLocalQueue* self, SchedulerTask* task) {
    size_t currentTail = atomic_load(&(self->tail));
    size_t currentHead = atomic_load(&(self->head));

    if (ThreadLocalQueueSnapshotIsFull(currentHead, currentTail)) {
        return false;
    }

    atomic_store(
            &self->ringBuffer[ThreadLocalQueueRingIndex(currentTail + 1)].task,
            task
        );
//    printf("ThreadLocalQueueTryPush store tail: %zu\n", currentTail + 1);
//    fflush(stdout);
    atomic_store(&self->tail, currentTail + 1);
    return true;
}

/* Multi consumer pop */

// Pop allowed if queue not empty
// lockfree try
SchedulerTask* ThreadLocalQueueTryPop(ThreadLocalQueue* self) {
    while (true) {
        size_t currentHead = atomic_load(&(self->head));
        size_t currentTail = atomic_load(&(self->tail));

        if (ThreadLocalQueueSnapshotIsEmpty(currentHead, currentTail)) {
            return NULL;
        }

        printf("ThreadLocalQueueTryPop: [head: %d, tail: %d]\n", currentHead, currentTail);
        fflush(stdout);

        assert(currentHead < currentTail);

        // atomically try to forward head
        // succeeded if local head equals to thread-shared
        // in case of local version stale, save current thread-global
        SchedulerTask* task = atomic_load(&(self->ringBuffer[ThreadLocalQueueRingIndex(currentHead)].task));
        if (atomic_compare_exchange_weak(&self->head, &currentHead, currentHead + 1)) {
            return task;
        }
    }
}

/**
 * Tries to move tasks from queue to fill ArrayView.
 * Returns count of moved tasks.
 */
size_t ThreadLocalQueueTryMultiPop(ThreadLocalQueue* self, /*ArrayView<SchedulerTask>*/ArrayView* to) {
    while (true) {
        size_t currentHead = atomic_load(&(self->head));
        size_t currentTail = atomic_load(&(self->tail));
        printf("ThreadLocalQueueTryMultiPop: [head: %zu, tail: %zu, to.count: %zu]\n", currentHead, currentTail,
               ArrayViewLength(to));
        fflush(stdout);
        size_t popCount;

        if (ThreadLocalQueueSnapshotIsEmpty(currentHead, currentTail)) {
            return 0;
        }

        if (currentHead >= currentTail) {
            printf("assertion currentHead (%zu) < currentTail (%zu)\n", currentHead, currentTail);
            fflush(stdout);
        }
        assert(currentHead < currentTail);

        if (ArrayViewLength(to) > currentHead - currentTail) {
            popCount = ArrayViewLength(to);
        } else {
            popCount = currentHead - currentTail;
        }

        printf("ThreadLocalQueueTryMultiPop %zu tasks...\n", popCount);
        fflush(stdout);

        for (size_t i = 0; i < popCount; i++) {
            // TODO: prettify
            *(SchedulerTask*)ArrayViewAt(to, i) = *atomic_load(&self->ringBuffer[ThreadLocalQueueRingIndex(i)].task);
        }

        if (atomic_compare_exchange_weak(&self->head, &currentHead, currentHead + popCount)) {
            return popCount;
        }
        
    }
    
}

size_t ThreadLocalQueueSizeAtMost(ThreadLocalQueue* self) {
    return atomic_load(&self->tail) - atomic_load(&self->head);
}


/* Private */

bool ThreadLocalQueueIsEmpty(ThreadLocalQueue* self) {
    return self->head == self->tail;
}

bool ThreadLocalQueueSnapshotIsEmpty(size_t head, size_t tail) {
    return head == tail;
}

bool ThreadLocalQueueSnapshotIsFull(size_t head, size_t tail) {
    return tail - head > THREAD_LOCAL_QUEUE_CAPACITY;
}

size_t ThreadLocalQueueRingIndex(size_t index) {
    return index % THREAD_LOCAL_QUEUE_CAPACITY;
}

#endif //SCHED_THREADLOCAL_C
