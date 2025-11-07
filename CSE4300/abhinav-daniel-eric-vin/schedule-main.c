#include "process.h"
#include <stdio.h>
#include <stdlib.h>

int scheduleMain(struct process **procArray, int procArraySize, int maxTimesteps) {

    //create arrays for each queue level
    struct process **RRQueue = malloc(procArraySize * sizeof(struct process*)); //set all to nullptr
    struct process **STCFQueue = malloc(procArraySize * sizeof(struct process*));
    struct process **FIFOQueue = malloc(procArraySize * sizeof(struct process*));
    struct process **SJFQueue = malloc(procArraySize * sizeof(struct process*));

    int RRQueueidx = 0;
    int STCFQueueidx = 0;
    int FIFOQueueidx = 0;
    int SJFQueueidx = 0;

    for(int i=0; i<procArraySize; i++) {
        RRQueue[i] = NULL;
        STCFQueue[i] = NULL;
        FIFOQueue[i] = NULL;
        SJFQueue[i] = NULL;
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
        if(RRQueueidx == 0) {
            emptyQueues[0] = 1; //set to empty
        }
        if(STCFQueueidx == 0) {
            emptyQueues[1] = 1;
        }
        if(FIFOQueueidx == 0) {
            emptyQueues[2] = 1;
        }
        if(SJFQueueidx == 0) {
            emptyQueues[3] = 1;
        }

        //simulate some queueing & scheduling VV

        //iterate through procArray to find newly arriving processes
        //add new processes to appropriate queue & update emptyQueues array
        for(int i=0; i<procArraySize; i++) {
            if (procArray[i]->arrivalTime == t) {
                if (procArray[i]->priority == 1) {
                    //round robin
                    RRQueue[RRQueueidx] = procArray[i]; //add pointer to queue
                    (RRQueueidx)++; //increment queue tracker
                    emptyQueues[0] = 0; //not empty anymore
                } else if (procArray[i]->priority == 2) {
                    //shortest time to completion first
                    STCFQueue[STCFQueueidx] = procArray[i];
                    (STCFQueueidx)++;
                    emptyQueues[1] = 0;
                } else if (procArray[i]->priority == 3) {
                    //first in first out
                    FIFOQueue[FIFOQueueidx] = procArray[i];
                    (FIFOQueueidx)++;
                    emptyQueues[2] = 0;
                } else if (procArray[i]->priority == 4) {
                    //shortest job first
                    SJFQueue[SJFQueueidx] = procArray[i];
                    (SJFQueueidx)++;
                    emptyQueues[3] = 0;
                } else {
                    printf("Invalid process priority level!\n");
                    exit(13);
                }
            }
        }

        //execute processes based on the highest non-empty queue

        //check to make sure parameters make sense for each function
        if(emptyQueues[0] == 0) { //if this queue is not empty..
            rr(RRQueue, &RRQueueidx, t);
        } else if(emptyQueues[1] == 0) {
            srtrf(STCFQueue, &STCFQueueidx, t);
        } else if(emptyQueues[2] == 0) {
            fifo(FIFOQueue, &FIFOQueueidx, t);
        } else if(emptyQueues[3] == 0) {
            sjf(SJFQueue, &SJFQueueidx, t);
        }

        t++; //increment time (based on type of algorithm)
        }

        //free all allocated memory!!
        free(RRQueue);
        free(STCFQueue);
        free(FIFOQueue);
        free(SJFQueue);

        printf("All processes scheduled!\n");
        return 0;
    }
