# 20. Thread Runtime Stats — The Official CPU Usage API

## What You'll Learn

In Lab 06, you approximated CPU idle time by hand, using a low-priority thread. Zephyr provides an **official API** for this purpose — and it's a **portable, standard API that works identically on any board**, not one tied to a specific SoC vendor's SDK.

## Prerequisite Setup

Add the following options to `prj.conf`.

```
CONFIG_THREAD_RUNTIME_STATS=y
CONFIG_SCHED_THREAD_USAGE=y
```

## Key Concepts

| Function | Description |
|---|---|
| `k_thread_runtime_stats_get(thread_id, &stats)` | Retrieves the execution cycle count for a specific thread |
| `k_thread_runtime_stats_all_get(&stats)` | Retrieves the execution cycle count for the entire system (core) |
| `stats.execution_cycles` | Total CPU cycles spent executing |

Usage percentage formula: `(thread's execution_cycles × 100) / total execution_cycles`

## Code

```c
#include <zephyr/kernel.h>

#define STACK_SIZE 1024

void busy_entry(void *p1, void *p2, void *p3) {
    while (1) {
        for (volatile int i = 0; i < 500000; i++) { }   // CPU-bound work
        k_sleep(K_MSEC(50));
    }
}

void idle_ish_entry(void *p1, void *p2, void *p3) {
    while (1) {
        k_sleep(K_MSEC(500));   // mostly sleeping
    }
}

K_THREAD_DEFINE(busy_id, STACK_SIZE, busy_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(idlish_id, STACK_SIZE, idle_ish_entry, NULL, NULL, NULL, 5, 0, 0);

void monitor_entry(void *p1, void *p2, void *p3) {
    while (1) {
        k_sleep(K_SECONDS(2));

        struct k_thread_runtime_stats busy_stats, idlish_stats, cpu_stats;
        k_thread_runtime_stats_get(busy_id, &busy_stats);
        k_thread_runtime_stats_get(idlish_id, &idlish_stats);
        k_thread_runtime_stats_all_get(&cpu_stats);

        uint32_t busy_pct = (uint32_t)((busy_stats.execution_cycles * 100ULL) / cpu_stats.execution_cycles);
        uint32_t idlish_pct = (uint32_t)((idlish_stats.execution_cycles * 100ULL) / cpu_stats.execution_cycles);

        printk("CPU usage - BusyThread: %u%%, IdleIshThread: %u%%\n", busy_pct, idlish_pct);
    }
}

K_THREAD_DEFINE(monitor_id, STACK_SIZE, monitor_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
```

## Run & Verify

- Confirm `BusyThread`'s usage percentage comes out noticeably higher than `IdleIshThread`'s (the relative size difference matters more than the exact numbers)

## Things to Notice

- This API is calculated based on **cumulative values** — the code above recomputes the total cycles accumulated since boot each time, so as time goes on, you'll see the ratio between the two threads gradually converge and flatten out. If you want the usage over just "the recent interval," you need to store the previous measurement and compute the delta yourself
- Enabling `CONFIG_SCHED_THREAD_USAGE_ANALYSIS` additionally gives you finer-grained stats like `current_cycles`/`peak_cycles`/`average_cycles` (execution time analysis per scheduled-in-to-scheduled-out interval)
- Zephyr also offers a separate debugging service called the `Thread Analyzer` that uses these stats — instead of calling the API directly as in this lab, it's a convenience feature that prints stack/CPU usage for the entire thread list at once, in a standardized format

## Next

Lab 21 (`21_PRODUCER_CONSUMER_LAB.md`) combines everything you've learned so far into a Producer-Consumer pattern.
