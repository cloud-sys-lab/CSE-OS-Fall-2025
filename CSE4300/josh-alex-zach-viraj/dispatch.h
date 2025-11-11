#ifndef DISPATCH_H
#define DISPATCH_H

#include "sim.h"

/* Which scheduler to use */
typedef enum {
    DISP_FIFO = 0,
    /* future: DISP_SJF, DISP_RR, DISP_PRIORITY */
} DispatchAlgo;

/* Function pointer type for any scheduler */
typedef void (*DispatchFn)(CPU* cpu, Queue* ready);

/* Get a scheduler by enum */
DispatchFn dispatch_get(DispatchAlgo algo);

/* Optional: name helper */
const char* dispatch_name(DispatchAlgo algo);

/* Concrete policies */
void dispatch_fifo(CPU* cpu, Queue* ready);

#endif
