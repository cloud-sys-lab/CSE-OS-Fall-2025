#ifndef CPU_H
#define CPU_H

#include "sim.h"

/* Initialize CPU with n cores; cores start idle (NULL) */
void cpu_init(CPU* cpu, int ncores);

/* Return number of idle cores */
int  cpu_idle_count(const CPU* cpu);

/* Bind a thread to a specific core (core must be idle) */
void cpu_bind_core(CPU* cpu, int core_idx, Thread* t);

/* Unbind (make core idle), return the thread pointer (or NULL) */
Thread* cpu_unbind_core(CPU* cpu, int core_idx);

/* Preempt: remove thread from core -> push to Ready. Returns T* or NULL. */
void preempt_to_ready(CPU* cpu, int core_idx, Queue* ready);

/* Block: remove thread from core -> push to Waiting with unblock_at time. */
void block_to_waiting(CPU* cpu, int core_idx, Queue* waiting, int unblock_at);

/* Returns 1 if any core is idle, else 0 */
int  cpu_any_idle(const CPU* cpu);

/* Returns index of first idle core, or -1 if none */
int  cpu_first_idle(const CPU* cpu);

/* Bind a thread to the first idle core. Returns core index or -1 if none */
int  cpu_bind_first_idle(CPU* cpu, Thread* t);

/* Advance one clock tick:
   - decrement remaining on each running thread
   - if a thread reaches 0, leave it bound (caller can detect and complete) */
void cpu_step(CPU* cpu);

#endif /* CPU_H */
