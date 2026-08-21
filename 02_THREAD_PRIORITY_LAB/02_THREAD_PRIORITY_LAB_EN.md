# 02. The Priority System and Cooperative/Preemptible Threads

## What You'll Learn

**In Zephyr, the smaller the number, the higher the priority.** And there's an even more important characteristic — Zephyr clearly splits threads into two categories: **cooperative (negative priority)** and **preemptible (priority 0 or above)**. This dual scheduling model is one of Zephyr's core design elements.

## Key Concepts

| Thread Type | Priority Range | Behavior |
|---|---|---|
| **Cooperative** | Negative (e.g., -16 to -1) | Once it starts running, it **never loses the CPU to another thread until it voluntarily yields** (yield/sleep/a blocking call) — true even if an even more urgent cooperative thread is ready and waiting |
| **Preemptible** | 0 or above | Gets preempted the instant a higher-priority thread (including a cooperative one) becomes ready |

## Code

```c
#include <zephyr/kernel.h>

#define STACK_SIZE 1024

/* ---- Part 1: two PREEMPTIBLE threads - normal preemption applies ---- */
void low_prio_preempt_entry(void *p1, void *p2, void *p3) {
    for (int i = 0; i < 5; i++) {
        printk("PreemptLow (prio 5): step %d\n", i);
        k_busy_wait(300000);   // 300ms busy-wait (no yield call, but tick interrupts still fire)
    }
    printk("PreemptLow: done\n");
}

void high_prio_preempt_entry(void *p1, void *p2, void *p3) {
    k_sleep(K_MSEC(500));      // let PreemptLow start first
    printk("PreemptHigh (prio 3): ready now - preempts PreemptLow immediately\n");
}

K_THREAD_DEFINE(low_id, STACK_SIZE, low_prio_preempt_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(high_id, STACK_SIZE, high_prio_preempt_entry, NULL, NULL, NULL, 3, 0, 0);

/* ---- Part 2: a COOPERATIVE thread that refuses to yield ---- */
void coop_low_entry(void *p1, void *p2, void *p3) {
    printk("CoopLow (prio -1): starting a long computation, will NOT yield\n");
    for (volatile long i = 0; i < 30000000; i++) { }   // long busy loop - no yield/sleep/blocking call
    printk("CoopLow: finished - only NOW does CoopHigh get a chance to run\n");
}

void coop_high_entry(void *p1, void *p2, void *p3) {
    k_sleep(K_MSEC(100));      // becomes "ready" after 100ms
    printk("CoopHigh (prio -5): this line had to wait for CoopLow to finish!\n");
}

K_THREAD_DEFINE(coop_low_id, STACK_SIZE, coop_low_entry, NULL, NULL, NULL, -1, 0, 0);
K_THREAD_DEFINE(coop_high_id, STACK_SIZE, coop_high_entry, NULL, NULL, NULL, -5, 0, 0);

int main(void) {
    return 0;
}
```

## Run & Verify

- **Part 1 (preemptible)**: while `PreemptLow` is printing its 5 steps, confirm `PreemptHigh (prio 3): ready now...` cuts in right around the 500ms mark — a thread with a smaller (more urgent) priority number preempts the instant it's ready
- **Part 2 (cooperative)**: `CoopHigh` becomes "ready" after 100ms, but confirm the `CoopHigh (prio -5): this line had to wait...` log appears much later (several seconds later, right after `CoopLow` finishes its computation) — **even though it has the more urgent priority (`-5` < `-1`), it never preempted at all**

## Things to Notice

- Try changing just `CoopLow`'s priority from `-1` to `2` (preemptible) — leave everything else unchanged, and this time you'll see `CoopHigh` cut in immediately around the 100ms mark. **A single change of sign (negative vs. 0-or-above) changes the entire scheduling behavior**
- Because of this property, whenever you use a cooperative thread in Zephyr, you must make sure it **calls `k_yield()` on its own, frequently** (covered in Lab 04) — otherwise every other thread (including ones with higher priority than itself) can starve
- Being able to mix "this thread is cooperative, that one is preemptible" per thread is a powerful piece of flexibility — it lets you design precisely, making only the truly-must-not-be-interrupted short work cooperative while leaving everything else preemptible

## Next

Lab 03 (`03_THREAD_LIFECYCLE_LAB.md`) covers dynamically creating and terminating threads at runtime.
