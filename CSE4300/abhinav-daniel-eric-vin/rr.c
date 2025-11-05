#include "process.h"
#include "rr.h"

static int counter = 0; //Used to keep track of whose turn it is
const int timeQuantum = 1; //A constant value so I can set th time quantum for reset

int tq = timeQuantum; //Keeps track of the remaining time quantum for current process

void rr(struct process **procArray, int *arrayIdx, int globalTime){
    int turn = counter%(*arrayIdx);                           
    procArray[turn] -> remainingTime -= 1;                      
    if(procArray[turn] -> remainingTime <= 0){
        procArray[turn] -> finishTime = globalTime+1;
        tq = timeQuantum;

        int j = 0;
        for(int i = 0; i < arrayIdx - 1; i++){
            if(i == turn){
                continue;
            }
            procArray[j] = procArray[i];
            j++;
        }
        (*arrayIdx)--; 
    }

    tq --;
    if(tq <= 0){
        tq = timeQuantum;
        counter++;
    }
}