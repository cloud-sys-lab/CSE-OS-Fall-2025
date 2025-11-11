#ifndef UTIL_H
#define UTIL_H

#include "sim.h"

/* ---------- Queue primitives ---------- */
void q_init(Queue* q);
int  q_empty(const Queue* q);
void q_push(Queue* q, Thread* t);
Thread* q_pop(Queue* q);
/* Add 1 tick of waiting to every thread in q (use for Ready; optionally Waiting) */
void bump_queue_wait(Queue* q);

/* (Optional) clear queue nodes (does NOT free Thread objects themselves) */
void q_clear_shallow(Queue* q);

/* Move any WAITING threads whose unblocked_at <= now back to Ready. */
void waiting_io_resolve(Queue* waiting, Queue* ready, int now);

/* ---------- Logging ---------- */
typedef struct {
    FILE* fp;
    int   multiline;   // 0 = one-line (default), 1 = multi-line block
} Log;

int  log_open(Log* L, const char* path);
void log_close(Log* L);
void log_set_multiline(Log* L, int enable);  // NEW

void log_snapshot(Log* L, int t,
                  const Queue* ready,
                  const Queue* waiting,
                  const CPU* cpu,
                  const Queue* finished);

/* Pretty print the workload queue as a table, without modifying it. */
void log_workload(Log* L, const char* heading, const Queue* workload);

/* Optional: detailed print for any queue with per thread fields. */
void log_queue_detailed(Log* L, const char* heading, const Queue* q);

/* Append average response/turnaround/waiting to sim log */
void log_final_averages(Log* L, const Queue* finished);

/* Log whether random I/O interrupts are enabled and their parameters */
void log_interrupts_config(Log* L, int enabled, int pct_io, int io_min, int io_max);

/* Log a single I/O interrupt event */
void log_io_event(Log* L, int t, int core_idx, int tid, int duration, int unblock_at);

/* Write per-core run traces to "core trace.txt".
   Format:
     Core 0: [T1, -, T3, ...]
     Core 1: [-, T2, T2, ...]
   Returns 0 on success, nonzero on error. */
int write_core_trace_default(const CPU* cpu);

/* Same as above but lets you choose the output path. */
int write_core_trace(const CPU* cpu, const char* path);

#endif /* UTIL_H */
