# 05. Reproducing Priority Inversion

## What You'll Learn

This reproduces a classic RTOS problem, priority inversion, in Zephyr. Here, all three threads are made **preemptible (priority 0 or above)**, so you can view this problem separately from the cooperative-thread quirks covered in Lab 04.

> Remember, in Zephyr, the smaller the number, the higher the priority — `ThreadH` (2) is the most urgent, and `ThreadL` (8) is the most relaxed.

## Code

```c
#include <zephyr/kernel.h>

#define STACK_SIZE 1024

K_SEM_DEFINE(lock_sem, 1, 1);   // binary semaphore used as a simple lock (not priority-aware)

void thread_l_entry(void *p1, void *p2, void *p3) {   // priority 8 (low) - holds the resource
    while (1) {
        k_sem_take(&lock_sem, K_FOREVER);
        printk("ThreadL: acquired the resource\n");
        for (int i = 0; i < 20; i++) {
            k_busy_wait(50000);   // simulate real work, 50ms chunks
        }
        printk("ThreadL: releasing the resource\n");
        k_sem_give(&lock_sem);
        k_sleep(K_MSEC(500));
    }
}

void thread_m_entry(void *p1, void *p2, void *p3) {    // priority 5 (medium) - does NOT need the resource
    while (1) {
        k_sleep(K_MSEC(300));
        printk("ThreadM: doing unrelated work (preempts ThreadL)\n");
        for (int i = 0; i < 5; i++) {
            k_busy_wait(100000);
        }
    }
}

void thread_h_entry(void *p1, void *p2, void *p3) {    // priority 2 (high) - urgently needs the resource
    while (1) {
        k_sleep(K_MSEC(1000));
        int64_t start = k_uptime_get();
        printk("ThreadH: requesting the resource...\n");
        k_sem_take(&lock_sem, K_FOREVER);
        int64_t waited_ms = k_uptime_get() - start;
        printk("ThreadH: acquired after waiting %lld ms\n", waited_ms);
        k_sem_give(&lock_sem);
    }
}

K_THREAD_DEFINE(l_id, STACK_SIZE, thread_l_entry, NULL, NULL, NULL, 8, 0, 0);
K_THREAD_DEFINE(m_id, STACK_SIZE, thread_m_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(h_id, STACK_SIZE, thread_h_entry, NULL, NULL, NULL, 2, 0, 0);

int main(void) {
    return 0;
}
```

## Run & Verify

- Watch the `ThreadH: acquired after waiting XXX ms` value — because `ThreadM` keeps cutting in, `ThreadL` holds the resource for a long time, and `ThreadH`, despite being the most urgent, ends up waiting longer than you'd expect
- Try temporarily deleting the `K_THREAD_DEFINE(m_id, ...)` line that creates `ThreadM`, then rebuild — without `ThreadM`'s interference, `ThreadH`'s wait time becomes much shorter and more consistent

## Things to Notice

- `k_sem` is a simple signaling device with no concept of an "owner," so the scheduler has no way to do `ThreadL` the favor of boosting its priority
- To fix this, you need to switch to `k_mutex`, as in Lab 09 — Zephyr's `k_mutex` has **priority inheritance built in by default**, so simply swapping `k_sem` for `k_mutex`, with no extra configuration, mitigates the problem

## Next

Lab 06 (`06_IDLE_THREAD_LAB.md`) covers the Idle Thread and CPU idle time.
