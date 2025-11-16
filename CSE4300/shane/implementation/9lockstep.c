// demonstrates channels and synchronization

#include <u.h>
#include <libc.h>
#include <thread.h>

#define MAX_VALUE 20

Channel* initiator_channel;
Channel* responder_channel;

void
initiatorthread(void*)
{
    int control;
    
    for(;;) {
        recv(initiator_channel, &control);

        if (control >= MAX_VALUE) {
            print("initiator detected max count (%d), closing channel\n", control);
            chanclose(responder_channel);
            threadexits(nil);
        }
        
        control++;
        print("initiator: %d\n", control);
        send(responder_channel, &control);
    }
}

void
responderthread(void*)
{
    int control;
    
    for(;;) {
        if(recv(responder_channel, &control) < 0) {
            print("\t responder: channel closed: exiting.\n");
            threadexits(nil);
        }

        control++;
        print("\t responder: %d\n", control);
        send(initiator_channel, &control);
    }
}

void
threadmain(int, char*[])
{
    int start;
    
    initiator_channel = chancreate(sizeof(int), 0);
    responder_channel = chancreate(sizeof(int), 0);
    
    threadcreate(initiatorthread, nil, 8*1024);
    threadcreate(responderthread, nil, 8*1024);
    
    start = 0;
    print("started lockstep up to %d iterations\n", MAX_VALUE);
    send(initiator_channel, &start);
    
    threadexits(nil);
}