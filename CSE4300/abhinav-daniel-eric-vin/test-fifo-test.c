#include"process.h"
#include <stdio.h>
#include <stdlib.h>
#include "fifo-schedulemain.h"

int main(void) {

    //create array of processes

    struct process *processA = (struct process*)malloc(sizeof(struct process));
    struct process *processB = (struct process*)malloc(sizeof(struct process));
    struct process *processC = (struct process*)malloc(sizeof(struct process));
    struct process *processD = (struct process*)malloc(sizeof(struct process));
    int procArraySize = 4;

    processA->remainingTime = 6;
    processB->remainingTime = 2;
    processC->remainingTime = 15;
    processD->remainingTime = 3;

    processA->arrivalTime = 0;
    processB->arrivalTime = 0;
    processC->arrivalTime = 0;
    processD->arrivalTime = 0;

    processA->priority = 3;
    processB->priority = 3;
    processC->priority = 3;
    processD->priority = 4;

    struct process *processArray[] = {processA, processB, processC, processD};

    //run scheduler

    scheduleMain(processArray, procArraySize, 30);


    //print out finish times
    printf("Finish times:\n");
    for(int i=0; i<procArraySize; i++) {
        printf("process %d: %d\n", i, processArray[i]->finishTime);
    }

    free(processA);
    free(processB);
    free(processC);
    free(processD);


    return 0;
}