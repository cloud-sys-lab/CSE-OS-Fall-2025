#ifndef PAGE_REPLACEMENT_H
#define PAGE_REPLACEMENT_H

typedef struct {
    int faults;
    int hits;
} SimResult;

// Simulation functions for each page replacement algorithm
SimResult simulate_fifo(const int *refs, int n, int frames);
SimResult simulate_random(const int *refs, int n, int frames);
SimResult simulate_lru(const int *refs, int n, int frames);
SimResult simulate_min(const int *refs, int n, int frames);

// Functions to print the step by step frame trace
void fifo_trace(const int *refs, int n, int frames);
void random_trace(const int *refs, int n, int frames);
void lru_trace(const int *refs, int n, int frames);
void min_trace(const int *refs, int n, int frames);

#endif
