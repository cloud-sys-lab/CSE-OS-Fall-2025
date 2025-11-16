## Purpose

This project is a teaching simulator written in C that models how CPU schedulers select and run threads on one or more cores. It supports several policies, produces clear text logs, and generates a per core timeline for visualization. The code favors readability and modular design so that new schedulers or experiments can be added by the team.

## High level design

-   **Tick based simulator**  
    A global integer clock `SIM_TIME` advances by one each call to `cpu_step`. At each tick we admit new arrivals, choose who runs, execute one tick on each busy core, and log the result.
    
-   **Thread states and queues**  
    Threads move among Ready, Waiting, Running and Finished. Each state is represented by a singly linked FIFO queue, except Running which is stored per core in the CPU.
    
-   **Multiple cores**  
    `CPU` holds an array of core pointers so each core can run at most one thread per tick. A separate two dimensional array records the thread id that ran on each core at each tick for plotting.
    
-   **Policies as dispatchers**  
    Scheduling policies live in `dispatch.c` and are called once per tick to place threads on idle cores and to preempt when needed.
    
-   **Logs and plots**  
    The simulator writes a readable log at each tick and an easy to parse per core timeline that the Python script plots.
    

## Code layout

```
cpu.h cpu.c             CPU object  both cores and helpers cpu_step bind unbind preempt block
dispatch.h dispatch.c 	Scheduling policies: FIFO SJF SRTCF RR Priority
sim.h sim.c 		    Types main loop workload builder prompts and glue
util.h util.c           Queues selection helpers waiting resolution priority decay logging trace writing
plot_core_trace.py      Python visualizer for core_trace.txt
Makefile                Build run and clean targets
```

## Core types

### Thread

Located in `sim.h`.

-   `tid` integer id
    
-   `arrival_time` first eligible tick
    
-   `burst_time` total CPU time required
    
-   `remaining` remaining CPU time
    
-   `state` one of NEW READY RUNNING WAITING FINISHED
    
-   `unblocked_at` tick when IO completes if waiting
    
-   `next` link for queues
    
-   `start_time` first tick when it ran or minus one until set
    
-   `finish_time` completion tick or minus one until set
    
-   `wait_time` accumulated ticks in Ready
    
-   `quanta_rem` remaining time slice for Round Robin
    
-   `priority` smaller number means higher priority
    

### Queue

Singly linked FIFO of `Thread*` with `front` `rear` and `size`.

### CPU

-   `ncores` count of cores
    
-   `core[i]` pointer to thread running on core i or NULL when idle
    
-   `run_trace[c][t]` thread id that ran on core c at tick t or minus one if idle
    
-   `trace_len` bound used to avoid overruns when recording the trace
    

### InterruptConfig

-   `enable_random` toggle
    
-   `pct_io` chance that a running thread blocks for IO on a tick
    
-   `io_min io_max` duration range for IO blocks
    

## The main loop

Inside `main` in `sim.c`:

1.  **Admit arrivals**  
    `workload_admit_tick` moves any thread whose `arrival_time == SIM_TIME` from the workload queue to Ready.
    
2.  **Resolve IO completions**  
    `waiting_resolve` moves any thread from Waiting to Ready if `unblocked_at <= SIM_TIME`.
    
3.  **Optional random IO**  
    `random_interrupts` may block running threads for a random duration and logs each event.
    
4.  **Dispatch**  
    Call one scheduler per tick to bind ready threads to cores and possibly preempt.
    
5.  **Accounting**  
    `bump_queue_wait` increments `wait_time` for all threads still waiting in Ready this tick.
    
6.  **Execute one tick**  
    `cpu_step` decrements `remaining` and `quanta_rem` for running threads, records the timeline entry for each core at index `SIM_TIME`, then increments the global clock.
    
7.  **Collect completions**  
    Threads with `remaining == 0` are unbound from cores, marked FINISHED, time stamped, and queued in Finished.
    
8.  **Stop condition**  
    End simulation when Ready and Waiting are empty and all cores are idle.
    

The simulator logs a formatted snapshot each tick and appends average response turnaround and waiting times at the end.

## Scheduling policies

Located in `dispatch.c`.

-   **FIFO**  
    Bind the head of Ready to idle cores. Non preemptive.
    
-   **SJF**  
    Pop the thread with the smallest `burst_time` from Ready and bind to idle cores. Non preemptive. Ties honor arrival order since the scan keeps the first minimum.
    
-   **SRTCF**  
    Shortest remaining time to complete. Fill idle cores with smallest `remaining` then repeatedly compare the best Ready job against running jobs. If a running job has strictly larger `remaining` than the best Ready job, preempt it to Ready and run the better job.
    
-   **Round Robin**  
    Parameterized by a quantum in ticks. Any running thread with `quanta_rem == 0` is preempted to Ready at the beginning of dispatch. Idle cores are filled from Ready and new bindings receive `quanta_rem = quantum`.
    
-   **Priority**  
    Smaller `priority` value means higher rank. Fill idle cores with the best priorities. If a Ready thread has better priority than some running thread, preempt the running thread with the worst priority that is still worse than the Ready candidate.
    

## Priority decay

A simple aging helper in `util.c` is called once per tick:

`void  decay_priority(Queue* waiting, Queue* ready);` 

It decreases the numeric priority of threads in Ready and Waiting by one down to a floor of zero. Running threads are not aged here. This provides steady anti starvation pressure.

If you prefer to age only Ready, adjust the function to touch just that queue. The current code touches both queues for simplicity.

## Workload definition

Three modes are available at run time:

-   Preset small example  
    Four threads with hand picked values
    
-   Preset large randomized  
    One thousand threads with random arrival burst and priority within a modest range
    
-   Manual entry  
    The program prompts for a count then asks for arrival burst and priority for each thread. Tids are assigned from one upward in the order entered.
    

The workload queue is not required to be sorted. At each tick the simulator scans the workload queue and admits any threads whose arrival time matches the current tick, preserving order for non arrivals.

## Logging and plotting

-   **Tick snapshot**  
    `log_snapshot` writes a readable block that shows Ready Waiting cores and Finished. Multi line mode is enabled in `main` to improve legibility.
    
-   **Final averages**  
    `log_final_averages` computes and prints average response time average turnaround time and average waiting time using the recorded timestamps and `wait_time`.
    
-   **Timeline for plots**  
    `write_core_trace_default` writes `core_trace.txt` where each line lists a core followed by a comma separated list of tokens for each tick such as `T3` or `IDLE`. The Python script reads this file and draws one horizontal lane per core. For small runs the plot shows a legend and x ticks. For large runs it omits both for clarity and speed.
    

## CPU details

From `cpu.c`:

-   `cpu_init` allocates the core array and sets all cores to idle
    
-   `cpu_bind_core` places a thread on a core and stamps `start_time` if this is the first time the thread ran
    
-   `cpu_unbind_core` clears a core and returns the thread
    
-   `preempt_to_ready` moves a running thread back to Ready
    
-   `block_to_waiting` moves a running thread to Waiting and sets `unblocked_at`
    
-   `cpu_step` performs one tick of work on each running core and records the run trace before advancing the global clock
    

## Queue helpers

From `util.c`:

-   FIFO operations `q_push` and `q_pop`
    
-   Selection pops used by schedulers  
    `q_pop_min_burst`  
    `q_pop_min_remaining`  
    `q_pop_highest_priority` selects the first thread with the minimum numeric priority
    
-   `waiting_resolve` moves threads whose `unblocked_at <= now` back to Ready while preserving order
    
-   `bump_queue_wait` adds one unit of waiting to all threads in Ready
    

## Extending the simulator

-   Add a new policy in `dispatch.c` that matches `void policy(CPU* cpu, Queue* ready)`
    
-   Use the selection helpers or write a new one in `util.c`
    
-   Add any new per thread counters to `Thread` and initialize them in `make_thread`
    
-   Update the run time prompt menu in `sim.c` to call your policy
    

## Notes and limitations

-   The simulator does not model context switch cost
    
-   Memory and caches are not modeled
    
-   All time units are ticks and are abstract
    
-   Run trace length is bounded by `MAX_TICKS` in `sim.c`  
    Increase it if you plan to run longer workloads
    

## Quick reference for prompts

-   Choose scheduler by number
    
-   For RR enter quantum in ticks
    
-   Enter number of CPU cores
    
-   Choose whether to enable random IO
    
-   Pick workload mode or enter threads manually  
    Each manual entry expects arrival burst and priority in that order
    

## Example workflow to compare policies

1.  Run FIFO on the small preset and view both `sim_log.txt` and the plot
    
2.  Run SRTCF on the same preset and compare average response and timeline shape
    
3.  Try RR with two or three different quanta to see how slice length affects fairness and latency
    
4.  Try Priority with decay enabled and vary the initial priorities when entering a manual workload
