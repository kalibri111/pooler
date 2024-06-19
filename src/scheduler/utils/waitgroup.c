#include "scheduler/utils/waitgroup.h"
#include <stdio.h>

WaitGroup* WaitGroupNew(void) {
    WaitGroup* wg = (WaitGroup*)calloc(1, sizeof(WaitGroup));
    wg->count = 0;
    pthread_mutex_init(&wg->mutex, NULL);
    pthread_cond_init(&wg->isIdle, NULL);

    return wg;
}

void WaitGroupDelete(WaitGroup* self) {
    free(self);
    self = NULL;
}

void WaitGroupAdd(WaitGroup* self, size_t count) {
    atomic_fetch_add(&self->count, count);
}

void WaitGroupDone(WaitGroup* self, size_t count) {
    printf("done notify\n");
    fflush(stdout);
    // if all done
    if (atomic_fetch_sub(&self->count, count) == count) {
        pthread_mutex_lock(&self->mutex);
        pthread_cond_broadcast(&self->isIdle);
        pthread_mutex_unlock(&self->mutex);
    }

}

void WaitGroupWaitIdle(WaitGroup* self) {
    pthread_mutex_lock(&self->mutex);

    printf("waiting for done: %d remaining\n", self->count);
    fflush(stdout);
    while (atomic_load(&self->count) > 0) {
        pthread_cond_wait(&self->isIdle, &self->mutex);
    }

    printf("all workers done...\n");
    fflush(stdout);

    pthread_mutex_unlock(&self->mutex);
}