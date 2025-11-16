#include "sim.h"
#include "dispatch.h"

// max simulation ticks
#define MAX_TICKS 50000

/* make a new thread object */
static Thread* make_thread(int tid, int arrival, int burst, int priority) {
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
    t->quanta_rem = 0;
    t->priority = priority;
    return t;
}

/* workload api */
void workload_init(Queue* workload) {
    q_init(workload);
}

void workload_add(Queue* workload, int tid, int arrival, int burst, int priority) {
    Thread* t = make_thread(tid, arrival, burst, priority);
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

// random IO interrupts
static int rnd(int a, int b) { return a + rand() % (b - a + 1); }

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

/* read an int with a prompt and basic validation */
static int prompt_int(FILE* in, FILE* out, const char* msg, int min_allowed) {
    int x;
    for (;;) {
        if (out) fprintf(out, "%s", msg);
        int rc = fscanf(in, "%d", &x);
        if (rc == 1 && x >= min_allowed) return x;
        if (out) fprintf(out, "Invalid input. Please enter an integer >= %d.\n", min_allowed);
        // clear bad token
        int ch;
        while ((ch = fgetc(in)) != '\n' && ch != EOF) { /* discard */ }
        if (feof(in)) return min_allowed - 1; /* signal failure if stream closed */
    }
}

/* Prompts user for workload:
   - number of threads N
   - for i in 1..N: arrival_i, burst_i
   Adds TIDs 1..N in the order entered. */
int workload_prompt(Queue* workload, FILE* in, FILE* out) {
    if (!in) in = stdin;
    if (!out) out = stdout;

    q_init(workload);

    int n = prompt_int(in, out, "Enter number of threads: ", 1);
    if (n < 1) return -1;

    for (int i = 1; i <= n; ++i) {
        int arrival, burst, priority;
        if (out) fprintf(out, "Thread %d - enter arrival burst and priority (e.g. 0 5 0): ", i);
        for (;;) {
            int rc = fscanf(in, "%d %d %d", &arrival, &burst, &priority);
            if (rc == 3 && arrival >= 0 && burst > 0) break;
            if (out) fprintf(out, "Invalid. Format: <arrival>=>=0 <burst>>0 <priority=int>. Try again: ");
            int ch;
            while ((ch = fgetc(in)) != '\n' && ch != EOF) { /* discard */ }
            if (feof(in)) return -1;
        }
        workload_add(workload, /*tid*/ i, arrival, burst, priority);
    }

    if (out) fprintf(out, "Loaded %d threads.\n\n", n);
    return n;
}

int main(void) {
    /* --------- INIT LOGGING ----------- */
    /* open log */
    Log log;
    if (log_open(&log, "sim_log.txt") != 0) {
        fprintf(stderr, "cannot open sim_log.txt\n");
        return 1;
    }
    log_set_multiline(&log, 1);  // turn on the indented block style

    /* --------- INIT WORKLOAD ---------- */
    Queue workload;
    workload_init(&workload);

    /* sim queues */
    Queue ready, waiting, finished;
    q_init(&ready);
    q_init(&waiting);
    q_init(&finished);

    /* ------------------- USER INPUT FOR SCHEDULER -------------------*/
    printf("\nSelect scheduler:\n");
    printf("  1) FIFO\n");
    printf("  2) SJF\n");
    printf("  3) SRTCF\n");
    printf("  4) Round Robin\n");
    printf("  5) Priority\n");
    int choice = 1;
    for (;;) {
        choice = prompt_int(stdin, stdout, "Enter choice [1-5]: ", 1);
        if (choice >= 1 && choice <= 5) break;
        fprintf(stdout, "Please enter a number between 1 and 5.\n");
    }
    
    DispatchAlgo algo = DISP_FIFO;  // default to fifo
    switch (choice) {
        case 2:  algo = DISP_SJF;    break;
        case 3:  algo = DISP_SRTCF;  break;
        case 4:  algo = DISP_RR;     break;
        case 5:  algo = DISP_PR;     break;
        case 1: break;
        default: algo = DISP_FIFO;   break;
    }

    // case for RR chosen (need quantum)
    int rr_quantum = 0;  // init to 0
    if (algo == DISP_RR) {
        printf("\nEnter RR quantum in ticks (>=1): ");
        int choice = 1;
        if (scanf("%d", &choice) != 1) choice = 1;
        rr_quantum = choice;
    }
        
    /* ------------------- USER INPUT FOR CORES -------------------*/
    int ncores = 1;  // init to 1
    choice = 1;
    for (;;) {
        choice = prompt_int(stdin, stdout, "Enter number of CPU cores (>=1): ", 1);
        if (choice >= 1) break;
        fprintf(stdout, "Please enter an integer >=1:\n");
    }
    ncores = choice;


    /*-------  USER INPUT FOR INTERRUPT CONFIGURATION ----------------*/
    choice = 0;
    for (;;) {
        choice = prompt_int(stdin, stdout, "Simulate with random interrupts? (0/1): ", 0);
        if (choice >= 0) break;
        fprintf(stdout, "Please enter 0 or 1:\n");
    }
    int interrupt_enable = choice;
    InterruptConfig intr = {.enable_random = interrupt_enable, .pct_io = 10, .io_min = 2, .io_max = 6};
    srand(42);
    log_interrupts_config(&log, intr.enable_random, intr.pct_io, intr.io_min, intr.io_max);


    /* ------------- USER INPUT FOR WORKLOAD CHOICE -------------- */
    printf("\nSelect workload mode:\n");
    printf("  1) Preset small example\n");
    printf("  2) Preset large randomized\n");
    printf("  3) Manual entry\n");
    choice = 0;
    for (;;) {
        choice = prompt_int(stdin, stdout, "Enter choice [1 3]: ", 1);
        if (choice >= 1 && choice <= 3) break;
        fprintf(stdout, "Please enter an integer 1 through 3:\n");
    }
    /* clear trailing line */
    int ch; while ((ch = fgetc(stdin)) != '\n' && ch != EOF) {}

    switch (choice) {
        case 1: {
            /* small preset */
            workload_add(&workload, 1, 0, 5, 10);
            workload_add(&workload, 2, 0, 3, 7);
            workload_add(&workload, 3, 2, 6, 5);
            workload_add(&workload, 4, 4, 4, 4);
            printf("Loaded preset small workload\n\n");
            break;
        }
        case 2: {
            /* large randomized preset */
            int N = 1000;
            /* simple local rnd helper if you do not already have one */
            srand(42);
            for (int i = 1; i <= N; ++i) {
                workload_add(&workload, i, rnd(0, 300), rnd(1, 30), rnd(1, 10));
            }
            printf("Loaded preset large randomized workload with %d threads\n\n", N);
            break;
        }
        case 3:
        default: {
            // user defined workload
            if (workload_prompt(&workload, stdin, stdout) < 0) {
                fprintf(stderr, "Failed to read workload\n");
                return 1;
            }
            break;
        }
    }

    /* ------------ INIT CPU AND CPU TRACE MEMBERS ------------ */
    CPU cpu; cpu_init(&cpu, ncores);

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

    /* TICK AT ZERO*/
    SIM_TIME = 0;

    /* show what will be simulated */
    log_workload(&log, "Workload before simulation", &workload);

    // Admit arrivals for tick 0 from workload -> ready
    workload_admit_tick(&workload, &ready, SIM_TIME);

    // MAIN SIMULATION LOOP
    for (;;) {
        // add processes that arrive at current tick to ready qeue
        workload_admit_tick(&workload, &ready, SIM_TIME);
        waiting_resolve(&waiting, &ready, SIM_TIME);     // move threads from waiting queue to ready queue if block_time has been met

        random_interrupts(&intr, &cpu, &waiting, &log);  // simulate random IO interrupts
    
        // Schedule with selected policy
        switch (algo) {
            case DISP_FIFO:  dispatch_fifo(&cpu, &ready); break;
            case DISP_SJF:   dispatch_sjf(&cpu, &ready);  break;
            case DISP_SRTCF: dispatch_srtcf(&cpu, &ready); break;
            case DISP_RR: dispatch_rr(&cpu, &ready, rr_quantum); break;
            case DISP_PR: dispatch_priority(&cpu, &ready); break;
        }

        // everyone still queued this tick accrues 1 unit of waiting
        bump_queue_wait(&ready);

        /* log state*/
        log_snapshot(&log, SIM_TIME, &ready, &waiting, &cpu, &finished);

        /* run one tick on all cores */
        cpu_step(&cpu);

        /* decay priority (only effective in priority policy)*/
        decay_priority(&waiting, &ready);

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
