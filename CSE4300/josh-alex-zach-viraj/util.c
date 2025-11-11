#include "util.h"

/* ---------------- Queue ---------------- */

void q_init(Queue* q) {
    q->front = q->rear = NULL;
    q->size = 0;
}

int q_empty(const Queue* q) {
    return q->size == 0;
}

void q_push(Queue* q, Thread* t) {
    t->next = NULL;
    if (!q->rear) {
        q->front = q->rear = t;
    } else {
        q->rear->next = t;
        q->rear = t;
    }
    q->size++;
}

Thread* q_pop(Queue* q) {
    if (!q->front) return NULL;
    Thread* t = q->front;
    q->front = t->next;
    if (!q->front) q->rear = NULL;
    t->next = NULL;
    q->size--;
    return t;
}

void bump_queue_wait(Queue* q) {
    for (Thread* p = q->front; p; p = p->next) {
        p->wait_time += 1;
    }
}

void q_clear_shallow(Queue* q) {
    while (!q_empty(q)) {
        (void)q_pop(q);
    }
}

void waiting_io_resolve(Queue* waiting, Queue* ready, int now) {
    if (q_empty(waiting)) return;
    Queue keep; q_init(&keep);
    while (!q_empty(waiting)) {
        Thread* t = q_pop(waiting);
        if (t->unblocked_at <= now) {
            t->state = ST_READY;
            q_push(ready, t);
        } else {
            q_push(&keep, t);
        }
    }
    // keep original order of waiting threads
    while (!q_empty(&keep)) q_push(waiting, q_pop(&keep));
}

/* ---------- Logging ---------- */

static void fprint_queue_flat(FILE* fp, const char* label, const Queue* q) {
    fprintf(fp, "%s[", label);
    for (Thread* p = q->front; p; p = p->next) {
        fprintf(fp, "T%d", p->tid);
        if (p->next) fprintf(fp, " ");
    }
    fprintf(fp, "]");
}

static void fprint_queue_block(FILE* fp, const char* label, const Queue* q, const char* indent) {
    fprintf(fp, "%s%s: [", indent, label);
    int first = 1;
    for (Thread* p = q->front; p; p = p->next) {
        if (!first) fprintf(fp, ", ");
        fprintf(fp, "T%d", p->tid);
        first = 0;
    }
    fprintf(fp, "]\n");
}

int log_open(Log* L, const char* path) {
    L->fp = fopen(path, "w");
    L->multiline = 0;               // default is one-line
    if (!L->fp) return -1;
    fprintf(L->fp, "# Simple CPU scheduler simulation log\n");
    fprintf(L->fp, "# Use multiline mode for block formatting per tick.\n\n");
    return 0;
}

void log_close(Log* L) {
    if (L->fp) fclose(L->fp);
    L->fp = NULL;
}

void log_set_multiline(Log* L, int enable) {
    L->multiline = enable ? 1 : 0;
}

static void fprint_cores_array(FILE* fp, const CPU* cpu) {
    fprintf(fp, "Running: [");
    for (int i = 0; i < cpu->ncores; ++i) {
        if (cpu->core[i]) fprintf(fp, "T%d", cpu->core[i]->tid);
        else               fprintf(fp, "-");
        if (i + 1 < cpu->ncores) fprintf(fp, " | ");
    }
    fprintf(fp, "]");
}

void log_snapshot(Log* L, int t,
                  const Queue* ready,
                  const Queue* waiting,
                  const CPU* cpu,
                  const Queue* finished)
{
    FILE* fp = L && L->fp ? L->fp : stdout;

    if (!L || !L->multiline) {
        // compact one-line (your original style)
        fprintf(fp, "t=%d  ", t);
        fprint_queue_flat(fp, "Ready", ready);
        fprintf(fp, " ");
        fprint_queue_flat(fp, "Waiting", waiting);
        fprintf(fp, " ");
        fprint_cores_array(fp, cpu);
        fprintf(fp, "  ");
        fprint_queue_flat(fp, "Finished", finished);
        fprintf(fp, "\n");
        return;
    }

    // pretty multi-line block
    fprintf(fp, "t=%d\n", t);
    fprint_queue_block(fp, "Ready",   ready,   "\t");
    fprint_queue_block(fp, "Waiting", waiting, "\t");

    fprintf(fp, "\t");
    fprint_cores_array(fp, cpu);
    fprintf(fp, "\n");

    fprint_queue_block(fp, "Finished", finished, "\t");
    fprintf(fp, "\n");
}

/* stringify thread state for logs */
static const char* state_str(ThreadState s) {
    switch (s) {
        case ST_NEW:      return "NEW";
        case ST_READY:    return "READY";
        case ST_RUNNING:  return "RUN";
        case ST_WAITING:  return "WAIT";
        case ST_FINISHED: return "DONE";
        default:          return "?";
    }
}

/* internal: print a header line for tables */
static void fprint_table_header(FILE* fp, const char* title) {
    if (title && *title) fprintf(fp, "%s\n", title);
    fprintf(fp, "%-6s %-8s %-8s %-8s\n", "TID", "ARR", "BURST", "STATE");
}

/* internal: print one thread row */
static void fprint_thread_row(FILE* fp, const Thread* t) {
    fprintf(fp, "%-6d %-8d %-8d %-8s\n",
            t->tid, t->arrival_time, t->burst_time, state_str(t->state));
}

/* public: print the workload queue as a table */
void log_workload(Log* L, const char* heading, const Queue* workload) {
    FILE* fp = L && L->fp ? L->fp : stdout;
    fprint_table_header(fp, heading ? heading : "Workload");
    for (const Thread* p = workload->front; p; p = p->next) {
        fprint_thread_row(fp, p);
    }
    fprintf(fp, "\n");
}

/* optional: print any queue in the same tabular format */
void log_queue_detailed(Log* L, const char* heading, const Queue* q) {
    FILE* fp = L && L->fp ? L->fp : stdout;
    fprint_table_header(fp, heading ? heading : "Queue");
    for (const Thread* p = q->front; p; p = p->next) {
        fprint_thread_row(fp, p);
    }
    fprintf(fp, "\n");
}

void log_final_averages(Log* L, const Queue* finished) {
    FILE* fp = L && L->fp ? L->fp : stdout;

    int n = 0;
    long long sum_resp = 0;
    long long sum_turn = 0;
    long long sum_wait = 0;

    for (Thread* p = finished->front; p; p = p->next) {
        if (p->start_time >= 0 && p->finish_time >= 0) {
            int resp = p->start_time  - p->arrival_time;
            int turn = p->finish_time - p->arrival_time;
            int wait = p->wait_time;  // accumulated per tick from queues

            sum_resp += resp;
            sum_turn += turn;
            sum_wait += wait;
            n += 1;
        }
    }

    fprintf(fp, "\n# Final statistics\n");
    if (n > 0) {
        fprintf(fp, "Average response time:   %.3f\n", (double)sum_resp / n);
        fprintf(fp, "Average turnaround time: %.3f\n", (double)sum_turn / n);
        fprintf(fp, "Average waiting time:    %.3f\n", (double)sum_wait / n);
    } else {
        fprintf(fp, "No completed threads to compute stats.\n");
    }
    fprintf(fp, "\n");
}

void log_interrupts_config(Log* L, int enabled, int pct_io, int io_min, int io_max) {
    FILE* fp = (L && L->fp) ? L->fp : stdout;
    fprintf(fp, "# Random I/O interrupts: %s", enabled ? "ENABLED" : "DISABLED");
    if (enabled) {
        fprintf(fp, "  (pct_io=%d%%, io_min=%d, io_max=%d)", pct_io, io_min, io_max);
    }
    fprintf(fp, "\n\n");
}

void log_io_event(Log* L, int t, int core_idx, int tid, int duration, int unblock_at) {
    FILE* fp = (L && L->fp) ? L->fp : stdout;
    /* One concise line per event */
    fprintf(fp, "INT t=%d core=%d T%d IO_BLOCK duration=%d unblock_at=%d\n",
            t, core_idx, tid, duration, unblock_at);
}

/* ---------------- Core trace writing ---------------- */

/* Compute the last tick index (exclusive) where ANY core is non-idle,
   so we don't dump the entire MAX_TICKS if the tail is all idle. */
static int trace_used_len(const CPU* cpu) {
    int used = cpu->trace_len;
    while (used > 0) {
        int any = 0;
        for (int c = 0; c < cpu->ncores; ++c) {
            if (cpu->run_trace && cpu->run_trace[c] && cpu->run_trace[c][used - 1] != -1) {
                any = 1; break;
            }
        }
        if (any) break;
        used--;
    }
    return used; /* may be 0 */
}

/* Internal: write one core line: "Core k: [T1, -, T3]" */
static void fprint_core_line(FILE* fp, int core_idx, const int* lane, int len) {
    fprintf(fp, "Core %d: [", core_idx);
    for (int t = 0; t < len; ++t) {
        if (t) fprintf(fp, ", ");
        int tid = lane ? lane[t] : -1;
        if (tid < 0) fprintf(fp, "WAITING");
        else         fprintf(fp, "T%d", tid);
    }
    fprintf(fp, "]\n");
}

int write_core_trace(const CPU* cpu, const char* path) {
    if (!cpu || !path || !cpu->run_trace) return 1;
    FILE* f = fopen(path, "w");
    if (!f) return 2;

    int used = trace_used_len(cpu);
    /* If everything was idle, we still print empty lists (len=0). */
    for (int c = 0; c < cpu->ncores; ++c) {
        fprint_core_line(f, c, cpu->run_trace[c], used);
    }

    fclose(f);
    return 0;
}

int write_core_trace_default(const CPU* cpu) {
    return write_core_trace(cpu, "core_trace.txt");
}
