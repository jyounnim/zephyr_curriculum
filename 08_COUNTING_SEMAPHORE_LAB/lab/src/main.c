// Source: 08_COUNTING_SEMAPHORE_LAB.md

#include <zephyr/kernel.h>

#define STACK_SIZE 1024
#define POOL_SIZE 2
#define WORKER_COUNT 4

K_SEM_DEFINE(resource_pool, POOL_SIZE, POOL_SIZE);   // 2 slots, both available at start

void worker_entry(void *p1, void *p2, void *p3) {
    int id = (int)(intptr_t)p1;
    while (1) {
        printk("Worker%d: waiting for a free slot\n", id);
        k_sem_take(&resource_pool, K_FOREVER);
        printk("Worker%d: acquired a slot, using resource...\n", id);
        k_sleep(K_MSEC(2000));   // simulate using the resource
        printk("Worker%d: releasing the slot\n", id);
        k_sem_give(&resource_pool);
        k_sleep(K_MSEC(500));
    }
}

K_THREAD_STACK_ARRAY_DEFINE(worker_stacks, WORKER_COUNT, STACK_SIZE);
static struct k_thread worker_threads[WORKER_COUNT];

int main(void) {
    for (int i = 0; i < WORKER_COUNT; i++) {
        k_thread_create(
            &worker_threads[i], worker_stacks[i], STACK_SIZE,
            worker_entry, (void *)(intptr_t)(i + 1), NULL, NULL,
            5, 0, K_NO_WAIT
        );
    }
    return 0;
}