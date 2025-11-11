#include "process.h"
#include "rr.h"
#include <stdio.h>
#include <stdlib.h>

static int counter = 0; //Used to keep track of whose turn it is
const int timeQuantum = 1; //A constant value so I can set th time quantum for reset

int tq = timeQuantum; //Keeps track of the remaining time quantum for current process

void rr(struct process **procArray, int *arrayIdx, int globalTime){
    int turn = counter%(*arrayIdx);    
    printf("turn at time %d: %s     ", globalTime, procArray[turn]->name);
    
    procArray[turn] -> remainingTime--;   
    printf("Process remaining time: %d\n", procArray[turn] -> remainingTime);

    if(procArray[turn] -> remainingTime <= 0){
        printf("%s finished at time %d\n", procArray[turn]->name, globalTime+1);
        procArray[turn] -> finishTime = globalTime+1;
        
        for(int i = turn + 1; i < (*arrayIdx); i++){
            procArray[i-1] = procArray[i];
        }

        (*arrayIdx)--;
        tq = timeQuantum;
        counter--;
    }
    else{
        tq --;
        if(tq <= 0){
            tq = timeQuantum;
            counter++;
        }
    }
}