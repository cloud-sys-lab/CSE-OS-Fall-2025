/*
------------------------------------------------------
---------- CPU Process Scheduling Simulator ----------
-- Group Project by Mukunda Joshi and Eli Perchenok --
-------- University of Connecticut - CSE 4300 --------
------------------------------------------------------
*/


// Header Files
#include <stdio.h>
#include <stdlib.h>     // memory allocation commands -- malloc()/free()
#include <limits.h>     // for INT-MAX
#include <stdbool.h>    // boolean variables

// Process Structure Definition
typedef struct {
    int pid;            // Process ID
    int arrival;        // Arrival Time
    int burst;          // Burst Time
    int completion;     // Completion Time
    int turnaround;     // Turnaround Time
    int wait;           // Waiting Time
    int remaining;      // Remaining Time
    int priority;       // Priority Rating
    bool inQueue;       // Queueing Check
    bool complete;      // Completion Check
} Process;

// function prototypes
void sjf(Process *jobs, int n);
void srtf(Process *jobs, int n);
int rr(Process *jobs, int n);
void pri(Process *jobs, int n);

// Queue and Queue functions
// initialize queue array
#define QUEUE_SIZE 100 // hard coded size for simulation purposes only.
int readyQueue[QUEUE_SIZE];
// initialize queue counters
int queueFront = 0;
int queueBack = 0;

// Queue helper functions
void enqueue(int proc_IDX) {
    readyQueue[queueBack] = proc_IDX;
    queueBack = (queueBack + 1) % QUEUE_SIZE;
}

int dequeue() {
    int proc_IDX = readyQueue[queueFront];
    queueFront = (queueFront + 1) % QUEUE_SIZE;
    return proc_IDX;
}

bool queueEmpty() {
    return queueFront == queueBack;
}


// Main Function
int main(int argc, char *argv[]){
    printf(" CPU Process Scheduler Simulator \n");
    if (argc < 2) {
        fprintf(stderr, "Usage: ./cpusim [s/f/r/p]\n");
        return 1;
    }
    // check selection
    char select = argv[1][0];
    switch (select) {
    case 's':   // SJF
        printf("You have selected: Shortest Job First (SJF)\n");
        break;

    case 'f':   // SRTF
        printf("You have selected: Shortest Remaining Time to Finish (SRTF)\n");
        break;

    case 'r':   // Round Robin
        printf("You have selected: Round Robin\n");
        printf("(Note: You can only have a maximum of %d processes to simulate.)\n", QUEUE_SIZE);
        break;

    case 'p':   // Priority
        printf("You have selected: Priority\n");
        break;

    default:    // Incorrect Usage
        fprintf(stderr,"Usage: ./cpusim [s/f/r/p]\n");
        return 1;
    }

    // initialize number of processes
    int nproc;
    printf("Enter the number of processes: ");
    scanf("%d", &nproc);

    // check number of processs
    if (nproc <= 0) {
        fprintf(stderr, "Must have a positive number of processes.\n");
        return 1;
    }

    // memory allocation
    Process *jobs = (Process *)malloc(nproc * sizeof(Process));
    if (jobs == NULL) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        return 1;
    }

    // user customization of processes
    printf("\nEnter process details (Arrival and Burst Times):\n");
    for (int i = 0; i < nproc; i++){
        jobs[i].pid = i + 1;
        printf("Process %d\n", jobs[i].pid);
        printf("Arrival Time: ");
        scanf("%d", &jobs[i].arrival);
        printf("\nBurst Time: ");
        scanf("%d", &jobs[i].burst);
        if (argv[1][0] == 'p') {
            printf("\n Select process priority 1-5 with 1 being highest: ");
            scanf("%d", &jobs[i].priority);
        } else {
            jobs[i].priority = 0;
        }

        jobs[i].remaining = jobs[i].burst; // initialize remaining time
        jobs[i].complete = false; // initialize bool as incomplete
        jobs[i].inQueue = false; // initialize bool as in queue
    }

    // switch case for simulator selection
    switch (select) {
    case 's':
        sjf(jobs, nproc);
        break;

    case 'f':
        srtf(jobs, nproc);
        break;

    case 'r':
        if(rr(jobs, nproc) == 1) {
            free(jobs);
            return 1;
        }
        break;

    case 'p':
        pri(jobs, nproc);
        break;
    }

    // cleanup
    free(jobs);
    return 0;
}

// Shortest Job First
void sjf(Process *jobs, int n) {
    // initialize counters
    int time = 0;
    int completed = 0;

    while (completed < n) {
        int shortestIDX = -1;
        int min_burst = INT_MAX;

        // find shortest available job -- criteria: 
        // 1. it has arrived, and 
        // 2. it wasn't yet completed
        for (int i = 0; i < n; i++){
            if (jobs[i].arrival <= time && jobs[i].complete == false) {
                if (jobs[i].burst < min_burst) {
                    min_burst = jobs[i].burst;
                    shortestIDX = i;
                }
            }
        }

        if (shortestIDX == -1){
            // cpu is idle here if no jobs -- advance time
            time++;
        } else {
            // takes next job
            Process *job = &jobs[shortestIDX];
            
            // run to completion since non-preemptive
            time += job->burst;
            job->completion = time;
            job->turnaround = job->completion - job->arrival;
            job->wait = job->turnaround - job->burst;
            job->complete = true;
            completed++;
        }
    }

    // results

    float totalTurnaround = 0;
    float totalWait = 0;

    printf("\n--- SJF Results ---\n");
    for (int i = 0; i < n; i++){
        printf("==================================\n");
        printf("Process ID: %d\n", jobs[i].pid);
        printf("Arrival Time: %d\tBurst Time: %d\n", jobs[i].arrival, jobs[i].burst);
        printf("Completion Time: %d\tTurnaround Time: %d\tWait Time: %d\n", jobs[i].completion, jobs[i].turnaround, jobs[i].wait);
        totalTurnaround += jobs[i].turnaround;
        totalWait += jobs[i].wait;
    }

    printf("==================================\n");
    printf("Average Turnaround Time: %.2f\n", totalTurnaround/n);
    printf("Average Wait Time: %.2f\n", totalWait/n);
}

// Shortest Remaining Time to Finish
void srtf(Process *jobs, int n) {
    int time = 0;
    int completed = 0;

    while (completed < n) {
        // initialize counters
        int shortestIDX = -1;
        int minrt = INT_MAX;

        // find job with the shortest remaining time that has arrived
        for (int i = 0; i < n; i++) {
            if (jobs[i].arrival <= time && jobs[i].complete == false && jobs[i].remaining > 0) {
                if (jobs[i].remaining < minrt) {
                    minrt = jobs[i].remaining;
                    shortestIDX = i;
                }
            }
        }

        if (shortestIDX == -1) {
            // no job available, advance time
            time++;
        } else {
            // job is found, run for one interval
            Process *job = &jobs[shortestIDX];

            // subtract from remaining time
            job->remaining--;

            // check if the selected job is finished in this interval
            if (job->remaining == 0) {
                job->complete = true;
                completed++;

                // set completion time, calculate turnaround and wait time
                job->completion = time + 1;
                job->turnaround = job->completion - job->arrival;
                job->wait = job->turnaround - job->burst;
            }

            // advance time
            time++;
        }
    }

    // results
    float totalTurnaround = 0;
    float totalWait = 0;

    printf("\n--- SRTF Results ---\n");
    for (int i = 0; i < n; i++){
        printf("==================================\n");
        printf("Process ID: %d\n", jobs[i].pid);
        printf("Arrival Time: %d\tBurst Time: %d\n", jobs[i].arrival, jobs[i].burst);
        printf("Completion Time: %d\tTurnaround Time: %d\tWait Time: %d\n", jobs[i].completion, jobs[i].turnaround, jobs[i].wait);
        totalTurnaround += jobs[i].turnaround;
        totalWait += jobs[i].wait;
    }

    printf("==================================\n");
    printf("Average Turnaround Time: %.2f\n", totalTurnaround/n);
    printf("Average Wait Time: %.2f\n", totalWait/n);
}

// Round Robin (set to int for error handling)
int rr(Process *jobs, int n) {

    // check process amount
    if (n > QUEUE_SIZE) {
        fprintf(stderr, "Error: Process amount exceeded program limits.\n");
        return 1;
    }

    // initialize time interval from user input
    int tq;
    printf("Please enter desired time interval: ");
    scanf("%d", &tq);

    // check user input
    if (tq <= 0) {
        fprintf(stderr, "Time interval must be a positive value.\n");
        return 1;
    }

    // initiate counters
    int time = 0;
    int completed = 0;

    // add first processes that arrive at time = 0 (or earliest)
    // find earliest first
    int min_arrival = 10000;
    for (int i = 0; i < n; i++) {
        if (jobs[i].arrival < min_arrival) {
            min_arrival = jobs[i].arrival;
        }
    }

    // set current time to first arrival
    time = min_arrival;

    // add all processes that arrive at this point
    for (int i = 0; i < n; i ++) {
        if (jobs[i].arrival == time && !jobs[i].inQueue) {
            enqueue(i);
            jobs[i].inQueue = true;
        }
    }

    while (completed < n) {
        if (queueEmpty()) {
            // CPU idle here. Find next arrival and advance time.
            int nextArrival = 100000;
            bool foundNext = 0;
            for (int i = 0; i < n; i++){
                if (!jobs[i].complete && !jobs[i].inQueue) {
                    if (jobs[i].arrival < nextArrival) {
                        nextArrival = jobs[i].arrival;
                        foundNext = 1;
                    }
                }
            // if no jobs, that's a bug.
            }
            if (foundNext) {
                time = nextArrival;
                // add all jobs that arrive at this point
                for (int i = 0; i < n; i++) {
                    if (jobs[i].arrival == time && !jobs[i].inQueue) {
                        enqueue(i);
                        jobs[i].inQueue = true;
                    }
                }
            }
            continue; // return to start of while
        }

        // prepare next process from queue
        int procIDX = dequeue();
        Process *job = &jobs[procIDX];

        // determine timing for process
        int run = 0;
        if (job->remaining > tq) {
            run = tq;
        } else {
            run = job->remaining;
        }

        // run the process
        job->remaining -= run;
        int priorRun = time;
        time += run;

        // add new arrivals (especially those that arrived while job was running)
        for (int i = 0; i < n; i++) {
            if (i == procIDX){
                continue;   // skip current job
            }

            if (jobs[i].arrival > priorRun && jobs[i].arrival <= time && !jobs[i].inQueue && !jobs[i].complete) {
                enqueue(i);
                jobs[i].inQueue = true;
            }
        }

        // check if running job is done
        if (job->remaining == 0) {
            job->complete = true;
            completed++;

            job->completion = time;
            job->turnaround = job->completion - job->arrival;
            job->wait = job->turnaround - job->burst;

            job->inQueue = false;
        } else {
            // return to queue if not complete
            enqueue(procIDX);
        }
    }

    // results
    float totalTurnaround = 0;
    float totalWait = 0;

    printf("\n--- Round Robin Results ---\n");
    for (int i = 0; i < n; i++){
        printf("==================================\n");
        printf("Process ID: %d\n", jobs[i].pid);
        printf("Arrival Time: %d\tBurst Time: %d\n", jobs[i].arrival, jobs[i].burst);
        printf("Completion Time: %d\tTurnaround Time: %d\tWait Time: %d\n", jobs[i].completion, jobs[i].turnaround, jobs[i].wait);
        totalTurnaround += jobs[i].turnaround;
        totalWait += jobs[i].wait;
    }

    printf("==================================\n");
    printf("Average Turnaround Time: %.2f\n", totalTurnaround/n);
    printf("Average Wait Time: %.2f\n", totalWait/n);

    return 0;
}

// Non-preemptive priority scheduler
void pri(Process *jobs, int n) {
    int time = 0;
    int completed = 0;

    while (completed < n) {
        int highestPriorityIDX = -1;
        int highestPriority = INT_MAX;
        
        // find highest priority job available -- has arrived and not completed
        for (int i = 0; i < n; i++) {
            if (jobs[i].arrival <= time && jobs[i].complete == false) {
                if (jobs[i].priority < highestPriority) {
                    highestPriority = jobs[i].priority;
                    highestPriorityIDX = i;
                }
            }
        }

        if (highestPriorityIDX == -1) {
            //  No job available, cpu idle. Advance time.
            time++;
        } else {
            // Run job -- non-preemptive
            Process *job = &jobs[highestPriorityIDX];

            time += job->burst;
            job->completion = time;
            job->turnaround = job->completion - job->arrival;
            job->wait = job->turnaround - job->burst;
            job->complete = true;
            completed++;
        }
    }

    // results
    float totalTurnaround = 0;
    float totalWait = 0;

    printf("\n--- Priority Scheduling Results ---\n");
    for (int i = 0; i < n; i++){
        printf("==================================\n");
        printf("Process ID: %d\n", jobs[i].pid);
        printf("Arrival Time: %d\tBurst Time: %d\n", jobs[i].arrival, jobs[i].burst);
        printf("Completion Time: %d\tTurnaround Time: %d\tWait Time: %d\n", jobs[i].completion, jobs[i].turnaround, jobs[i].wait);
        totalTurnaround += jobs[i].turnaround;
        totalWait += jobs[i].wait;
    }

    printf("==================================\n");
    printf("Average Turnaround Time: %.2f\n", totalTurnaround/n);
    printf("Average Wait Time: %.2f\n", totalWait/n);
}

