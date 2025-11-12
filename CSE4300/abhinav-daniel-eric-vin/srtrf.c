#include "process.h"
#include "srtrf.h"
#include <limits.h>
#include <stdio.h>

void srtrf(struct process **procArray, int *arrayIdx, int globalTime){
  int srt = INT_MAX; //shortest remaining time
  int pid;
  //find shortest remaaining time
  for (int i = 0; i < *arrayIdx; i++){
    if(procArray[i]->remainingTime <= srt){
      srt = procArray[i]->remainingTime;
      pid = i;
    }
  }
  printf("Time %d: Executing %s  SRTRF \n", globalTime, procArray[pid]->name);
  //decrement the shortest time since it runs on this tick
  procArray[pid]->remainingTime--;
  //handle if the job finishes
  if(procArray[pid]->remainingTime == 0){
    
    procArray[pid]->finishTime = globalTime + 1;
    for(int i = pid; i < *arrayIdx; i++){
      procArray[i] = procArray[i+1];
    }

    (*arrayIdx)--;
  }
}
