#ifndef PROCESS_H
#define PROCESS_H

struct Process {
    int pid;       // Process ID
    int arrival;   // Arrival Time
    int burst;     // Burst Time
    int priority;  // Priority level (smaller number = higher priority)
};

#endif


