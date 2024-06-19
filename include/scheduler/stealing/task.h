#include "scheduler/utils/list.h"
#include "bouncer.h"
#include <stdbool.h>
#include <stdatomic.h>

#pragma once

extern _Atomic int counter;
/* Task */

// lifetime: from allocation in client_proto_coordinator to delete unpacking in WorkerWork
// TODO: possibly may avoid this allocation
PgPacketWrapper* PgPacketWrapperNew(SBuf *sbuf, SBufEvent evtype, struct MBuf *data);
void PgPacketWrapperDelete(PgPacketWrapper* self);


typedef struct SchedulerTask {
    IntrusiveNode node;
    bool externalTask;
    void* task;  // client
} SchedulerTask;

SchedulerTask* SchedulerTaskNew(SBuf *sbuf, SBufEvent evtype, struct MBuf *data);
void SchedulerTaskDelete(SchedulerTask* self);

void SchedulerTaskRun(SchedulerTask* self, void(*runner)(void*));

void initTestRunner(void);

int testResult(void);

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

