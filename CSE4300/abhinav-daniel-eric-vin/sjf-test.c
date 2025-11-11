#include "process.h"
#include "sjf.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(){
    int procArraySize = 3;
    int maxTimeSteps = 20;

    struct process *processA = (struct process*)malloc(sizeof(struct process));
    struct process *processB = (struct process*)malloc(sizeof(struct process));
    struct process *processC = (struct process*)malloc(sizeof(struct process));

    strcpy(processA->name, "processA");
    strcpy(processB->name, "processB");
    strcpy(processC->name, "processC");

    processA->remainingTime = 10;
    processB->remainingTime = 3;
    processC->remainingTime = 5;

    processA->arrivalTime = 0;
    processB->arrivalTime = 0;
    processC->arrivalTime = 0;

    processA->priority = 1;
    processB->priority = 2;
    processC->priority = 3;

    struct process *procArray[] = {processA, processB, processC};

    struct process **SJFQueue = malloc(procArraySize * sizeof(struct process*));

    int SJFQueueidx = 0;


    for (int i = 0; i < procArraySize; i++){
        SJFQueue[i] = NULL;
    }


    for (int t = 0; t < maxTimeSteps; t++){
        for (int i = 0; i < procArraySize; i++){
            if (procArray[i]->arrivalTime == t){
                SJFQueue[SJFQueueidx] = procArray[i];
                SJFQueueidx++;
            }
        }

        
        sjf(SJFQueue, &SJFQueueidx, t);
    }

    printf("Finish times:\n");

    printf("ProcessA: %d\n", processA->finishTime);
    printf("ProcessB: %d\n", processB->finishTime);
    printf("ProcessC: %d\n", processC->finishTime);

    printf("tasks completed\n");

    return 0;
}
