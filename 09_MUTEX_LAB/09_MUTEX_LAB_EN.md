# 09. k_mutex vs k_sem — Priority Inheritance

## What You'll Learn

This mitigates Lab 05's priority inversion problem by swapping `k_sem` for `k_mutex`. Zephyr's `k_mutex` has **priority inheritance built in by default** — the mutex is a distinct type from the semaphore from the start, and it always includes the inheritance logic with no extra configuration needed.

## Key Concepts

| Item | `k_sem` | `k_mutex` |
|---|---|---|
| Owner concept | None | Yes |
| Priority inheritance | None | **Always applied by default** |
| Use in an ISR | Allowed (`k_sem_give` can be called from an ISR) | **Forbidden** (both lock and unlock must happen only in thread context) |
| Recursive locking (same thread locking again) | Not supported | Supported — must be unlocked the same number of times it was locked |
| Primary use | Events/signals, ISR↔thread synchronization | Mutual exclusion for a shared resource |

## Code

This is Lab 05's code with `k_sem` swapped for `k_mutex`.

```c
#include <zephyr/kernel.h>

#define STACK_SIZE 1024

K_MUTEX_DEFINE(lock_mutex);   // priority inheritance is built in

void thread_l_entry(void *p1, void *p2, void *p3) {   // priority 8 (low) - holds the resource
    while (1) {
        k_mutex_lock(&lock_mutex, K_FOREVER);
        printk("ThreadL: acquired the resource\n");
        for (int i = 0; i < 20; i++) {
            k_busy_wait(50000);
        }
        printk("ThreadL: releasing the resource\n");
        k_mutex_unlock(&lock_mutex);
        k_sleep(K_MSEC(500));
    }
}

void thread_m_entry(void *p1, void *p2, void *p3) {    // priority 5 (medium) - does NOT need the resource
    while (1) {
        k_sleep(K_MSEC(300));
        printk("ThreadM: doing unrelated work (tries to preempt ThreadL)\n");
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
        k_mutex_lock(&lock_mutex, K_FOREVER);
        int64_t waited_ms = k_uptime_get() - start;
        printk("ThreadH: acquired after waiting %lld ms\n", waited_ms);
        k_mutex_unlock(&lock_mutex);
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

- Compare the `ThreadH: acquired after waiting XXX ms` value against the result from Lab 05 (the `k_sem` version) — this time, the wait should be shorter and more consistent
- What's happening under the hood: the moment `ThreadH` starts waiting on the mutex, Zephyr temporarily boosts the priority of `ThreadL` (the current mutex holder) up to `ThreadH`'s level (2). While that's in effect, priority-5 `ThreadM` can no longer preempt `ThreadL`

## Things to Notice

- A Kconfig option called `CONFIG_PRIORITY_CEILING` can cap how far a priority can be inherited (unlimited by default) — adjustable via `prj.conf`
- If you nest multiple mutex locks, note that **you must unlock them in the reverse order you locked them** for priority inheritance to work as intended (this is explicitly called out as a caution in the official documentation)
- In Lab 07, an ISR was able to call `k_sem_give` directly, but with `k_mutex`, neither Lock nor Unlock can **ever be called from an ISR** — because the "who currently holds this mutex" ownership concept doesn't naturally apply to an interrupt context

## Next

Lab 10 (`10_MSGQ_BASICS_LAB.md`) covers the Message Queue, for passing real data between threads.
