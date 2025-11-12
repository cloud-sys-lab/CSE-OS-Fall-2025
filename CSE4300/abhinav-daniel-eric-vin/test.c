#include "process.h"
#include "schedule-main.h"
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

struct process* createProcess(char name[9], int remainingTime, int arrivalTime, int priority) {
    struct process* newProcess = (struct process*)malloc(sizeof(struct process));

    strcpy(newProcess->name, name);
    newProcess->remainingTime = remainingTime;
    newProcess->priority = priority;

    return newProcess;
}

void testA(){
    struct process *processA = createProcess("processA", 5, 0, 4);
    struct process *processB = createProcess("processB", 7, 0, 4);
    struct process *processC = createProcess("processC", 4, 0, 3);
    struct process *processD = createProcess("processD", 4, 12, 1);
    struct process *processE = createProcess("processE", 4, 12, 1);
    struct process *processF = createProcess("processF", 20, 30, 4);
    struct process *processG = createProcess("processG", 5, 32, 2);
    struct process *processH = createProcess("processH", 7, 35, 2);
    struct process *processI = createProcess("processI", 1, 35, 2);
    struct process *processJ = createProcess("processJ", 3, 41, 3); 
    struct process *processK = createProcess("processK", 2, 42, 3);
    struct process *processL = createProcess("processL", 5, 57, 1);  
    struct process *processM = createProcess("processM", 5, 58, 1); 
    struct process *processN = createProcess("processN", 7, 75, 2); 
    struct process *processO = createProcess("processO", 4, 77, 2);

    struct process *processes[] = {processA, processB, processC, processD, processE,
                                   processF, processG, processH, processI,
                                   processJ, processK, processL, processM,
                                   processN, processO};

    scheduleMain(processes, 15, 100);

    for(int i = 0; i < 15; i++) {
        printf("%s finished at time %d\n", processes[i]->name, processes[i]->finishTime);
    }
    /*
    assert(processA->finishTime == 5);
    assert(processB->finishTime == 26);
    assert(processC->finishTime == TODO);
    assert(processD->finishTime == TODO);
    assert(processE->finishTime == TODO);
    assert(processF->finishTime == TODO);
    assert(processG->finishTime == TODO);
    assert(processH->finishTime == TODO);
    assert(processI->finishTime == TODO);
    assert(processJ->finishTime == TODO);
    assert(processK->finishTime == TODO);
    assert(processL->finishTime == TODO);
    assert(processM->finishTime == TODO);
    assert(processN->finishTime == TODO);
    assert(processO->finishTime == TODO);
    */

    free(processA);
    free(processB);
    free(processC);    
    free(processD);
    free(processE);
    free(processF);
    free(processG);
    free(processH);
    free(processI);
    free(processJ);
    free(processK);
    free(processL);
    free(processM);
    free(processN);
    free(processO);
}

int main(){
    testA();
    return 0;
    
}