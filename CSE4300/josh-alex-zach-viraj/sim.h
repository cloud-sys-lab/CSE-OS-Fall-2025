#ifndef SIM_H
#define SIM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/*
  Minimal CPU scheduler simulator scaffold (tick-driven).

  Goals:
   - Simple API to define a workload (tid, arrival, burst).
   - Queues initialized empty; we pre-admit arrival==0 at tick -1.
   - Each clock tick:
       1) admit arrivals for this tick
       2) scheduling policy chooses which ready threads run
       3) cpu advances one tick
       4) we log the snapshot of all queues/cores to a txt file
   - Keep room to grow (multi-core, waiting/blocked, stats), but stay simple now.
*/

typedef enum {
    ST_NEW = 0,
    ST_READY,
    ST_RUNNING,
    ST_WAITING,
    ST_FINISHED
} ThreadState;

typedef struct Thread {
    int tid;            // integer thread id
    int arrival_time;   // arrival time
    int burst_time;     // total CPU time needed to complete
    int remaining;      // remaining run time
    ThreadState state;  // current state / queue
    int unblocked_at;   // for handling IO blocking
    struct Thread* next;    // for singly-linked queues
    int start_time;         // first time it ever ran; -1 until set
    int finish_time;        // completion time; -1 until set
    int wait_time;          // ticks spent in ready qeue
} Thread;

/* -------- Generic queue of Thread* (singly linked) -------- */
typedef struct {
    Thread* front;
    Thread* rear;
    int     size;
} Queue;

/* -------- CPU with N cores (one thread per core) -------- */
typedef struct {
    int ncores;
    Thread** core;  // core[i] points to the running thread or NULL

    // to trace core activity / schedule
    int  **run_trace;     // run_trace[c][t] = tid or -1
    int    trace_len;     // MAX_TICKS (bounds check convenience)
} CPU;

/* ---------------- Interrupt Configuration ---------------- */
typedef struct {
    int enable_random;   // 0/1
    int pct_io;          // % chance a running thread blocks for I/O
    int io_min;          // min I/O duration (ticks)
    int io_max;          // max I/O duration (ticks)
} InterruptConfig;

/* -------- Global clock (integer ticks) -------- */
extern int SIM_TIME;

/* ===== util.h will expose queue and logging helpers ===== */
#include "util.h"

/* ===== cpu.h will expose CPU helpers ===== */
#include "cpu.h"

/* ===== Workload API ===== */

/* Build an empty, sorted-by-arrival "arrivals" queue through finalize() */
/* workload api you requested */
void workload_init(Queue* workload);
void workload_add(Queue* workload, int tid, int arrival, int burst);
/* Move any threads whose arrival_time == now from workload -> ready */
void workload_admit_tick(Queue* workload, Queue* ready, int now);

/* Utility to free the simple workload list (after finalize). */
void workload_clear(void);

#endif /* SIM_H */
