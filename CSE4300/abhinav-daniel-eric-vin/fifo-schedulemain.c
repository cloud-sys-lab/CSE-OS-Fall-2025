#include "process.h"
#include <stdio.h>
#include <stdlib.h>
#include "fifo.h"

int scheduleMain(struct process **procArray, int procArraySize, int maxTimesteps) {

    //create arrays for each queue level
    struct process **FIFOQueue = malloc(procArraySize * sizeof(struct process*));

    int FIFOQueueidx = 0;

    for(int i=0; i<procArraySize; i++) {
        FIFOQueue[i] = NULL;
    }

    //array to keep track of empty/non-empty queues - empty by default
    int emptyQueues[] = {
        1, //RoundRobin
        1, //STCF
        1, //FIFO
        1  //SJF
    };

    int t = 0; //central time counter - represents ms
    while (t < maxTimesteps) {

        //(leftovers from previous run)
        if(FIFOQueueidx == 0) {
            emptyQueues[2] = 1;
        }

        //simulate some queueing & scheduling VV

        //iterate through procArray to find newly arriving processes
        //add new processes to appropriate queue & update emptyQueues array
        for(int i=0; i<procArraySize; i++) {
            if (procArray[i]->arrivalTime == t) {
                if (procArray[i]->priority == 1) {
                } if (procArray[i]->priority == 3) {
                    //first in first out
                    FIFOQueue[FIFOQueueidx] = procArray[i];
                    (FIFOQueueidx)++;
                    emptyQueues[2] = 0;
                } else {
                    printf("Invalid process priority level!\n");
                    exit(13);
                }
            }
        }

        //execute processes based on the highest non-empty queue

        //check to make sure parameters make sense for each function

        if(emptyQueues[2] == 0) {
            fifo(FIFOQueue, &FIFOQueueidx, t);
        }

        t++;
        }

        //free all allocated memory!!
        free(FIFOQueue);

        printf("All processes scheduled!\n");
        return 0;
    }
