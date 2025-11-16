#include "process.h"
#include "schedule-main.h"
#include <string.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct process* createProcess(char name[9], int remainingTime, int arrivalTime, int priority) {
    struct process* newProcess = (struct process*)malloc(sizeof(struct process));

    strcpy(newProcess->name, name);
    newProcess->remainingTime = remainingTime;
    newProcess->arrivalTime = arrivalTime;
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

    assert(processA->finishTime == 9);
    assert(processB->finishTime == 24);
    assert(processC->finishTime == 4);
    assert(processD->finishTime == 19);
    assert(processE->finishTime == 20);
    assert(processF->finishTime == 89);
    assert(processG->finishTime == 38);
    assert(processH->finishTime == 45);
    assert(processI->finishTime == 36);
    assert(processJ->finishTime == 48);
    assert(processK->finishTime == 50);
    assert(processL->finishTime == 66);
    assert(processM->finishTime == 67);
    assert(processN->finishTime == 86);
    assert(processO->finishTime == 81);


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