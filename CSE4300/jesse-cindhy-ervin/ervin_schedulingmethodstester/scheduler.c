#include <stdio.h>
#include <string.h>
#include "fcfs.h"
#include "rr.h"
#include "sjf.h"
#include "priority.h"

#define NUM_PROCESSES 5

#ifndef ALGO_NAME
#define ALGO_NAME "Unknown"
#endif

/* =====================================================
   DISPLAY RESULTS
   ===================================================== */
void display_results(struct Process p[], int ct[], int tat[], int wt[], const char *algo_name) {
    printf("\nAlgorithm: %s\n", algo_name);
    printf("PID\tArrival\tBurst\tCT\tTAT\tWT\n");

    float avg_tat = 0, avg_wt = 0;

    for (int i = 0; i < NUM_PROCESSES; i++) {
        printf("%d\t%d\t%d\t%d\t%d\t%d\n",
               p[i].pid, p[i].arrival, p[i].burst, ct[i], tat[i], wt[i]);
        avg_tat += tat[i];
        avg_wt += wt[i];
    }

    printf("Average TAT: %.2f\n", avg_tat / NUM_PROCESSES);
    printf("Average WT : %.2f\n\n", avg_wt / NUM_PROCESSES);
}

/* =====================================================
   TEST CASES (10 TOTAL)
   ===================================================== */

// 1️⃣ All same burst, same arrival
void test_case_same_time_same_burst(struct Process p[]) {
    for (int i = 0; i < NUM_PROCESSES; i++) {
        p[i].pid = i + 1;
        p[i].arrival = 0;
        p[i].burst = 5;
        p[i].priority = i + 1;
    }
}

// 2️⃣ Same burst, different arrivals
void test_case_diff_arrival_same_burst(struct Process p[]) {
    for (int i = 0; i < NUM_PROCESSES; i++) {
        p[i].pid = i + 1;
        p[i].arrival = i * 2;
        p[i].burst = 5;
        p[i].priority = NUM_PROCESSES - i;
    }
}

// 3️⃣ Different bursts, same arrival
void test_case_same_arrival_diff_burst(struct Process p[]) {
    int bursts[5] = {2, 4, 6, 8, 10};
    for (int i = 0; i < NUM_PROCESSES; i++) {
        p[i].pid = i + 1;
        p[i].arrival = 0;
        p[i].burst = bursts[i];
        p[i].priority = i + 1;
    }
}

// 4️⃣ Different bursts + arrivals
void test_case_diff_arrival_diff_burst(struct Process p[]) {
    int bursts[5] = {3, 7, 2, 9, 5};
    for (int i = 0; i < NUM_PROCESSES; i++) {
        p[i].pid = i + 1;
        p[i].arrival = i * 3;
        p[i].burst = bursts[i];
        p[i].priority = (i % 2 == 0) ? 1 : 3;
    }
}

// 5️⃣ One long, rest short (same arrival)
void test_case_one_long_rest_short(struct Process p[]) {
    for (int i = 0; i < NUM_PROCESSES; i++) {
        p[i].pid = i + 1;
        p[i].arrival = 0;
        if (i == 0)
            p[i].burst = 20;
        else
            p[i].burst = 3;
        p[i].priority = i + 1;
    }
}

// 6️⃣ Processes arriving at staggered times
void test_case_staggered_arrival(struct Process p[]) {
    int bursts[5] = {6, 2, 8, 3, 4};
    int arrivals[5] = {0, 1, 4, 6, 10};
    for (int i = 0; i < NUM_PROCESSES; i++) {
        p[i].pid = i + 1;
        p[i].arrival = arrivals[i];
        p[i].burst = bursts[i];
        p[i].priority = i + 2;
    }
}

// 7️⃣ Randomized burst and arrival (typical load)
void test_case_random(struct Process p[]) {
    int arrivals[5] = {0, 2, 4, 6, 8};
    int bursts[5] = {9, 1, 4, 7, 2};
    int prio[5] = {3, 1, 4, 2, 5};
    for (int i = 0; i < NUM_PROCESSES; i++) {
        p[i].pid = i + 1;
        p[i].arrival = arrivals[i];
        p[i].burst = bursts[i];
        p[i].priority = prio[i];
    }
}

// 8️⃣ All arrive late, different priorities
void test_case_late_arrival_priority(struct Process p[]) {
    int bursts[5] = {5, 3, 4, 2, 6};
    int arrivals[5] = {10, 11, 12, 13, 14};
    int prio[5] = {2, 5, 1, 3, 4};
    for (int i = 0; i < NUM_PROCESSES; i++) {
        p[i].pid = i + 1;
        p[i].arrival = arrivals[i];
        p[i].burst = bursts[i];
        p[i].priority = prio[i];
    }
}

// 9️⃣ Very short time quantum test (for RR)
void test_case_rr_small_quantum(struct Process p[]) {
    int bursts[5] = {8, 4, 6, 2, 5};
    int arrivals[5] = {0, 1, 2, 3, 4};
    for (int i = 0; i < NUM_PROCESSES; i++) {
        p[i].pid = i + 1;
        p[i].arrival = arrivals[i];
        p[i].burst = bursts[i];
        p[i].priority = i + 1;
    }
}

// 🔟 High variance bursts and priorities
void test_case_mixed_priority(struct Process p[]) {
    int bursts[5] = {2, 10, 1, 8, 4};
    int arrivals[5] = {0, 0, 1, 2, 3};
    int prio[5] = {5, 1, 3, 2, 4};
    for (int i = 0; i < NUM_PROCESSES; i++) {
        p[i].pid = i + 1;
        p[i].arrival = arrivals[i];
        p[i].burst = bursts[i];
        p[i].priority = prio[i];
    }
}

/* =====================================================
   UNIVERSAL TEST RUNNER
   ===================================================== */
void run_test(
    void (*test_func)(struct Process[]),
    const char *test_name,
    const char *algo_name,
    void (*algo_func)(struct Process[], int, int[], int[], int[])
) {
    struct Process p[NUM_PROCESSES];
    int ct[NUM_PROCESSES], tat[NUM_PROCESSES], wt[NUM_PROCESSES];

    printf("\n%s\n", test_name);
    test_func(p);
    algo_func(p, NUM_PROCESSES, ct, tat, wt);
    display_results(p, ct, tat, wt, algo_name);
}

/* =====================================================
   MAIN FUNCTION
   ===================================================== */
int main() {
    printf("\n=== CPU Scheduling Algorithm Tester ===\n");
    printf("Running Algorithm: %s\n", ALGO_NAME);

    // FCFS
    if (strcmp(ALGO_NAME, "FCFS") == 0) {
        run_test(test_case_same_time_same_burst, "1. Same Arrival + Same Burst", ALGO_NAME, fcfs);
        run_test(test_case_diff_arrival_same_burst, "2. Diff Arrival + Same Burst", ALGO_NAME, fcfs);
        run_test(test_case_same_arrival_diff_burst, "3. Same Arrival + Diff Burst", ALGO_NAME, fcfs);
        run_test(test_case_diff_arrival_diff_burst, "4. Diff Arrival + Diff Burst", ALGO_NAME, fcfs);
        run_test(test_case_one_long_rest_short, "5. One Long + Short", ALGO_NAME, fcfs);
        run_test(test_case_staggered_arrival, "6. Staggered Arrival", ALGO_NAME, fcfs);
        run_test(test_case_random, "7. Randomized Jobs", ALGO_NAME, fcfs);
        run_test(test_case_late_arrival_priority, "8. Late Arrival + Priority", ALGO_NAME, fcfs);
        run_test(test_case_rr_small_quantum, "9. Small Quantum RR Test", ALGO_NAME, fcfs);
        run_test(test_case_mixed_priority, "10. Mixed Priority and Bursts", ALGO_NAME, fcfs);
    }

    // ROUND ROBIN
    else if (strcmp(ALGO_NAME, "RR") == 0) {
        run_test(test_case_same_time_same_burst, "1. Same Arrival + Same Burst", ALGO_NAME, rr);
        run_test(test_case_diff_arrival_same_burst, "2. Diff Arrival + Same Burst", ALGO_NAME, rr);
        run_test(test_case_same_arrival_diff_burst, "3. Same Arrival + Diff Burst", ALGO_NAME, rr);
        run_test(test_case_diff_arrival_diff_burst, "4. Diff Arrival + Diff Burst", ALGO_NAME, rr);
        run_test(test_case_one_long_rest_short, "5. One Long + Short", ALGO_NAME, rr);
        run_test(test_case_staggered_arrival, "6. Staggered Arrival", ALGO_NAME, rr);
        run_test(test_case_random, "7. Randomized Jobs", ALGO_NAME, rr);
        run_test(test_case_late_arrival_priority, "8. Late Arrival + Priority", ALGO_NAME, rr);
        run_test(test_case_rr_small_quantum, "9. Small Quantum RR Test", ALGO_NAME, rr);
        run_test(test_case_mixed_priority, "10. Mixed Priority and Bursts", ALGO_NAME, rr);
    }

    // SJF
    else if (strcmp(ALGO_NAME, "SJF") == 0) {
        run_test(test_case_same_time_same_burst, "1. Same Arrival + Same Burst", ALGO_NAME, sjf);
        run_test(test_case_diff_arrival_same_burst, "2. Diff Arrival + Same Burst", ALGO_NAME, sjf);
        run_test(test_case_same_arrival_diff_burst, "3. Same Arrival + Diff Burst", ALGO_NAME, sjf);
        run_test(test_case_diff_arrival_diff_burst, "4. Diff Arrival + Diff Burst", ALGO_NAME, sjf);
        run_test(test_case_one_long_rest_short, "5. One Long + Short", ALGO_NAME, sjf);
        run_test(test_case_staggered_arrival, "6. Staggered Arrival", ALGO_NAME, sjf);
        run_test(test_case_random, "7. Randomized Jobs", ALGO_NAME, sjf);
        run_test(test_case_late_arrival_priority, "8. Late Arrival + Priority", ALGO_NAME, sjf);
        run_test(test_case_rr_small_quantum, "9. Small Quantum RR Test", ALGO_NAME, sjf);
        run_test(test_case_mixed_priority, "10. Mixed Priority and Bursts", ALGO_NAME, sjf);
    }

    // PRIORITY
    else if (strcmp(ALGO_NAME, "PRIORITY") == 0) {
        run_test(test_case_same_time_same_burst, "1. Same Arrival + Same Burst", ALGO_NAME, priority_scheduling);
        run_test(test_case_diff_arrival_same_burst, "2. Diff Arrival + Same Burst", ALGO_NAME, priority_scheduling);
        run_test(test_case_same_arrival_diff_burst, "3. Same Arrival + Diff Burst", ALGO_NAME, priority_scheduling);
        run_test(test_case_diff_arrival_diff_burst, "4. Diff Arrival + Diff Burst", ALGO_NAME, priority_scheduling);
        run_test(test_case_one_long_rest_short, "5. One Long + Short", ALGO_NAME, priority_scheduling);
        run_test(test_case_staggered_arrival, "6. Staggered Arrival", ALGO_NAME, priority_scheduling);
        run_test(test_case_random, "7. Randomized Jobs", ALGO_NAME, priority_scheduling);
        run_test(test_case_late_arrival_priority, "8. Late Arrival + Priority", ALGO_NAME, priority_scheduling);
        run_test(test_case_rr_small_quantum, "9. Small Quantum RR Test", ALGO_NAME, priority_scheduling);
        run_test(test_case_mixed_priority, "10. Mixed Priority and Bursts", ALGO_NAME, priority_scheduling);
    }

    else {
        printf("Unknown algorithm! Please define ALGO_NAME.\n");
    }

    return 0;
}



