#include <stdio.h>
#include "global.h"

#ifndef SCHED_GLOBAL_C
#define SCHED_GLOBAL_C


GlobalQueue* GlobalQueueNew() {
    GlobalQueue* queue = (GlobalQueue*)calloc(1, sizeof(GlobalQueue));

    pthread_mutex_init(&queue->mutex, NULL);

    queue->tasks = IntrusiveListNew();

    queue->test = 0;

    return queue;
}

void GlobalQueueDelete(GlobalQueue* self) {
    IntrusiveListDelete(self->tasks);
    free(self);
    self = NULL;
}

/**
 * Entrypoint for:
 *  - Suspended coroutines
 */
void GlobalQueuePushOne(GlobalQueue* self, SchedulerTask* item) {
    pthread_mutex_lock(&self->mutex);
    self->test += 1;
    assert(self->test < 2);
    IntrusiveListPushBack(self->tasks, (void*)item);
    self->test -= 1;
    pthread_mutex_unlock(&self->mutex);
}

/**
 * GlobalQueueSteal used by Worker to get more tasks.
 * Safely appends tasks to local queue:
 *  at least 1, at most min(size of global queue / workers count, local queue capacity)
 */
void GlobalQueuePushMany(GlobalQueue* self, ArrayView* local) {
    pthread_mutex_lock(&self->mutex);

    IntrusiveList* asList = IntrusiveListNew();

    for (int i = 0; i < ArrayViewLength(local); ++i) {
        IntrusiveListPushBack(asList, ArrayViewAt(local, i));
    }

    IntrusiveListConcat(self->tasks, asList);

    pthread_mutex_unlock(&self->mutex);
}

void* GlobalQueuePop(GlobalQueue* self) {
    pthread_mutex_lock(&self->mutex);
    void* item = IntrusiveListPopFront(self->tasks);
    pthread_mutex_unlock(&self->mutex);
    return item;
}

size_t GlobalQueuePopMany(GlobalQueue* self, ArrayView* to, size_t nWorkers) {
    pthread_mutex_lock(&self->mutex);

    if (!IntrusiveListSize(self->tasks)) {
        pthread_mutex_unlock(&self->mutex);
        return 0;
    }

    size_t share = IntrusiveListSize(self->tasks) / nWorkers;

    if (share == 0)
        share = 1;
    
    if (share > ArrayViewLength(to))
        share = ArrayViewLength(to);
    
    size_t idx = 0;
    while (share > 0) {
        SchedulerTask* task = (SchedulerTask*)IntrusiveListPopFront(self->tasks);

        if (!task)
            break;
        
        share -= 1;
        *(SchedulerTask*)ArrayViewAt(to, idx) = *task;
        idx += 1;
    }
    pthread_mutex_unlock(&self->mutex);

    return idx;
}

#endif //SCHED_GLOBAL_C
