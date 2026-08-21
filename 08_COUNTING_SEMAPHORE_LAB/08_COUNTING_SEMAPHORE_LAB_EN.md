# 08. Counting Semaphore — Managing a Resource Pool

## What You'll Learn

Zephyr's `k_sem` becomes a counting semaphore simply by giving it different initial and maximum values — no separate type is needed. A binary semaphore (count 1) and a counting semaphore use exactly the same API; only the numbers passed to `K_SEM_DEFINE` differ.

## Key Concepts

```c
K_SEM_DEFINE(sem, initial_count, max_count);
```

- `initial_count`: how many resources are available at the start
- `max_count`: the maximum value the semaphore can hold (= the number of concurrently allowed resources)

## Code

```c
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
```

## Run & Verify

- Confirm all 4 Workers request the resource at the same time, but only **2 at a time** reach "acquired a slot"
- Confirm the other 2 sit at "waiting for a free slot" and acquire the resource the instant a slot frees up

## Things to Notice

- Setting `POOL_SIZE` to 1 makes this effectively behave like mutual exclusion — though unlike `k_mutex` (covered in Lab 09), there's no ownership concept or priority inheritance here
- This lab creates worker threads dynamically in a loop with `k_thread_create` — declaring an array of stacks all at once with `K_THREAD_STACK_ARRAY_DEFINE` lets you cleanly create multiple threads in a loop like this, when the count is known ahead of time

## Next

Lab 09 (`09_MUTEX_LAB.md`) covers the mutex and priority inheritance.
