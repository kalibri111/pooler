#include <stdio.h>
#include "task.h"

_Atomic int counter;

/* Task */

void SchedulerTaskRun(SchedulerTask* self, void(*runner)(void*)) {
    runner(self->task);
}

void testRunner(void* arg) {
    atomic_fetch_add(&counter, 1);
}

void initTestRunner() {
    atomic_store(&counter, 0);
}

int testResult() {
    return atomic_load(&counter);
}

/* ArrayView */

ArrayView* ArrayViewNew(void* ptr, size_t size, size_t len) {
    ArrayView* arr = (ArrayView*)calloc(1, sizeof(ArrayView));
    arr->len_ = len;
    arr->ptr_ = ptr;
    arr->size_ = size;
    return arr;
}

void ArrayViewDelete(ArrayView* self) {
    assert(self);

    free(self->ptr_);
    self->ptr_ = NULL;
    free(self);
    self = NULL;
}

void* ArrayViewAt(ArrayView* self, size_t index) {
    assert(self);
//    printf("array view at: [len: %d, index: %d]\n", self->len_, index);
//    fflush(stdout);
    assert(index < self->len_ && index >= 0);

    return (char*)self->ptr_ + self->size_ * index;
}

size_t ArrayViewLength(ArrayView* self) {
    return self->len_;
}

void ArrayViewReindex(ArrayView* self, size_t shift) {
    self->ptr_ = (char*)self->ptr_ + self->size_ * shift;
    self->len_ -= shift;
}