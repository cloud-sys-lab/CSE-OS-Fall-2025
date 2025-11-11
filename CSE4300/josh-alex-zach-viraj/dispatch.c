#include "dispatch.h"

DispatchFn dispatch_get(DispatchAlgo algo) {
    switch (algo) {
        case DISP_FIFO:   return dispatch_fifo;
        default:          return dispatch_fifo; /* safe default */
    }
}

const char* dispatch_name(DispatchAlgo algo) {
    switch (algo) {
        case DISP_FIFO:   return "FIFO";
        default:          return "unknown";
    }
}

void dispatch_fifo(CPU* cpu, Queue* ready) {
    while (cpu_any_idle(cpu)) {
        Thread* t = q_pop(ready);
        if (!t) break;
        cpu_bind_first_idle(cpu, t);
    }
}


