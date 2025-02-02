#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdatomic.h>
#include <memory.h>
#include <stdlib.h>

#pragma once

typedef struct WaitGroup {
    _Atomic size_t count;
    pthread_mutex_t mutex;
    pthread_cond_t isIdle;
} WaitGroup;

WaitGroup* WaitGroupNew(void);

void WaitGroupDelete(WaitGroup* self);

void WaitGroupAdd(WaitGroup* self, size_t count);

void WaitGroupDone(WaitGroup* self, size_t count);

void WaitGroupWaitIdle(WaitGroup* self);