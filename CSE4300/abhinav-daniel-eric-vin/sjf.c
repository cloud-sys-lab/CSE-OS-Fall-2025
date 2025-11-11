#include "process.h"
#include "sjf.h"
#include <stdlib.h>
#include <stdio.h>

static int curr_task = -1;

void sjf(struct process **procArray, int *len, int globalTime){
    int shortest = 2147483647;

    if(*len == 0){
        return;
    }

    if (curr_task != -1){
        printf("Time %d: Executing %s\n", globalTime, procArray[curr_task]->name);
        procArray[curr_task]->remainingTime--;
        if (procArray[curr_task]->remainingTime == 0){
            //printf("task %d completed, new len is %d\n", procArray[curr_task]->priority, *len - 1);
            procArray[curr_task]->finishTime = globalTime + 1;

            for(int i = curr_task; i < *len - 1; i++){
        //shifts items toward the front of the "queue"
                procArray[i] = procArray[i+1];
            }
            curr_task = -1;
            (*len)--;
        }
        return;
    }


    for(int i = 0; i < *len; i++){
        if(procArray[i]->remainingTime < shortest){
            shortest = procArray[i]->remainingTime;
            curr_task = i;
        }
    }
    //printf("new task is %d\n", procArray[curr_task]->priority);
    
    printf("Time %d: Executing %s\n", globalTime, procArray[curr_task]->name);
    procArray[curr_task]->remainingTime--;
    if (procArray[curr_task]->remainingTime == 0){
        //printf("task %d completed, new len is %d\n", procArray[curr_task]->priority, *len - 1);
        procArray[curr_task]->finishTime = globalTime + 1;

        for(int i = curr_task; i < *len - 1; i++){
        //shifts items toward the front of the "queue"
            procArray[i] = procArray[i+1];
        }
        curr_task = -1;
        (*len)--;
    }
    
    return;
}