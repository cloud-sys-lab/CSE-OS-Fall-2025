#include "sim.h"
#include "dispatch.h"

// max simulation ticks
#define MAX_TICKS 10000

/* make a new thread object */
static Thread* make_thread(int tid, int arrival, int burst) {
    Thread* t = (Thread*)calloc(1, sizeof(Thread));
    t->tid = tid;
    t->arrival_time = arrival;
    t->burst_time = burst;
    t->remaining = burst;
    t->state = ST_NEW;
    t->next = NULL;
    t->unblocked_at = -1;
    t->start_time  = -1;
    t->finish_time = -1;
    t->wait_time   = 0;
    return t;
}

/* workload api */
void workload_init(Queue* workload) {
    q_init(workload);
}

void workload_add(Queue* workload, int tid, int arrival, int burst) {
    Thread* t = make_thread(tid, arrival, burst);
    /* do not reorder here, user adds in any order */
    q_push(workload, t);
}

// if a thread's arrival time is the current time, add it to ready queue
void workload_admit_tick(Queue* workload, Queue* ready, int now) {
    /* If your workload is unsorted, we scan it fully each tick.
       Complexity is fine for class-sized inputs. */

    if (q_empty(workload)) return;

    Queue keep;
    q_init(&keep);  // threads not arriving this tick

    while (!q_empty(workload)) {
        Thread* t = q_pop(workload);
        if (t->arrival_time == now) {
            t->state = ST_READY;
            q_push(ready, t);
        } else {
            q_push(&keep, t);
        }
    }
    /* put back the non-arrivals in original order */
    while (!q_empty(&keep)) q_push(workload, q_pop(&keep));
}

/* move finished off cores into finished queue */
static void collect_completions(CPU* cpu, Queue* finished) {
    for (int i = 0; i < cpu->ncores; ++i) {
        Thread* t = cpu->core[i];
        if (!t) continue;
        if (t->remaining == 0) {
            (void)cpu_unbind_core(cpu, i);
            t->state = ST_FINISHED;
            if (t->finish_time < 0) t->finish_time = SIM_TIME;  // SIM_TIME advanced after cpu_step_one
            q_push(finished, t);
        }
    }
}

/* stop when no work is left anywhere */
static int all_done(const Queue* ready, const Queue* waiting, const CPU* cpu) {
    if (!q_empty((Queue*)ready))   return 0;
    if (!q_empty((Queue*)waiting)) return 0;
    for (int i = 0; i < cpu->ncores; ++i)
        if (cpu->core[i]) return 0;
    return 1;
}

/* -------- RANDOM INTERRUPT Implementation ----------------*/
static int rnd(int a, int b) { return a + rand() % (b - a + 1); }

// random IO interrupts
static void random_interrupts(const InterruptConfig* cfg, CPU* cpu, Queue* waiting, Log* log)  {
    if (!cfg || !cfg->enable_random) return;

    for (int c = 0; c < cpu->ncores; ++c) {
        Thread* t = cpu->core[c];
        if (!t) continue;

        int r = rand() % 100;
        if (r < cfg->pct_io) {
            int dur = rnd(cfg->io_min, cfg->io_max);
            int unblock = SIM_TIME + dur;

            /* move running thread to Waiting until unblock time */
            block_to_waiting(cpu, c, waiting, unblock);

            /* log the event */
            log_io_event(log, SIM_TIME, c, t->tid, dur, unblock);
        }
    }
}


int main(void) {
    /* build workload directly with the queue api */
    Queue workload;
    workload_init(&workload);

    workload_add(&workload, 1, 0, 5);
    workload_add(&workload, 2, 0, 3);
    workload_add(&workload, 3, 2, 6);
    workload_add(&workload, 4, 4, 4);

    /* sim queues */
    Queue ready, waiting, finished;
    q_init(&ready);
    q_init(&waiting);
    q_init(&finished);

    /* cpu init and n cores */
    CPU cpu;
    cpu_init(&cpu, 1);
    /* run_trace[c][t] = tid at tick t for core c, or -1 if idle */
    int **run_trace = NULL;
    int ncores_for_trace = cpu.ncores;
    // create arrays for tracking cpu run schedule
    run_trace = (int**)malloc(sizeof(int*) * ncores_for_trace);
    for (int c = 0; c < ncores_for_trace; ++c) {
        run_trace[c] = (int *)malloc(sizeof(int) * MAX_TICKS);
        for (int t = 0; t < MAX_TICKS; ++t) run_trace[c][t] = -1;  // idle mark
    }
    cpu.run_trace = run_trace;
    cpu.trace_len = MAX_TICKS;

    /* open log */
    Log log;
    if (log_open(&log, "sim_log.txt") != 0) {
        fprintf(stderr, "cannot open sim_log.txt\n");
        return 1;
    }
    log_set_multiline(&log, 1);  // turn on the indented block style

    /* start at time zero */
    SIM_TIME = 0;

    /* show what will be simulated */
    log_workload(&log, "Workload before simulation", &workload);

    // Admit arrivals for tick 0 from workload -> ready
    workload_admit_tick(&workload, &ready, SIM_TIME);

    // SETUP SCHEDULER
    DispatchAlgo algo = DISP_FIFO;
    DispatchFn schedule = dispatch_get(algo);

    /* INTERRUPT CONFIGURATION*/
    InterruptConfig intr = {.enable_random = 1, .pct_io = 10, .io_min = 2, .io_max = 6};
    srand(42);
    log_interrupts_config(&log, intr.enable_random, intr.pct_io, intr.io_min, intr.io_max);

    // MAIN SIMULATION LOOP
    for (;;) {
        // add processes that arrive at current tick to ready qeue
        workload_admit_tick(&workload, &ready, SIM_TIME);
        waiting_io_resolve(&waiting, &ready, SIM_TIME);     // I/O completions

        random_interrupts(&intr, &cpu, &waiting, &log);  // simulate random IO interrupts
    
        /* choose who runs this tick */
        schedule(&cpu, &ready);

        // everyone still queued this tick accrues 1 unit of waiting
        bump_queue_wait(&ready);

        /* log state*/
        log_snapshot(&log, SIM_TIME, &ready, &waiting, &cpu, &finished);

        /* run one tick on all cores */
        cpu_step(&cpu);

        /* move completed to finished */
        collect_completions(&cpu, &finished);

        /* stop when all done */
        if (all_done(&ready, &waiting, &cpu)) break;
    }

    // final log
    log_snapshot(&log, SIM_TIME, &ready, &waiting, &cpu, &finished);
    log_final_averages(&log, &finished);
    log_close(&log);

    // output CPU core trace
    if (write_core_trace_default(&cpu) == 0) {
        printf("Wrote per-core trace to core trace.txt\n");
    } else {
        printf("Failed to write per-core trace\n");
    }

    /* free thread objects from finished queue only, since we moved all into it */
    while (!q_empty(&finished)) {
        Thread* t = q_pop(&finished);
        free(t);
    }

    // free
    for (int c = 0; c < ncores_for_trace; ++c) free(run_trace[c]);
    free(run_trace);

    free(cpu.core);
    return 0;
}
