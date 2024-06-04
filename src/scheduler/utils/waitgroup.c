#include "waitgroup.h"

WaitGroup* WaitGroupNew() {
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
    // if all done
    if (atomic_fetch_sub(&self->count, count) == count) {
        pthread_mutex_lock(&self->mutex);
        pthread_cond_broadcast(&self->isIdle);
        pthread_mutex_unlock(&self->mutex);
    }

}

void WaitGroupWaitIdle(WaitGroup* self) {
    pthread_mutex_lock(&self->mutex);

    while (atomic_load(&self->count) > 0) {
        pthread_cond_wait(&self->isIdle, &self->mutex);
    }

    pthread_mutex_unlock(&self->mutex);
}