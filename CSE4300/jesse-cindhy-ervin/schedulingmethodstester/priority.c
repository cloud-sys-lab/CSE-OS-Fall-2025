#include <stdio.h>
#include "priority.h"

// Priority Scheduling
void priority_scheduling(struct Process p[], int n, int ct[], int tat[], int wt[]) {
    int completed[100] = {0}; //keeps track if process is done
    int current_time = 0, completed_count = 0; //keeps track of time and how many process have finished

    while (completed_count < n) { //runs until all process are completed
        int idx = -1; //index of best process starts at none
        int best_priority = 99999; //slower nuber = higher prioritry

        // find process with highest priority (lower = higher)
        for (int i = 0; i < n; i++) {
            if (!completed[i] && p[i].arrival <= current_time) { //checks only the non completed proccesses
                if (p[i].priority < best_priority) { //pick the one with lowest number (highest prioirty)
                    best_priority = p[i].priority; // opriorty process
                    idx = i; //index for it
                } 
            }
        }

        if (idx == -1) {
            current_time++;
            continue;
        }

        // execute chosen process
        current_time += p[idx].burst;
        ct[idx] = current_time;
        tat[idx] = ct[idx] - p[idx].arrival;
        wt[idx] = tat[idx] - p[idx].burst;
        completed[idx] = 1;
        completed_count++; //increase completed count
    }
}

