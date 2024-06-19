/**
 * Futex wrapper for park and wake thread
 */
#pragma once

#ifdef __APPLE__
// TODO
#else
#include <linux/futex.h>
#endif
#include <sys/syscall.h>
#include <unistd.h>
#include <stdatomic.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct Park {
    _Atomic uint32_t epoch;
} Park;

void futex_wait(uint32_t * futex_addr, size_t val);

void futex_wake(uint32_t* futex_addr);

void ParkWait(Park* self, size_t old);

void ParkWake(Park* self);

size_t ParkAnounce(Park* self);

