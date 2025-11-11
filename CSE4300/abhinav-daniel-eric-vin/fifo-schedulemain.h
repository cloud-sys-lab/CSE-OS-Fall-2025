#ifndef SCHEDULE_MAIN_H
#define SCHEDILE_MAIN_H
#include "process.h"
enum QueueTypes {
    ROUNDROBIN, //priority 1
    STCF,       //priority 2
    FIFO,       //priority 3
    SJF         //priority 4
};


void scheduleMain(struct process **procArray, int procArraySize, int maxTimesteps);

#endif