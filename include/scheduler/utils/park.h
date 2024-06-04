/**
 * Futex wrapper for park and wake thread
 */
#pragma once

#include <linux/futex.h>      /* Definition of FUTEX_* constants */
#include <sys/syscall.h>      /* Definition of SYS_* constants */
#include <unistd.h>
#include <stdatomic.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Park {
    _Atomic uint32_t epoch;
} Park;

void futex_wait(uint32_t * futex_addr, size_t val);

void futex_wake(uint32_t* futex_addr);

void ParkWait(Park* self, size_t old);

void ParkWake(Park* self);

size_t ParkAnounce(Park* self);

