#include "process.h"
#include "fifo.h"

void fifo(struct process **FIFOQueue, int* queueptr, int globalTime){

    //rearrange for this algorithm
    
    //subtract from procArray[0]'s remaining time ("executing the process")

    //if remainingtime == 0:
        //set finishtime
        //remove procArray[0] from queue
        //shift all other processes in the queue down so the next one is in procArray[0]
        //update queueptr

}