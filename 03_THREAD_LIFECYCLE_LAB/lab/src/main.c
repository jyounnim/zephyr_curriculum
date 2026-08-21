// Source: 03_THREAD_LIFECYCLE_LAB.md

#include <zephyr/kernel.h>

#define STACK_SIZE 1024
#define WORKER_PRIORITY 5

K_THREAD_STACK_DEFINE(worker_stack, STACK_SIZE);
static struct k_thread worker_thread_data;
static k_tid_t worker_tid = NULL;

void worker_entry(void *p1, void *p2, void *p3) {
    int job_id = (int)(intptr_t)p1;
    printk("WorkerThread #%d: started\n", job_id);

    for (int i = 0; i < 5; i++) {
        printk("WorkerThread #%d: working... (%d/5)\n", job_id, i + 1);
        k_sleep(K_MSEC(500));
    }

    printk("WorkerThread #%d: done, exiting\n", job_id);
    worker_tid = NULL;
    // returning here ends the thread - no explicit "delete self" call needed
}

void manager_entry(void *p1, void *p2, void *p3) {
    int job_counter = 0;
    while (1) {
        if (worker_tid == NULL) {
            job_counter++;
            printk("ManagerThread: spawning WorkerThread #%d\n", job_counter);
            worker_tid = k_thread_create(
                &worker_thread_data, worker_stack, STACK_SIZE,
                worker_entry, (void *)(intptr_t)job_counter, NULL, NULL,
                WORKER_PRIORITY, 0, K_NO_WAIT
            );
        }
        k_sleep(K_MSEC(1000));
    }
}

K_THREAD_DEFINE(manager_id, 1024, manager_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
