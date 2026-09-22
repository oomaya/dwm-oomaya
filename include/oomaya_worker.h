/* oomaya_worker.h — Fixed pthread worker pool with eventfd completion.
 *
 * Phase 3 Core. C11 atomics + POSIX pthreads.
 *
 * Architecture:
 *   - N worker threads (2–4, caller-specified) dequeue tasks from the
 *     ABA-safe ring buffer (oomaya_ring_t).
 *   - Each worker calls pool->dispatch(task) to execute the intent.
 *   - On completion, worker writes 1 to pool->eventfd via write(2).
 *   - The reactor's epoll set includes eventfd as a readable fd;
 *     on wake it reads the completion result from task->resp_data and
 *     transmits the response to the originating client via writev(2).
 *
 * Shutdown:
 *   - oomaya_worker_pool_shutdown() sets stop=1 (release), then
 *     pthread_join()s each thread. Workers exit cleanly after their
 *     current task (no forced cancellation — preserves response delivery).
 *
 * Zero dynamic allocations. All state lives in the caller-supplied struct.
 */
#ifndef OOMAYA_WORKER_H
#define OOMAYA_WORKER_H

#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <semaphore.h>
#include "oomaya_ring.h"
#include "oomaya_state.h"

#define OOMAYA_WORKER_MAX_THREADS 4u

/* Dispatch function type: executed by the worker thread on each task.
 * The function must fill task->resp_data and task->resp_len before returning.
 * task->status = 0 on success, negative OOMAYA_IPC_ERR_* on failure. */
typedef void (*oomaya_dispatch_fn_t)(oomaya_task_t *task,
                                     oomaya_wm_state_t *state);

typedef struct {
    pthread_t            threads[OOMAYA_WORKER_MAX_THREADS];
    uint32_t             thread_count;
    int                  eventfd;   /* eventfd(2) — reactor polls this */
    oomaya_ring_t        ring;      /* ABA-safe MPMC task queue         */
    oomaya_ring_t        resp_ring; /* Completion response queue        */
    sem_t                sem_tasks; /* Event-driven sleep on empty     */
    _Atomic int          stop;      /* 1 = shutdown signal              */
    oomaya_wm_state_t   *state;     /* shared state cache (read-only)   */
    oomaya_dispatch_fn_t dispatch;  /* caller-provided handler fn       */
} oomaya_worker_pool_t;

/**
 * oomaya_worker_pool_init() — Initialise pool, create eventfd, spawn threads.
 *
 * @pool      Caller-owned struct (stack or BSS — never heap).
 * @nthreads  Number of worker threads [1, OOMAYA_WORKER_MAX_THREADS].
 * @state     Shared WM state cache pointer (must outlive the pool).
 * @dispatch  Task handler function executed by workers.
 * @return    0 on success, -1 on failure (eventfd or pthread error).
 */
int oomaya_worker_pool_init(oomaya_worker_pool_t *pool,
                             uint32_t              nthreads,
                             oomaya_wm_state_t    *state,
                             oomaya_dispatch_fn_t  dispatch);

/**
 * oomaya_worker_pool_submit() — Enqueue a task for worker execution.
 *
 * @return OOMAYA_RING_OK or OOMAYA_RING_FULL (reactor must backpressure).
 */
int oomaya_worker_pool_submit(oomaya_worker_pool_t *pool,
                               const oomaya_task_t  *task);

/**
 * oomaya_worker_pool_shutdown() — Signal stop, drain ring, join all threads.
 * Blocks until all workers exit. Safe to call from signal context indirectly
 * (set a flag, call from main loop after flag is observed).
 */
void oomaya_worker_pool_shutdown(oomaya_worker_pool_t *pool);

#endif /* OOMAYA_WORKER_H */
