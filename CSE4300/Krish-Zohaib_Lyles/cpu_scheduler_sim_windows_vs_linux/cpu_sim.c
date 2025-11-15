
//
// -----------------------------------------------------------------------------
// DESIGN OVERVIEW
// -----------------------------------------------------------------------------
// The program compares two schedulers using the SAME dummy workload:
//   1) Windows-like: Priority-based Round Robin (per-priority quantum, simple aging).
//      - Multiple ready queues (0..15). Higher number => higher priority.
//      - On dispatch, a thread runs for its priority's quantum. If unfinished, it
//        is re-enqueued (Round Robin). Arrival of new tasks does not preempt mid-slice.
//      - "Aging": if a task waited too long, we nudge its dynamic priority upward.
//   2) Linux CFS-like: Completely Fair Scheduler (simplified).
//      - A min-heap ordered by "vruntime". Lower vruntime runs first.
//      - Each pick gets a time slice proportional to its "weight" (derived from nice).
//      - After running, vruntime += actual_runtime * (ref_weight / weight).
//        (ref_weight is weight of nice 0).
//
// The simulator is purely discrete-time; all times are integers ("ms").
// We track per-process metrics: start, completion, waiting, response.
// A simple Gantt chart is printed for each policy.
//
// For clarity, each process has ONE CPU burst (no I/O), which keeps the comparison
// focused on core run-queue behavior.
// -----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAXP 64
#define CLAMP(v,lo,hi) ((v)<(lo)?(lo):((v)>(hi)?(hi):(v)))

// ---------------------------
// Process representation
// ---------------------------
typedef struct {
    int pid;        // process id shown in Gantt (human-friendly label)
    int arrival;    // time it appears in the system
    int burst;      // total CPU time needed

    int base_prio;  // [Windows-like] base priority 0..15 (15 is highest)
    int nice;       // [CFS-like] nice -20..+19 (lower is higher priority)

    // --- runtime state (updated during simulation) ---
    int remaining;      // countdown from burst to 0
    int start_time;     // first time it ever ran (-1 until set)
    int completion;     // time it finished (-1 until set)
    int waiting;        // cumulative time in ready queue
    int last_enq;       // last time it was enqueued into ready

    // Windows-like dynamic priority
    int dyn_prio;       // mutable priority 0..15

    // CFS-like virtual runtime
    double vruntime;    // smaller means "more entitled" to run next
} Proc;

// ---------------------------
// Gantt structure
// ---------------------------
typedef struct {
    int start, end, pid; // [start,end) time slice for process pid
} Slice; // A dynamic array of Slice entries + current size and capacity for amortized growth.

typedef struct {
    Slice *a;  // pointer to heap-allocated array of slice
    int n, cap;// n = number of used elements; cap = allocated capacity
} Gantt;


// Initialize an empty Gantt: no array yet, size 0, capacity 0.
static void gantt_init(Gantt *g){ g->a=NULL; g->n=0; g->cap=0; }
// Append a new slice [s, e) for process pid to the Gantt timeline.
static void gantt_push(Gantt *g, int s, int e, int pid){
    if(e<=s) return; // ignore zero-length or negative slices
    if(g->n==g->cap){ // need to grow
        g->cap = g->cap? g->cap*2 : 64; // start with 64 entries
        g->a = (Slice*)realloc(g->a, g->cap * sizeof(Slice)); // realloc handles NULL case
    }
    g->a[g->n++] = (Slice){s,e,pid};// append new slice
}
static void gantt_print(const char* title, const Gantt *g){ // Print the Gantt chart to stdout.
    printf("\n=== Gantt: %s ===\n", title);// Header
    if(g->n==0){ puts("(empty)"); return; }// Empty case
    for(int i=0;i<g->n;i++){ // For each slice
        int w = g->a[i].end - g->a[i].start;// width proportional to duration
        if(w<1) w=1;// minimum width
        printf("%3d | ", g->a[i].start);// start time
        for(int k=0;k<w;k++) putchar('-');// bar
        printf(" P%d %d\n", g->a[i].pid, g->a[i].end);// process id and end time
    }
}

// ---------------------------
// Dummy workload 
// ---------------------------
static Proc work[] = {
    // pid, arrival, burst, base_prio(0..15), nice(-20..19)
    {1,   0, 16, 10,  0}, // Medium priority → runs early but then yields
    {2,   2,  4,  8, -5}, // Moderate priority → runs after higher ones
    {3,   4, 20,  6,  5}, // Lowest priority → runs last
    {4,   6,  3, 12, -10}, //High priority → short interactive job → runs early
    {5,  10, 12,  7,  0}, // Lower priority → runs later
    {6,  12,  8, 14,  2}, // Very high priority → runs early
};
static const int NWORK = sizeof(work)/sizeof(work[0]);// number of processes in the workload

static void reset(Proc *dst, const Proc *src, int n){// Copy src array to dst and reset runtime state
    for(int i=0;i<n;i++){// For each process
        dst[i] = src[i];// copy all fields
        dst[i].remaining  = dst[i].burst;// reset remaining time
        dst[i].start_time = -1;// not started yet
        dst[i].completion = -1;// not completed yet
        dst[i].waiting    = 0;// no waiting yet
        dst[i].last_enq   = -1;// never enqueued yet
        dst[i].dyn_prio   = CLAMP(dst[i].base_prio, 0, 15);// reset dynamic priority
        dst[i].vruntime   = 0.0;// reset virtual runtime
    }
}

typedef struct { int t; int idx; } Arrival;// arrival event for sorting

// ---------------- Windows-like (Priority RR) ----------------
typedef struct Node { int idx; struct Node* next; } Node;// linked list node for queue
typedef struct { Node* head; Node* tail; } Q;// simple queue with head and tail pointers
static void qpush(Q* q, int idx){// enqueue idx to queue q
    Node* n = (Node*)malloc(sizeof(Node));// allocate new node
    n->idx = idx; n->next = NULL;// initialize node
    if(q->tail) q->tail->next = n; else q->head = n;// link node
    q->tail = n;// update tail pointer
}
static int qpop(Q* q, int *ok){// dequeue from queue q; set *ok=1 if successful, *ok=0 if empty
    if(!q->head){ *ok=0; return -1; }// empty
    Node* n = q->head;// get head node
    int idx = n->idx;// get index
    q->head = n->next;// update head pointer
    if(!q->head) q->tail = NULL;// if empty now, update tail
    free(n);// free node
    *ok=1; return idx;// return index
}

typedef struct {// Windows-like scheduler simulation state
    Q queues[16];// ready queues for priorities 0..15
    int ready_count;//  number of ready processes
    int now;//  current time
    int cs_cost;//  context switch cost
    int quantum_for_prio[16];// time quantum per priority level
    Gantt gantt;//  Gantt chart
    int busy_time;// total CPU busy time
} WinSim;// Windows-like scheduler simulation state

static void win_init(WinSim* s, int cs_cost){// initialize Windows-like simulator
    memset(s, 0, sizeof(*s));// zero out entire struct
    s->cs_cost = cs_cost;// set context switch cost
    s->now = 0;// start at time 0
    for(int i=0;i<16;i++) s->quantum_for_prio[i] = 6 + i/2; // ~6..13ms
    gantt_init(&s->gantt);// initialize Gantt chart
}
static void win_enqueue(WinSim* s, Proc *P, int idx){// enqueue process idx into Windows-like scheduler
    int lvl = CLAMP(P[idx].dyn_prio, 0, 15);// get dynamic priority level
    P[idx].last_enq = s->now;// record last enqueue time
    qpush(&s->queues[lvl], idx);// enqueue into appropriate queue
    s->ready_count++;// increment ready count
}
static int win_pick(WinSim* s, int *quantum){// pick next process to run; return idx and set *quantum
    for(int lvl=15; lvl>=0; lvl--){// check queues from highest to lowest priority
        int ok=0; int idx = qpop(&s->queues[lvl], &ok);// try to dequeue
        if(ok){ s->ready_count--; *quantum = s->quantum_for_prio[lvl]; return idx; }// found one
    }
    return -1;// no process ready
}
static void simulate_windows(Proc *procs, int n, int cs_cost){// simulate Windows-like scheduler
    WinSim sim; win_init(&sim, cs_cost);// initialize simulator

    Arrival arrivals[MAXP];// arrival events
    for(int i=0;i<n;i++){ arrivals[i].t=procs[i].arrival; arrivals[i].idx=i; }// populate arrivals
    for(int i=0;i<n-1;i++) for(int j=i+1;j<n;j++){// sort arrivals by time, then pid
        if(arrivals[j].t < arrivals[i].t ||// earlier time
           (arrivals[j].t==arrivals[i].t && procs[arrivals[j].idx].pid < procs[arrivals[i].idx].pid)){// same time, lower pid
            Arrival tmp = arrivals[i]; arrivals[i]=arrivals[j]; arrivals[j]=tmp;// swap
        }
    }
    int ai=0, running=-1, slice_start=-1, slice_end=-1;// arrival index, running process index, slice times

    while(1){// main simulation loop
        while(ai<n && arrivals[ai].t <= sim.now){// handle arrivals
            win_enqueue(&sim, procs, arrivals[ai].idx);// enqueue arriving process
            ai++;// advance arrival index
        }
        if(running!=-1 && sim.now >= slice_end){// running process slice ended
            int ran = slice_end - slice_start;// actual run time
            if(ran>0){ procs[running].remaining -= ran; sim.busy_time += ran; }// update remaining and busy time
            if(procs[running].remaining <= 0){// process finished
                procs[running].completion = slice_end;// record completion time
                running = -1;// no running process now
            } else {// not finished, re-enqueue with priority adjustment
                procs[running].dyn_prio = CLAMP(procs[running].dyn_prio - 1, 0, 15);// degrade priority
                win_enqueue(&sim, procs, running);// re-enqueue
                running = -1;// no running process now
            }
        }
        if(running==-1){
            if(sim.ready_count==0 && ai<n){ sim.now = arrivals[ai].t; continue; }// idle until next arrival
            int q=0, idx = win_pick(&sim, &q);// pick next process
            if(idx==-1) break;//no process ready, end simulation
            if(procs[idx].last_enq!=-1){// apply aging
                int waited = sim.now - procs[idx].last_enq;// time waited since last enqueue
                if(waited>10) procs[idx].dyn_prio = CLAMP(procs[idx].dyn_prio+1,0,15);// improve priority
            }
            if(sim.cs_cost>0) sim.now += sim.cs_cost;// context switch cost
            if(procs[idx].start_time==-1) procs[idx].start_time = sim.now;// record start time
            int run_len = procs[idx].remaining < q ? procs[idx].remaining : q;// determine run length
            slice_start = sim.now; slice_end = sim.now + run_len;// set slice times
            gantt_push(&sim.gantt, slice_start, slice_end, procs[idx].pid);// record in Gantt
            if(procs[idx].last_enq!=-1) procs[idx].waiting += (slice_start - procs[idx].last_enq);// update waiting time
            running = idx;// set running process
        }
        int next_t = slice_end;// next event time
        if(ai<n && arrivals[ai].t < next_t){ sim.now = arrivals[ai].t; }// next arrival sooner
        else { sim.now = next_t; }// next slice end sooner
    }

    int makespan = 0;// calculate makespan
    for(int i=0;i<n;i++) if(procs[i].completion>makespan) makespan=procs[i].completion;// 

    puts("===== Windows-like (Priority RR) =====");// print header
    printf("%-4s %-7s %-6s %-6s %-10s %-8s %-8s\n","pid","arrival","burst","start","completion","waiting","response");// print column headers
    double avgT=0, avgW=0, avgR=0;// averages
    for(int i=0;i<n;i++){// print per-process stats
        int response = (procs[i].start_time==-1)?-1:(procs[i].start_time - procs[i].arrival);// calculate response time
        printf("%-4d %-7d %-6d %-6d %-10d %-8d %-8d\n",// print stats
               procs[i].pid, procs[i].arrival, procs[i].burst,
               procs[i].start_time, procs[i].completion, procs[i].waiting, response);// print stats
        avgT += (procs[i].completion - procs[i].arrival);// turnaround time
        avgW += procs[i].waiting;// waiting time
        avgR += response;// response time
    }
    avgT/=n; avgW/=n; avgR/=n;// compute averages
    double util = makespan? (double)sim.busy_time / (double)makespan : 0.0;// compute CPU utilization
    printf("Makespan=%d  CPU_util=%.3f  AvgTurn=%.2f  AvgWait=%.2f  AvgResp=%.2f\n",// print summary
           makespan, util, avgT, avgW, avgR);// print summary
    gantt_print("Windows-like", &sim.gantt);// print Gantt chart
    free(sim.gantt.a);
}

// ---------------- Linux CFS-like ----------------
typedef struct {// min-heap for CFS run queue
    int idx[MAXP];// process indices
    double key[MAXP];// vruntime keys
    int n;// number of elements
} MinHeap;// min-heap for CFS run queue

static void hinit(MinHeap* h){ h->n=0; }// initialize heap
static void hswap(MinHeap* h, int i, int j){// swap elements i and j in heap
    int ti=h->idx[i]; h->idx[i]=h->idx[j]; h->idx[j]=ti;// swap indices
    double tk=h->key[i]; h->key[i]=h->key[j]; h->key[j]=tk;// swap keys
}
static void hpush(MinHeap* h, int idx, double key){// push new element onto heap
    int i=h->n++;// insert at end
    h->idx[i]=idx; h->key[i]=key;// set values
    while(i>0){// bubble up
        int p=(i-1)/2;// parent index
        if(h->key[p] <= h->key[i]) break;// heap property satisfied
        hswap(h,i,p); i=p;// swap with parent
    }
}
static int hpop(MinHeap* h, int *idx, double *key){// pop min element from heap
    if(h->n==0) return 0;// empty
    *idx=h->idx[0]; *key=h->key[0];// get min element
    h->n--;// reduce size
    if(h->n>0){ h->idx[0]=h->idx[h->n]; h->key[0]=h->key[h->n]; }// move last to root
    int i=0;// bubble down
    while(1){// bubble down
        int l=2*i+1, r=2*i+2, m=i;// left, right, min
        if(l<h->n && h->key[l] < h->key[m]) m=l;// left child smaller
        if(r<h->n && h->key[r] < h->key[m]) m=r;// right child smaller
        if(m==i) break;// heap property satisfied
        hswap(h,i,m); i=m;// swap with smaller child
    }
    return 1;// success
}

static int nice_weight(int nice){// get weight for given nice value
    if(nice<-20) nice=-20; if(nice>19) nice=19;// 
    switch(nice){// predefined weights
        case -20: return 2048;// highest priority
        case -15: return 1247;// high priority
        case -10: return 933;// above normal
        case  -5: return 717;// normal
        case   0: return 512;// default
        case   1: return 460;// slightly lower
        case   2: return 410;// lower
        case   5: return 335;// low
        case  10: return 222;// very low
        case  15: return 140;// lower
        case  19: return 110;// lowest
        default: {// interpolate for intermediate nice values
            double w = 512.0 * pow(1.25, -(nice/5.0));// exponential decay
            if(w<50) w=50;// minimum weight
            return (int)(w+0.5);// round to nearest
        }
    }
}

typedef struct {// CFS-like scheduler simulation state
    MinHeap runq;// run queue as min-heap
    int sum_weights;// sum of weights of ready processes
    int now;// current time
    int cs_cost;// context switch cost
    int sched_period;// scheduling period for slice calculation
    Gantt gantt;// Gantt chart
    int busy_time;// total CPU busy time
} CFSSim;// CFS-like scheduler simulation state

static void cfs_init(CFSSim* s, int cs_cost, int sched_period){// initialize CFS-like simulator
    hinit(&s->runq);// initialize run queue
    s->sum_weights = 0;// initialize sum of weights
    s->now = 0;// start at time 0
    s->cs_cost = cs_cost;// set context switch cost
    s->sched_period = sched_period;// set scheduling period
    gantt_init(&s->gantt);// initialize Gantt chart
    s->busy_time = 0;// initialize busy time
}
static void cfs_enqueue(CFSSim* s, Proc *P, int idx){// enqueue process idx into CFS-like scheduler
    P[idx].last_enq = s->now;// record last enqueue time
    hpush(&s->runq, idx, P[idx].vruntime);// push onto run queue
    s->sum_weights += nice_weight(P[idx].nice);// update sum of weights
}
static int cfs_pick(CFSSim* s, Proc *P, int *idx, int *slice){// pick next process to run; return idx and slice length
    if(s->runq.n==0) return 0;// no process ready
    double key; int id;// temp variables
    hpop(&s->runq, &id, &key);// pop min element
    *idx = id;// return index
    int w = nice_weight(P[id].nice);// get weight of selected process
    int denom = s->sum_weights>0 ? s->sum_weights : w;// avoid division by zero
    int sl = (int)((double)s->sched_period * ((double)w / (double)denom));// calculate slice length
    if(sl<1) sl=1;// minimum slice length
    *slice = sl;// return slice length
    return 1;// success
}
static void cfs_requeue(CFSSim* s, Proc *P, int idx){// re-enqueue process idx into CFS-like scheduler
    hpush(&s->runq, idx, P[idx].vruntime);// re-enqueue process
}
static void simulate_cfs(Proc *procs, int n, int cs_cost, int sched_period){// simulate CFS-like scheduler
    CFSSim sim; cfs_init(&sim, cs_cost, sched_period);// initialize simulator

    Arrival arrivals[MAXP];// arrival events
    for(int i=0;i<n;i++){ arrivals[i].t=procs[i].arrival; arrivals[i].idx=i; }// populate arrivals
    for(int i=0;i<n-1;i++) for(int j=i+1;j<n;j++){// sort arrivals by time, then pid
        if(arrivals[j].t < arrivals[i].t ||// earlier time
           (arrivals[j].t==arrivals[i].t && procs[arrivals[j].idx].pid < procs[arrivals[i].idx].pid)){// same time, lower pid
            Arrival tmp = arrivals[i]; arrivals[i]=arrivals[j]; arrivals[j]=tmp;// swap
        }
    }
    int ai=0;// arrival index

    while(1){// main simulation loop
        while(ai<n && arrivals[ai].t <= sim.now){// handle arrivals
            cfs_enqueue(&sim, procs, arrivals[ai].idx);// enqueue arriving process
            ai++;// advance arrival index
        }
        if(sim.runq.n==0){// no process ready 
            if(ai<n){ sim.now = arrivals[ai].t; continue; }// idle until next arrival
            break;// no more processes, end simulation
        }
        int idx, slice;// selected process index and slice length
        if(!cfs_pick(&sim, procs, &idx, &slice)) break;// pick next process

        if(sim.cs_cost>0) sim.now += sim.cs_cost;// context switch cost
        if(procs[idx].start_time==-1) procs[idx].start_time = sim.now;// record start time

        int run_len = procs[idx].remaining < slice ? procs[idx].remaining : slice;// determine run length
        int start = sim.now, end = sim.now + run_len;// set slice times

        if(procs[idx].last_enq!=-1) procs[idx].waiting += (start - procs[idx].last_enq);// update waiting time

        procs[idx].remaining -= run_len;// update remaining time
        int w = nice_weight(procs[idx].nice);// get weight
        procs[idx].vruntime += (double)run_len * (512.0 / (double)w);// update vruntime

        gantt_push(&sim.gantt, start, end, procs[idx].pid);// record in Gantt
        sim.busy_time += run_len;// update busy time
        sim.now = end;// advance current time

        if(procs[idx].remaining <= 0){// process finished
            procs[idx].completion = end;// record completion time
            sim.sum_weights -= w;// update sum of weights
        } else {
            procs[idx].last_enq = sim.now;// record last enqueue time
            cfs_requeue(&sim, procs, idx);// re-enqueue process
        }
    }

    int makespan = 0;
    for(int i=0;i<n;i++) if(procs[i].completion>makespan) makespan=procs[i].completion;// calculate makespan

    puts("\n===== Linux CFS-like =====");// print header
    printf("%-4s %-7s %-6s %-6s %-10s %-8s %-8s\n","pid","arrival","burst","start","completion","waiting","response");// print column headers
    double avgT=0, avgW=0, avgR=0;// averages
    for(int i=0;i<n;i++){// print per-process stats
        int response = (procs[i].start_time==-1)?-1:(procs[i].start_time - procs[i].arrival);// calculate response time
        printf("%-4d %-7d %-6d %-6d %-10d %-8d %-8d\n",// print stats
               procs[i].pid, procs[i].arrival, procs[i].burst,// print stats
               procs[i].start_time, procs[i].completion, procs[i].waiting, response);// print stats
        avgT += (procs[i].completion - procs[i].arrival);// turnaround time
        avgW += procs[i].waiting;// waiting time
        avgR += response;// response time
    }
    avgT/=n; avgW/=n; avgR/=n;
    double util = makespan? (double)sim.busy_time / (double)makespan : 0.0;// compute CPU utilization
    printf("Makespan=%d  CPU_util=%.3f  AvgTurn=%.2f  AvgWait=%.2f  AvgResp=%.2f\n",// print summary
           makespan, util, avgT, avgW, avgR);// print summary
    gantt_print("Linux CFS-like", &sim.gantt);// print Gantt chart
    free(sim.gantt.a);// free Gantt array
}

int main(void){// main function
    Proc wprocs[MAXP], cprocs[MAXP];// working copies of processes
    reset(wprocs, work, NWORK);// reset for Windows-like
    reset(cprocs, work, NWORK);// reset for Linux-like

    simulate_windows(wprocs, NWORK, 1);     // cs_cost = 1ms
    simulate_cfs(cprocs, NWORK, 1, 24);     // cs_cost = 1ms, sched_period = 24ms

    return 0;
}
