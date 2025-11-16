## CPU Scheduler Simulator

A small C program that simulates a CPU scheduler with multiple cores and several classic scheduling policies. It produces a text log of each tick and a per core timeline that can be visualized with a Python script.

## Quick start

1.  Build
    
    `make` 
    
2.  Run with auto plot
    
    `make run` 
    
    This runs `./sim`, then calls the Python plotter to visualize `core_trace.txt`.
    
3.  Clean build products and logs
    
    `make clean` 
    

## What you will be prompted for

-   Scheduler choice  
    FIFO  
    SJF  
    SRTCF  
    Round Robin  
    Priority
    
-   Round Robin quantum if RR is chosen
    
-   Number of CPU cores
    
-   Whether to simulate random IO interrupts
    
-   Workload mode  
    Preset small example  
    Preset large randomized  
    Manual entry where you type arrival time, burst time, and priority per thread
    

## Output files

-   `sim_log.txt` snapshot of Ready, Waiting, Running, and Finished at each tick plus final averages
    
-   `core_trace.txt` per core run list used by the plotter
    
-   optional image window from the plot script
    

## Requirements

-   gcc and make
    
-   Python 3 with matplotlib  
    Install: `python3 -m pip install matplotlib`
    

If `make run` complains about matplotlib, install it and run again.

## Typical workflow

    
-   Run `make run` and explore the prompts
    
-   Open `sim_log.txt` to see tick by tick state
    
-   Inspect the timeline plot to compare schedulers visually
