#include <stdio.h>
#include "rr.h"

#define QUANTUM 4  // Time slice for RR

// Round Robin Scheduling
void rr(struct Process p[], int n, int ct[], int tat[], int wt[]) {
    int remaining[100]; //tracks burst time remaining for each process
    int done = 0; //is for when process are all finished
    int current_time = 0; //time

    for (int i = 0; i < n; i++)
        remaining[i] = p[i].burst; //each process has its own full burst time

    while (1) {
        done = 1; //assume done
        for (int i = 0; i < n; i++) {
            if (remaining[i] > 0) {
                done = 0; //if process is found to not actually be done then done =0
                if (p[i].arrival <= current_time) { //only processes that have arrived
                    if (remaining[i] > QUANTUM) {
                        current_time += QUANTUM; //adds quantum to current time because it has passsed
                        remaining[i] -= QUANTUM; //subtracts the time
                    } else {
                        current_time += remaining[i];
                        remaining[i] = 0;
                        ct[i] = current_time;
                        tat[i] = ct[i] - p[i].arrival;
                        wt[i] = tat[i] - p[i].burst;
                    }
                } else {
                    current_time++;
                }
            }
        }
        if (done)
            break;
    }
}


