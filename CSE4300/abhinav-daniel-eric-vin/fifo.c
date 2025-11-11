#include "process.h"
#include "fifo.h"
#include <stdio.h>

void fifo(struct process **FIFOQueue, int* queueptr, int globalTime){

    //rearrange for this algorithm
    //(no rearranging needed for fifo)
    //subtract from procArray[0]'s remaining time ("executing the process")

    FIFOQueue[0]->remainingTime--;
    //if remainingtime == 0:
        //set finishtime
        //remove procArray[0] from queue
        //shift all other processes in the queue down so the next one is in procArray[0]
        //update queueptr

    if(FIFOQueue[0]->remainingTime == 0) {
        FIFOQueue[0]->finishTime = globalTime+1;
        FIFOQueue[0] = NULL; //remove the finished process from the list
        for(int i=0; i<*queueptr-1; i++) {
            FIFOQueue[i] = FIFOQueue[i+1]; //shift queue down
        }
        (*queueptr)--;
    }

}