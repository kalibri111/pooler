#include "list.h"
#include <stdbool.h>
#include <stdatomic.h>

#ifndef SCHED_TASK_HPP
#define SCHED_TASK_HPP
extern _Atomic int counter;
/* Task */

typedef struct SchedulerTask {
    IntrusiveNode node;
    bool externalTask;
    void* task;  // coroutine
} SchedulerTask;

void SchedulerTaskRun(SchedulerTask* self, void(*runner)(void*));

void initTestRunner();

int testResult();

void testRunner(void* arg);

/* Array view */

typedef struct ArrayView {
    void* ptr_;
    size_t size_;  // sizof item
    size_t len_;  // items count
} ArrayView;

ArrayView* ArrayViewNew(void* ptr, size_t size, size_t len);

void ArrayViewDelete(ArrayView* self);

void* ArrayViewAt(ArrayView* self, size_t index);

size_t ArrayViewLength(ArrayView* self);

void ArrayViewReindex(ArrayView* self, size_t shift);


#endif //SCHED_TASK_HPP
