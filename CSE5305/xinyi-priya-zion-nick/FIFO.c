#include <stdio.h>
#include <stdlib.h>
#include "thread.h"


int findNextJob(struct threads* jobs, int numjobs) {
    int minIndex = -1;
    int minTime = 10000000;

    for (int i = 0; i < numjobs; i++) {
        if (!jobs[i].completion_time && jobs[i].arrival_time < minTime) {
            minTime = jobs[i].arrival_time;
            minIndex = i;
        }
    }
    return minIndex;
}


void fifo(struct threads* jobs, int numjobs) {
    int time = 0;
    int doneCount = 0;

    while (doneCount < numjobs) {
        int next = findNextJob(jobs, numjobs);
        if (next == -1) break; 

        if (time < jobs[next].arrival_time)
            time = jobs[next].arrival_time;

        jobs[next].first_run = time;
        jobs[next].response_time = jobs[next].first_run - jobs[next].arrival_time;

        time += jobs[next].burst_time;

        jobs[next].completion_time = time;
        jobs[next].turnaround_time = time - jobs[next].arrival_time;

        doneCount++;
    }

}