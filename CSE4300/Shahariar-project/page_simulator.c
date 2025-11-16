#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "page_replacement.h"

// Prints the summary for the algorithms
static void print_result(const char *name, int frames, int n, SimResult r) {
    double hit_ratio = 0.0;
    double fault_ratio = 0.0;

// Compute ratios only if there are references
    if (n > 0) {
        hit_ratio = (double)r.hits / n;
        fault_ratio = (double)r.faults / n;
    }

    printf("Algorithm: %s\n", name);
    printf("Frames: %d\n", frames);
    printf("Total references: %d\n", n);
    printf("Page faults: %d\n", r.faults);
    printf("Page hits: %d\n", r.hits);
    printf("Hit ratio: %.4f\n", hit_ratio);
    printf("Fault ratio: %.4f\n", fault_ratio);
    printf("----------------------------------------\n");
}

int main(void) {
    int frames;
    int n;
    int *refs;
    int i;

    srand((unsigned int)time(NULL));


    // Get number of frames
    printf("Enter number of frames: ");
    if (scanf("%d", &frames) != 1 || frames <= 0) {
        fprintf(stderr, "Invalid number of frames.\n");
        return 1;
    }

    // Get number of references
    printf("Enter number of page references: ");
    if (scanf("%d", &n) != 1 || n <= 0) {
        fprintf(stderr, "Invalid number of references.\n");
        return 1;
    }

    refs = (int *)malloc(n * sizeof(int));
    if (refs == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }

    printf("Enter %d page numbers:\n", n);
    for (i = 0; i < n; i++) {
        if (scanf("%d", &refs[i]) != 1) {
            fprintf(stderr, "Invalid input.\n");
            free(refs);
            return 1;
        }
    }

    // Run all four algorithms
    SimResult fifo_res = simulate_fifo(refs, n, frames);
    SimResult lru_res  = simulate_lru(refs, n, frames);
    SimResult min_res  = simulate_min(refs, n, frames);
    SimResult rnd_res  = simulate_random(refs, n, frames);

    // Show results and traces for each algorithm
    print_result("FIFO", frames, n, fifo_res);
    printf("\nFIFO trace:\n");
    fifo_trace(refs, n, frames);
    printf("\n----------------------------------------\n\n");

    print_result("LRU", frames, n, lru_res);
    printf("\nLRU trace:\n");
    lru_trace(refs, n, frames);
    printf("\n----------------------------------------\n\n");

    print_result("MIN (Optimal)", frames, n, min_res);
    printf("\nMIN trace:\n");
    min_trace(refs, n, frames);
    printf("\n----------------------------------------\n\n");

    print_result("Random", frames, n, rnd_res);
    printf("\nRandom trace:\n");
    random_trace(refs, n, frames);
    printf("\n----------------------------------------\n");

    free(refs);
    return 0;
}
