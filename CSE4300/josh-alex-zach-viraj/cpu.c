#include "cpu.h"

int SIM_TIME = 0;  // global clock

void cpu_init(CPU* cpu, int ncores) {
    // intialize the cores to 0
    cpu->ncores = ncores;
    cpu->core = (Thread**)calloc(ncores, sizeof(Thread*));
    for (int i = 0; i < ncores; ++i) cpu->core[i] = NULL;
}

int cpu_idle_count(const CPU* cpu) {
    int n = 0;
    for (int i = 0; i < cpu->ncores; ++i)
        if (cpu->core[i] == NULL) n++;
    return n;
}

void cpu_bind_core(CPU* cpu, int core_idx, Thread* t) {
    assert(core_idx >= 0 && core_idx < cpu->ncores);
    assert(cpu->core[core_idx] == NULL);
    cpu->core[core_idx] = t;
    t->state = ST_RUNNING;
    if (t->start_time < 0) t->start_time = SIM_TIME;  // first response time stamp
}

Thread* cpu_unbind_core(CPU* cpu, int core_idx) {
    assert(core_idx >= 0 && core_idx < cpu->ncores);
    Thread* t = cpu->core[core_idx];
    cpu->core[core_idx] = NULL;
    return t;
}

void preempt_to_ready(CPU* cpu, int core_idx, Queue* ready) {
    Thread* t = cpu_unbind_core(cpu, core_idx);
    if (!t) return;
    t->state = ST_READY;
    q_push(ready, t);
}

void block_to_waiting(CPU* cpu, int core_idx, Queue* waiting, int unblock_at) {
    Thread* t = cpu_unbind_core(cpu, core_idx);
    if (!t) return;
    t->state = ST_WAITING;
    t->unblocked_at = unblock_at;
    q_push(waiting, t);
}

int cpu_any_idle(const CPU* cpu) {
    for (int i = 0; i < cpu->ncores; ++i) if (cpu->core[i] == NULL) return 1;
    return 0;
}

int cpu_first_idle(const CPU* cpu) {
    for (int i = 0; i < cpu->ncores; ++i) if (cpu->core[i] == NULL) return i;
    return -1;
}

int cpu_bind_first_idle(CPU* cpu, Thread* t) {
    int idx = cpu_first_idle(cpu);
    if (idx < 0) return -1;
    cpu_bind_core(cpu, idx, t);
    return idx;
}

void cpu_step(CPU* cpu) {
    /* One clock tick worth of CPU work on each busy core */
    for (int i = 0; i < cpu->ncores; ++i) {
        Thread* t = cpu->core[i];
        if (!t) continue;
        // update run time
        if (t->remaining > 0) {
            t->remaining -= 1;  // consume one tick
        }
    }
    // record who runs during [SIM_TIME, SIM_TIME+1)
    if (SIM_TIME < cpu->trace_len) {
        for (int c = 0; c < cpu->ncores; ++c) {
            cpu->run_trace[c][SIM_TIME] = cpu->core[c] ? cpu->core[c]->tid : -1;
        }
    }
    /* advance time */
    SIM_TIME += 1;
}
