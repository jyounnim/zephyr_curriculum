# 06. The Idle Thread and CPU Idle Time

## What You'll Learn

Zephyr runs an automatically created **Idle Thread** whenever there isn't a single thread ready to run. This lab observes that effect using a very-low-priority (preemptible) user thread.

## Code

```c
#include <zephyr/kernel.h>

#define STACK_SIZE 1024

volatile uint32_t idle_counter = 0;

void idle_counter_entry(void *p1, void *p2, void *p3) {   // priority 10 - very low (close to system idle)
    while (1) {
        idle_counter++;
    }
}

void reporter_entry(void *p1, void *p2, void *p3) {        // priority 5
    while (1) {
        k_sleep(K_MSEC(1000));
        printk("IdleCounterThread incremented %u times in the last second\n", idle_counter);
        idle_counter = 0;
    }
}

void busy_burst_entry(void *p1, void *p2, void *p3) {       // priority 2 - occasionally hogs the CPU
    while (1) {
        k_sleep(K_MSEC(3000));
        printk("BusyBurstThread: starting a 1s CPU burst\n");
        int64_t start = k_uptime_get();
        while (k_uptime_get() - start < 1000) { }
        printk("BusyBurstThread: burst done\n");
    }
}

K_THREAD_DEFINE(idle_counter_id, STACK_SIZE, idle_counter_entry, NULL, NULL, NULL, 10, 0, 0);
K_THREAD_DEFINE(reporter_id, STACK_SIZE, reporter_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(busy_id, STACK_SIZE, busy_burst_entry, NULL, NULL, NULL, 2, 0, 0);

int main(void) {
    return 0;
}
```

## Run & Verify

- Under normal conditions (the 3-second window when `BusyBurstThread` is idle), confirm the `idle_counter` report comes out very large
- While `BusyBurstThread` runs its 1-second CPU burst, confirm the value drops sharply — while priority-2 (more urgent) `BusyBurstThread` occupies the core, priority-10 (most relaxed) `idle_counter_entry` barely gets to run at all

## Things to Notice

- Because `idle_counter_entry` is **preemptible** (a positive priority of 0 or above), unlike the cooperative thread in Lab 04, a higher-priority thread will automatically preempt it **even though it never yields on its own** — that's why it's safe for this code to have no `k_yield()` at all
- Zephyr's real Idle Thread has an even lower priority than this user thread — it uses the lowest priority slot the system reserves internally
- Lab 20 (`20_RUNTIME_STATS_LAB.md`) covers Zephyr's officially provided CPU usage measurement API, as an alternative to this kind of manual counting

## Next

Lab 07 (`07_ISR_SEMAPHORE_LAB.md`) covers how an interrupt (ISR) wakes a thread with a semaphore.
