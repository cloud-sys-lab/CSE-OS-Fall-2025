
# CPU Scheduling Simulator: Windows-like vs Linux CFS-like (C)

This is a **single-file C program** that simulates the same dummy workload under two OS-style schedulers:

- **Windows-like**: Priority-based Round Robin (per-priority time quanta + simple aging)  
- **Linux CFS-like**: Min-`vruntime` run queue with per-`nice` weights and proportional slices

It prints per-process metrics (start, completion, waiting, response), aggregate stats, and a simple ASCII Gantt chart for each policy.

> This is a **simulation** of scheduling policies, **not** a kernel module. It runs on any OS with a C compiler.

---

## Files

- `cpu_sim.c` — the simulator with inline comments on important lines
- `README.md` — this file

---

## Build & Run

### macOS or Linux
```bash
gcc -O2 -Wall -Wextra -o cpu_sim cpu_sim.c
./cpu_sim
```

### Windows (MSYS2/MinGW, WSL, or similar)
```bash
gcc -O2 -Wall -Wextra -o cpu_sim.exe cpu_sim.c
cpu_sim.exe
```

### Visual Studio Code
1. Open the folder in VS Code.
2. Install the **C/C++** extension by Microsoft (IntelliSense & debugging).
3. Build from the integrated terminal (or set up a simple `tasks.json` gcc build task).
4. Run `./cpu_sim` (or `cpu_sim.exe` on Windows).

---

## Output 

- Per-process table:
  - `pid, arrival, burst, start, completion, waiting, response`
- Aggregates:
  - `Makespan, CPU_util, AvgTurn, AvgWait, AvgResp`
- Gantt timeline (ASCII):
```
  1 | -------- P1 12
 13 | ----- P6 21
 ...
```

---

### Windows-like (Priority RR)
- **Ready queues**: 16 levels (0..15). Highest non-empty queue is chosen.
- **Quantum** per priority: higher priority ⇒ slightly longer slice.
- **Aging**: jobs that waited > 10ms get `dyn_prio += 1` (capped at 15).
- **No preempt on arrival**: the current slice finishes before switching.

### Linux CFS-like (simplified)
- **Run queue**: min-heap keyed by `vruntime`.
- **Slice**: `sched_period * (weight / sum_weights)` where `weight` depends on `nice`.
- **vruntime**: increases by `actual_runtime * (512 / weight)`; lower vruntime runs first.

---

## Edit the workload

Open `cpu_sim.c` and modify the `work[]` array:

```c
static Proc work[] = {
    // pid, arrival, burst, base_prio(0..15), nice(-20..19)
    {1,   0, 16, 10,  0},
    {2,   2,  4,  8, -5},
    {3,   4, 20,  6,  5},
    {4,   6,  3, 12, -10},
    {5,  10, 12,  7,  0},
    {6,  12,  8, 14,  2},
};
```

- `base_prio` is used by the Windows-like scheduler (0..15; 15 is highest).
- `nice` is used by the CFS-like scheduler (-20..19; lower is favored).

