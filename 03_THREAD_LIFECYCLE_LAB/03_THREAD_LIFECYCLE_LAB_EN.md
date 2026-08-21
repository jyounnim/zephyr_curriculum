# 03. Dynamic Thread Creation/Termination

## What You'll Learn

`K_THREAD_DEFINE` is convenient when the number of threads is fixed, but for situations where you need to create and destroy threads on demand at runtime (like work-queue workers), you use `k_thread_create`. Zephyr has a notable characteristic here: you must **declare the stack memory the thread will use, separately and ahead of time**.

## Key Concepts

| Function/Macro | Description |
|---|---|
| `K_THREAD_STACK_DEFINE(name, size)` | Statically reserves the memory a thread will use as its stack (even for a dynamically created thread, the stack itself must be prepared beforehand) |
| `k_thread_create(&data, stack, size, function, p1, p2, p3, priority, options, start_delay)` | Creates a thread at runtime; returns a `k_tid_t` (thread ID) |
| A thread function `return`ing | This by itself terminates the thread |
| `k_thread_abort(tid)` | Forcibly terminates another thread from outside |

## Code

```c
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
```

## Run & Verify

- Confirm `ManagerThread` checks every second whether `worker_tid` is empty, and if so, spawns a new `WorkerThread` — and that this pattern repeats
- Confirm `WorkerThread` does 5 rounds of work and then terminates by `return`ing from its own function → on the next cycle, `ManagerThread` spawns a fresh Worker

## Things to Notice

- The `worker_stack`/`worker_thread_data` in this example are **static** variables — since they reuse the same memory, creating a new Worker before the previous one has fully terminated risks a memory collision. The current code prevents this with the `worker_tid == NULL` check. If you want multiple Workers running at once, you need a separate stack/struct declared for each one
- In Zephyr, **the developer prepares the stack space themselves** with `K_THREAD_STACK_DEFINE` — this reflects Zephyr's design philosophy of making memory usage more explicit and predictable (particularly to support extremely memory-constrained small MCUs alongside larger ones)
- You can also forcibly terminate a running thread from outside with `k_thread_abort(tid)` — but if the thread is holding a resource like a mutex when it's forcibly killed, that resource may never get released, so the "let it finish itself" approach used here is safer whenever possible

## Next

Lab 04 (`04_COOPERATIVE_YIELD_LAB.md`) goes deeper into why a cooperative thread must always yield.
