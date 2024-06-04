#include "worker.h"
#include "scheduler.h"

#ifndef SCHED_WORKER_C
#define SCHED_WORKER_C

/* Worker - thread owning object, associated with scheduler */

//Worker* thisThreadWorker;

Worker* WorkerNew(Scheduler* host, size_t index, void(*runner)(void*)) {
    Worker* w = (Worker*)calloc(1, sizeof(Worker));

    WorkerInit(w, host, index, runner);

    return w;
}

void WorkerInit(Worker* self, Scheduler* host, size_t index, void(*runner)(void*)) {
    self->host = host;
    self->index = index;
    self->iteration = 0;
    self->runner = runner;
}

Worker* WorkerPack(Scheduler* host, size_t nWorkers, void(*runner)(void*)) {
    Worker* wPack = (Worker*)calloc(nWorkers, sizeof(Worker));

    for (size_t i = 0; i < nWorkers; i++) {
        WorkerInit(wPack + i, host, i, runner);
    }

    return wPack;
}

/**
 * Starts new execution in separate own thread
 */
void WorkerStart(Worker* self) {
    WaitGroupAdd(self->host->workersCount, 1);

    // SchedulerInWorker(self->host, self);
    
    pthread_create(&self->thread, NULL, workerPthreadWork, self);
}

void WorkerStop(Worker* self) {
    printf("stop worker\n");
    fflush(stdout);
    WaitGroupDone(self->host->workersCount, 1);
}

/**
 * Join associated with self thread
 */
void WorkerJoin(Worker* self) {
    pthread_join(self->thread, NULL);
}

/**
 * Pick and starts available tasks via Runner
 */
void WorkerWork(Worker* self) {
    SchedulerTask* task = WorkerPickTask(self);
    while(task) {
        SchedulerTaskRun(task, self->runner);

        task = WorkerPickTask(self);
        printf("do task %p\n", task);
        fflush(stdout);
    }
    WorkerStop(self);
}

void* workerPthreadWork(void* arg) {
    Worker* self = (Worker*) arg;
    WorkerWork(self);
    pthread_exit(NULL);
}

/**
 * Single producer routine for add new task
 */
void WorkerPush(Worker* self, SchedulerTask* task) {
    /* push to local queue fails in case of overflow,
                                move half to global queue */
    if (!ThreadLocalQueueTryPush(&self->localQueue, task)) {
        workerSpillToGlobalQueue(self, task);
    }
}

/**
 * Wake parked worker.
 * Worker parks in case of ... TODO
 */
void WorkerWake(Worker* self) {
    ParkWake(&self->park);
}

/* private */

/**
 * Spill half of local queue to global in case of newly arrived task
 * overflows local queue.
 */
void workerSpillToGlobalQueue(Worker* self, SchedulerTask* arrived) {
    /* Offload from local queue */
    size_t toSpill = ThreadLocalQueueSizeAtMost(&self->localQueue) / 2;

    // newly arrived task
    toSpill += 1;

    ArrayView buffer = {
        .ptr_ = self->transferBuffer,
        .size_ = sizeof(SchedulerTask),
        .len_ = toSpill
    };

    ThreadLocalQueueTryMultiPop(&self->localQueue, &buffer);

    /* Upload to global queue */
    GlobalQueuePushMany(self->host->globalTasks, &buffer);
    GlobalQueuePushOne(self->host->globalTasks, arrived);
    
}

SchedulerTask* WorkerPickTask(Worker* self) {
    self->iteration += 1;

    while (!CoordinatorIsStopSignaled(self->host->coordinator)) {
        SchedulerTask* task;

        task = workerTryPickTask(self);
        if (task) {
            return task;
        }
        printf("anounce parking\n");
        fflush(stdout);
        /* Anounce parking */
        size_t epoch = ParkAnounce(&self->park);

        CoordinatorRegisterWorkerAsIdle(self->host->coordinator, self);

        printf("try pick task before shutdown\n");
        fflush(stdout);

        /* Recheck condition */
        task = workerTryPickTaskBeforeShutdown(self);
        if (task) {
            CoordinatorRegisterWorkerAsActive(self->host->coordinator, self);
            return task;
        }
        printf("recheck failed\n");
        fflush(stdout);
        if (CoordinatorIsStopSignaled(self->host->coordinator)) {
            CoordinatorRegisterWorkerAsActive(self->host->coordinator, self);
            return NULL;
        }

        printf("worker %p going sleep: %zu\n", self, self->host->workersCount->count);
        fflush(stdout);

        /* Go to sleep */
        WaitGroupDone(self->host->workersCount, 1);
        ParkWait(&self->park, epoch);

        /* Waken up */
        WaitGroupAdd(self->host->workersCount, 1);
    }

    return NULL;
}

/**
 * Pick task from queues in desired order
 */
SchedulerTask* workerTryPickTask(Worker* self) {
    SchedulerTask* task;

    /* Pick task from global queue sometimes */
    if (self->iteration % ORDER_SHUFFLE_FACTOR == 0) {
        task = workerTryPickTaskFromGlobalQueue(self);
        if (task) {
            return task;
        }
    }

    /* Pick task from local queue */
    task = ThreadLocalQueueTryPop(&self->localQueue);
    if (task) {
        return task;
    }

    /* No tasks in local queue, get banch from global */
    task = workerTakeFromGlobal(self);
    if (task) {
        return task;
    }

    /* No tasks in global queue, try to steal from other workers */
    if (CoordinatorStartSpinning(self->host->coordinator)) {
//        task = workerTryStealTask(self, STEALING_ATTEMPTS);
        bool last_spinner = CoordinatorStopSpinning(self->host->coordinator);
        if (task && last_spinner) {
            CoordinatorWakeOne(self->host->coordinator);
        }

        if (task) {
            return task;
        }
        
    }

    /* No tasks at all */
    return NULL;
}

/**
 * Lock-free stealing from other local queue
 */
SchedulerTask* workerTryStealTask(Worker* self, size_t nTries) {
    size_t toSteal = STEALING_FACTOR;

    ArrayView transferBufferView = {
        .ptr_ = self->transferBuffer,
        .size_ = sizeof(SchedulerTask),
        .len_ = STEALING_FACTOR
    };

    /* Steal from randomly chosen workers to transferBuffer */
    for (size_t nTry = 0; nTry < nTries; nTry++) {
        if (toSteal == 0)
                break;

        size_t randomFactor = random();  // TODO: replace with mt19937_64 like
        
        for (size_t nThread = 0; nThread < self->host->nThreads; nThread++) {
            if (toSteal == 0)
                break;

            size_t stealTargetIndex = (randomFactor + nThread) % self->host->nThreads;

            Worker* stealTarget = SchedulerGetWorker(self->host, stealTargetIndex);

            printf("stealing target: %p, self: %p\n", stealTarget, self);
            fflush(stdout);

            if (stealTarget == self)
                continue;

            size_t stolen = ThreadLocalQueueTryMultiPop(&stealTarget->localQueue, &transferBufferView);

            ArrayViewReindex(&transferBufferView, stolen);

            toSteal -= stolen;
        }   
    }

    /* Push stolen to local queue */
    if (toSteal < STEALING_FACTOR) {
        if (STEALING_FACTOR - toSteal > 1) {
            ArrayView withoutFirst = {
                .ptr_ = self->transferBuffer + sizeof(SchedulerTask),
                .size_ = sizeof(SchedulerTask),
                .len_ = STEALING_FACTOR - toSteal - 1
            };
            ThreadLocalQueueMultiPush(&self->localQueue, &withoutFirst);
        }
        return (SchedulerTask *) self->transferBuffer;
        
    } else {
        return NULL;
    }
}

/**
 * Synchronized grab tasks from global queue and returns new top
 */
SchedulerTask* workerTakeFromGlobal(Worker* self) {
    assert(self);
    SchedulerTask* toReturn = NULL;
    
    size_t available = THREAD_LOCAL_QUEUE_CAPACITY - ThreadLocalQueueSizeAtMost(&self->localQueue) / 2;

    size_t grabFromGlobal = available / 2 + 1;

    if (grabFromGlobal < GLOBAL_BANCH_MIN)
        grabFromGlobal = GLOBAL_BANCH_MIN;


    ArrayView buffer = {
        .ptr_ = self->transferBuffer,
        .size_ = sizeof(SchedulerTask),
        .len_ = grabFromGlobal
    };

    size_t taken = GlobalQueuePopMany(self->host->globalTasks, &buffer, self->host->nThreads);

    printf("worker takes %d tasks from global\n", taken);
    fflush(stdout);

    size_t externalsOccured = 0;
    if (taken > 0) {

        for (size_t i = 0; i < taken; i++) {
            /* Worker was done */
            if (self->transferBuffer[i].externalTask) {
                self->transferBuffer[i].externalTask = false;
                externalsOccured += 1;
            }

            WaitGroupDone(self->host->workersCount, externalsOccured);
        }

        printf("worker takes %d external tasks from global\n", externalsOccured);
        fflush(stdout);

        toReturn = &self->transferBuffer[0];

        ArrayView upload = {
            .ptr_ = self->transferBuffer + 1,
            .size_ = sizeof(SchedulerTask),
            .len_ = taken - 1
        };

        ThreadLocalQueueMultiPush(&self->localQueue, &upload);
    }

    return toReturn;
}

/**
 * Take task directly from global queue only in shutdown(?)
 */
SchedulerTask* workerTryPickTaskFromGlobalQueue(Worker* self) {
    printf("global queue pop\n");
    fflush(stdout);
    SchedulerTask* task = GlobalQueuePop(self->host->globalTasks);

    if (!task) {
        return NULL;
    }

    if (task->externalTask) {
        task->externalTask = false;
        WaitGroupDone(self->host->workersCount, 1);
    }

    return task;
}

/**
 * Try to take task from:
 *  - global queue directly
 *  - steal from other locals
 */
SchedulerTask* workerTryPickTaskBeforeShutdown(Worker* self) {
    printf("picking task before shutdown...\n");
    fflush(stdout);
    SchedulerTask* task = workerTryPickTaskFromGlobalQueue(self);
    if (task) {
        return task;
    }

    printf("no task from global queue\n");
    fflush(stdout);

//    task = workerTryStealTask(self, 1);
//    if (task) {
//        return task;
//    }

    return NULL;
}
#endif //SCHED_WORKER_C
