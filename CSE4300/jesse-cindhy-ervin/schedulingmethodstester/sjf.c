#include <stdio.h>
#include "sjf.h"

//  Shortest Job First (SJF)
void sjf(struct Process p[], int n, int ct[], int tat[], int wt[]) {
    int completed[100] = {0};
    int current_time = 0, completed_count = 0;

    while (completed_count < n) {
        int idx = -1;
        int min_burst = 99999;

        // find shortest available process
        for (int i = 0; i < n; i++) {
            if (!completed[i] && p[i].arrival <= current_time) {
                if (p[i].burst < min_burst) {
                    min_burst = p[i].burst;
                    idx = i;
                }
            }
        }

        if (idx == -1) {
            current_time++;
            continue;
        }

        // compute times
        current_time += p[idx].burst;
        ct[idx] = current_time;
        tat[idx] = ct[idx] - p[idx].arrival;
        wt[idx] = tat[idx] - p[idx].burst;
        completed[idx] = 1;
        completed_count++;
    }
}

