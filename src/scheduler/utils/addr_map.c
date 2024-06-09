#include "scheduler/utils/addr_map.h"

Worker *AddrMapGetWorker(AddrMap *self, uintptr_t addr) {
    return self->addrToWorker[addr % BUCKET_COUNT];
}

void AddrMapSetWorker(AddrMap *self, uintptr_t addr, Worker *worker) {
    self->addrToWorker[addr % BUCKET_COUNT] = worker;
}

void AddrMapInit(AddrMap* self) {
    memset(self->addrToWorker, 0, sizeof(self->addrToWorker));
}
