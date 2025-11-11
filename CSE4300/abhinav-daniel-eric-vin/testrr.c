#include "rr.h"
#include "process.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "string.h"

int scheduleMain(struct process **procArray, int procArraySize, int maxTimesteps) {

    //create arrays for each queue level
    struct process **RRQueue = malloc(procArraySize * sizeof(struct process*)); //set all to nullptr
    int RRQueueidx = 0;

    for(int i=0; i<procArraySize; i++) {
        RRQueue[i] = NULL;
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

        //simulate some queueing & scheduling VV

        //iterate through procArray to find newly arriving processes
        //add new processes to appropriate queue & update emptyQueues array
        for(int i=0; i<procArraySize; i++) {
            if (procArray[i]->arrivalTime == t) {
                if (procArray[i]->priority == 1) {
                    //round robin
                    RRQueue[RRQueueidx] = procArray[i]; //add pointer to queue
                    RRQueueidx++; //increment queue tracker
                    emptyQueues[0] = 0; //not empty anymore
                } 
                else {
                    printf("Invalid process priority level!\n");
                    return 13;
                }
            }
        }

        //execute processes based on the highest non-empty queue

        //check to make sure parameters make sense for each function
        if(emptyQueues[0] == 0) { //if this queue is not empty..
            rr(RRQueue, &RRQueueidx, t);
        } 

        t++; //increment time (based on type of algorithm)
        }

        //free all allocated memory!!
        free(RRQueue);

        printf("All processes scheduled!\n");
        return 0;
    }


void testA(){
    struct process *processA = (struct process*)malloc(sizeof(struct process));
    struct process *processB = (struct process*)malloc(sizeof(struct process));
    struct process *processC = (struct process*)malloc(sizeof(struct process));

    char a[] = "processA";
    char b[] = "processB";
    char c[] = "processC";

    strcpy(processA->name, a);
    strcpy(processB->name, b);
    strcpy(processC->name, c);

    processA->remainingTime = 10;
    processB->remainingTime = 3;
    processC->remainingTime = 5;

    processA->arrivalTime = 0;
    processB->arrivalTime = 0;
    processC->arrivalTime = 0;

    processA->priority = 1;
    processB->priority = 1;
    processC->priority = 1;

    struct process *processes[] = {processA, processB, processC};
    scheduleMain(processes, 3, 50); //The second variable being the array size, and third being the max time

    assert(processA->finishTime == 18);
    assert(processB->finishTime == 8);
    assert(processC->finishTime == 13);

    free(processA);
    free(processB);
    free(processC);
}

int main(){
    testA();
    return 0;
}

