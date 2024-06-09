#ifndef PGBOUNCER_ADDR_MAP_H
#define PGBOUNCER_ADDR_MAP_H

#include "scheduler/stealing/worker.h"
#include <stddef.h>

#define BUCKET_COUNT 1024

typedef struct AddrMap {
    Worker *addrToWorker[BUCKET_COUNT];
} AddrMap;

void AddrMapInit(AddrMap* self);

Worker *AddrMapGetWorker(AddrMap *self, uintptr_t addr);

void AddrMapSetWorker(AddrMap *self, uintptr_t addr, Worker *worker);


#endif //PGBOUNCER_ADDR_MAP_H
