/*
 * First-Come First-Served (FCFS) scheduling class.
 *
 * Extremely simple teaching implementation:
 *  - Each CPU has a FIFO queue of runnable tasks (rq->fcfs.queue).
 *  - New tasks are appended to the tail.
 *  - The scheduler always runs the head of the queue.
 *  - There is no timeslice inside FCFS; tasks run until they block,
 *    yield, or exit.
 *
 * Higher-priority classes (stop, deadline, RT) still preempt FCFS
 * as usual via the normal scheduler-class ordering.
 */

 #include <linux/sched.h>
 #include "sched.h"
 
 #ifdef CONFIG_SCHED_FCFS
 
 /* ------------------------------------------------------------------ */
 /* Per-rq helpers                                                      */
 /* ------------------------------------------------------------------ */
 
 void init_fcfs_rq(struct fcfs_rq *fcfs_rq, struct rq *rq)
 {
     INIT_LIST_HEAD(&fcfs_rq->queue);
     fcfs_rq->nr_running = 0;
 }
 
 static inline bool fcfs_rq_empty(struct fcfs_rq *fcfs_rq)
 {
     return list_empty(&fcfs_rq->queue);
 }
 
 /* ------------------------------------------------------------------ */
 /* Core class callbacks                                                */
 /* ------------------------------------------------------------------ */
 
 static void enqueue_task_fcfs(struct rq *rq, struct task_struct *p, int flags)
 {
     struct fcfs_rq *fcfs_rq = &rq->fcfs;
 
     /*
      * Don’t enqueue twice. We rely on p->fcfs_node being INIT_LIST_HEAD()
      * when the task is not on the FCFS runqueue.
      */
     if (!list_empty(&p->fcfs_node))
         return;
 
     list_add_tail(&p->fcfs_node, &fcfs_rq->queue);
     fcfs_rq->nr_running++;
 
     /* Global runnable count for this rq: */
     add_nr_running(rq, 1);
 }
 
 static bool dequeue_task_fcfs(struct rq *rq, struct task_struct *p, int flags)
 {
     struct fcfs_rq *fcfs_rq = &rq->fcfs;
 
     if (list_empty(&p->fcfs_node))
         return false;
 
     list_del_init(&p->fcfs_node);
     fcfs_rq->nr_running--;
 
     sub_nr_running(rq, 1);
     return true;
 }
 
 static struct task_struct *
 pick_next_task_fcfs(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
 {
     struct fcfs_rq *fcfs_rq = &rq->fcfs;
     struct task_struct *p;
 
     if (fcfs_rq_empty(fcfs_rq))
         return NULL;
 
     /*
      * Classic FCFS: always pick the task that has been waiting the longest,
      * i.e., the head of the queue.
      */
     p = list_first_entry(&fcfs_rq->queue, struct task_struct, fcfs_node);
     return p;
 }
 
 static void put_prev_task_fcfs(struct rq *rq,
                    struct task_struct *p,
                    struct task_struct *next)
 {
     /*
      * Nothing special to do here. The task either stays at the head
      * (if still running) or will be dequeued by dequeue_task_fcfs()
      * if it blocks / exits / migrates.
      */
 }
 
 static void task_tick_fcfs(struct rq *rq, struct task_struct *p, int queued)
 {
     /*
      * FCFS has no timeslice — we never voluntarily preempt a running
      * FCFS task on tick. Preemption still happens if a higher class
      * (RT, DL, stop) becomes runnable.
      */
 }
 
 static void switched_to_fcfs(struct rq *rq, struct task_struct *p)
 {
     /*
      * When a task’s policy changes to SCHED_FCFS, make sure it’s
      * enqueued on the FCFS queue if it is runnable.
      */
     if (task_on_rq_queued(p))
         enqueue_task_fcfs(rq, p, 0);
 }
 
 /* No special behaviour for prio_changed() / switched_from(). */
 
 /* ------------------------------------------------------------------ */
 /* Class descriptor                                                    */
 /* ------------------------------------------------------------------ */
 
 DEFINE_SCHED_CLASS(fcfs) = {
 
     .enqueue_task		= enqueue_task_fcfs,
     .dequeue_task		= dequeue_task_fcfs,
 
     .pick_next_task		= pick_next_task_fcfs,
     .put_prev_task		= put_prev_task_fcfs,
 
     .task_tick		= task_tick_fcfs,
 
     .switched_to		= switched_to_fcfs,
 
     /*
      * All the other hooks (.balance, .select_task_rq, .migrate_task_rq,
      * .task_fork, .task_dead, .set_cpus_allowed, etc.) can be left NULL
      * for a simple per-CPU FCFS implementation. The core scheduler will
      * then fall back to generic behaviour for those.
      */
 };
 
 #endif /* CONFIG_SCHED_FCFS */
 