#include "thread.h"

// Implement sorting alg (buble sort)
void swap(struct thread* a, struct thread* b) {
    struct thread temp = *a;
    *a = *b;
    *b = temp;
}

void bubble(struct thread* jobs, int num_jobs) {
    for (int i = 0; i < num_jobs - 1; i++) {
        for (int j = 0; j < num_jobs - i - 1; j++) {
            // sort the jobs by their burst time
            if (jobs[j].burst > jobs[j+ 1].burst) {swap(&jobs[j], &jobs[j + 1]);}
        }
    }
}

// sort shortest jobs (burst time) using BubbleSort
void sjf_scheduler(struct thread *jobs, int num_jobs) {
    int current_time = 0;

    bubble(jobs, num_jobs);
    for (int i = 0; i < num_jobs; i++) {
        current_time += jobs[i].burst;
        //adding turnaround time calculation
        jobs[i].turnaround = current_time - jobs[i].arrival;
        jobs[i].wait = jobs[i].turnaround - jobs[i].burst;
    }
}
