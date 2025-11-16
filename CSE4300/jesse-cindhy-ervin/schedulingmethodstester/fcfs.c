#include <stdio.h>
#include "fcfs.h"

void fcfs(struct Process p[], int n, int ct[], int tat[], int wt[]) {
    int current_time = 0; //keeps track of how much time has passed

    for (int i = 0; i < n; i++) { //goes through all proccess 1 by 1
        if (current_time < p[i].arrival) //waits until the process have arrived 
            current_time = p[i].arrival;

        current_time += p[i].burst; //cpu runs the process until burst complete
        ct[i] = current_time; //store completion time
        tat[i] = ct[i] - p[i].arrival; //calculates TAT (completion - arrival)
        wt[i] = tat[i] - p[i].burst; // calculates waiting time ( TAT-Burst time)
    }
}




