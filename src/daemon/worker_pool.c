/* worker_pool.c — Fixed pthread worker pool with eventfd completion.
 *
 * Phase 3 Core. C11 + POSIX.1-2008. Zero dynamic allocations.
 *
 * Worker lifecycle:
 *   worker_thread() waits on sem_tasks (0.0% CPU when idle).
 *   On task submission, oomaya_worker_pool_submit() enqueues the task
 *   into the ABA-safe ring buffer and posts sem_tasks.
 *
 *   On execution completion: worker enqueues the result into resp_ring
 *   and signals eventfd (write(eventfd, 1)) to wake the reactor.
 *   The reactor drains resp_ring and transmits the response back to client_fd.
 *
 *   On shutdown: stop flag is set, all threads are unblocked via sem_post,
 *   and joined cleanly.
 */
#define _POSIX_C_SOURCE 200809L
#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/eventfd.h>
#include <string.h>
#include <stdio.h>

#include "../../include/oomaya_worker.h"
#include "../../include/oomaya_ring.h"

/* ── Worker thread ──────────────────────────────────────────────────────── */

static void *
worker_thread(void *arg)
{
    oomaya_worker_pool_t *pool = (oomaya_worker_pool_t *)arg;
    oomaya_task_t         task;
    uint64_t              one = 1u;
    int                   rc;

    for (;;) {
        /* Block until a task is available (0% CPU idle) */
        if (sem_wait(&pool->sem_tasks) < 0)
            continue;

        if (atomic_load_explicit(&pool->stop, memory_order_acquire)) {
            /* If stopping, only process if a task is actually pending */
            rc = oomaya_ring_dequeue(&pool->ring, &task);
            if (rc != OOMAYA_RING_OK)
                break;
        } else {
            rc = oomaya_ring_dequeue(&pool->ring, &task);
            if (rc != OOMAYA_RING_OK)
                continue;
        }

        /* Execute intent — handler populates task.resp_data, task.resp_len, task.status */
        if (pool->dispatch != NULL)
            pool->dispatch(&task, pool->state);
        else
            task.status = 0;

        /* Enqueue result into completion response ring */
        (void)oomaya_ring_enqueue(&pool->resp_ring, &task);

        /* Signal reactor: one completion pending on the eventfd */
        ssize_t _w = write(pool->eventfd, &one, sizeof(one)); (void)_w;
    }

    return NULL;
}

/* ── Public API ─────────────────────────────────────────────────────────── */

int
oomaya_worker_pool_init(oomaya_worker_pool_t *pool,
                        uint32_t              nthreads,
                        oomaya_wm_state_t    *state,
                        oomaya_dispatch_fn_t  dispatch)
{
    uint32_t i;

    if (pool == NULL || nthreads == 0 || nthreads > OOMAYA_WORKER_MAX_THREADS)
        return -1;

    memset(pool, 0, sizeof(*pool));
    pool->thread_count = nthreads;
    pool->state        = state;
    pool->dispatch     = dispatch;
    atomic_store_explicit(&pool->stop, 0, memory_order_relaxed);

    /* Initialize counting semaphore for task notification */
    if (sem_init(&pool->sem_tasks, 0, 0) < 0)
        return -1;

    /* Create the completion eventfd (non-blocking, close-on-exec) */
    pool->eventfd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (pool->eventfd < 0) {
        sem_destroy(&pool->sem_tasks);
        return -1;
    }

    /* Initialise the ABA-safe task and response ring buffers */
    oomaya_ring_init(&pool->ring);
    oomaya_ring_init(&pool->resp_ring);

    /* Spawn worker threads */
    for (i = 0; i < nthreads; ++i) {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0) {
            atomic_store_explicit(&pool->stop, 1, memory_order_release);
            for (uint32_t j = 0; j <= i; ++j)
                sem_post(&pool->sem_tasks);
            while (i-- > 0)
                pthread_join(pool->threads[i], NULL);
            close(pool->eventfd);
            pool->eventfd = -1;
            sem_destroy(&pool->sem_tasks);
            return -1;
        }
    }

    return 0;
}

int
oomaya_worker_pool_submit(oomaya_worker_pool_t *pool, const oomaya_task_t *task)
{
    int rc;
    if (pool == NULL || task == NULL)
        return OOMAYA_RING_FULL;

    rc = oomaya_ring_enqueue(&pool->ring, task);
    if (rc == OOMAYA_RING_OK) {
        /* Wake one waiting worker thread */
        sem_post(&pool->sem_tasks);
    }
    return rc;
}

void
oomaya_worker_pool_shutdown(oomaya_worker_pool_t *pool)
{
    uint32_t i;

    if (pool == NULL)
        return;

    /* Signal all worker threads to stop */
    atomic_store_explicit(&pool->stop, 1, memory_order_release);

    /* Wake all worker threads so they can exit */
    for (i = 0; i < pool->thread_count; ++i)
        sem_post(&pool->sem_tasks);

    /* Join every thread */
    for (i = 0; i < pool->thread_count; ++i)
        pthread_join(pool->threads[i], NULL);

    sem_destroy(&pool->sem_tasks);

    /* Close the eventfd */
    if (pool->eventfd >= 0) {
        close(pool->eventfd);
        pool->eventfd = -1;
    }
}
