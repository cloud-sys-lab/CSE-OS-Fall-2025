#include "process.h"
#include <stdio.h>
#include <stdlib.h>

void scheduleMain(struct process **procArray, int procArraySize, int maxTimesteps) {

    //create arrays for each queue level
    struct process **RRQueue = malloc(procArraySize * sizeof(int*));
    struct process **STCFQueue = malloc(procArraySize * sizeof(int*));
    struct process **FIFOQueue = malloc(procArraySize * sizeof(int*));
    struct process **SJFQueue = malloc(procArraySize * sizeof(int*));

    //array to keep track of empty/non-empty queues - empty by default
    int emptyQueues[] = {
        1, //RoundRobin
        1, //STCF
        1, //FIFO
        1  //SJF
    };

    int t = 0; //central time counter
    while (t < maxTimesteps) {

        //simulate some queueing & scheduling VV

        //iterate through procArray to find newly arriving processes

        //add new processes to appropriate queue & update emptyQueues array

        //execute processes based on the highest non-empty queue

        t++; //increment time (based on type of algorithm)

    }


}
