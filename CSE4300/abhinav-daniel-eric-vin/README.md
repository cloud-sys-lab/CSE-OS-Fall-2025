# Group Names

Daniel Bokshan / dab21026
Eric Wang / erw22001

# Our Project

For our project, we chose to implement a multi level queue scheduling system. The central scheduler (implemented by Daniel) contains 4 queues, in the following priorities:

- Round Robin (highest priority) implemented by Eric
- Shortest Time to Completion Remaining First , implemented by Abhinav
- First in First out, implemented by Daniel
- Shortest Job First (lowest priority), implemented by Vin.

# Scheduling Policy

- Central scheduler function "scheduleMain" is responsible for allocating processes to each queue based on their priority level. The priority level will not change once it has been set, so a process will remain in that queue until completion. 
- The scheduler will then execute processes one time step (representing 1ms) at a time, and always executing the jobs from the highest queue which is not empty. If a high priority queue is empty, the scheduler checks the next lower queue for waiting processes.
- Once the process is finished, the end time is recorded in its struct and it is removed from the current scheduling policy's queue, but remains in the general queue for testing purposes.