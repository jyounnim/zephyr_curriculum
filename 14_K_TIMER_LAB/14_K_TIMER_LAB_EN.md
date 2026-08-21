# 14. k_timer (One-shot / Periodic)

## What You'll Learn

Zephyr's `k_timer` runs periodic/one-time work automatically via a callback. **The context its callback runs in is an important characteristic to know** — **Zephyr runs it in the system clock's interrupt handler, an ISR context.** Writing code without knowing this (e.g., calling a blocking function inside the callback) can lead to a crash.

## Key Concepts

| Function | Description |
|---|---|
| `K_TIMER_DEFINE(name, expiry_callback, stop_callback)` | Statically defines a timer |
| `k_timer_start(&timer, duration, period)` | Starts the timer. If `period` is `K_NO_WAIT` (0), it's one-shot; otherwise, periodic |
| Calling the expiry callback again | Calling `k_timer_start` again on an already-running timer restarts the countdown from the beginning (a reset) |

> ⚠️ **The expiry callback runs in an ISR context.** You must not call blocking kernel APIs like `k_sleep` or `k_mutex_lock` from inside it. Only use functions that are safe in an ISR, like `printk`.

## Code

```c
#include <zephyr/kernel.h>

void periodic_expiry(struct k_timer *timer_id) {
    printk("PeriodicTimer: tick (every 2s) [running in ISR context]\n");
}

void timeout_expiry(struct k_timer *timer_id) {
    printk("TimeoutTimer: no activity for 3s - timeout! [running in ISR context]\n");
}

K_TIMER_DEFINE(periodic_timer, periodic_expiry, NULL);
K_TIMER_DEFINE(timeout_timer, timeout_expiry, NULL);

void activity_simulator_entry(void *p1, void *p2, void *p3) {
    for (int i = 0; i < 3; i++) {
        k_sleep(K_MSEC(1500));
        printk("ActivitySimulatorThread: activity detected, resetting timeout timer\n");
        k_timer_start(&timeout_timer, K_MSEC(3000), K_NO_WAIT);   // restart = reset
    }
    printk("ActivitySimulatorThread: no more activity from now on\n");
}

K_THREAD_DEFINE(activity_id, 1024, activity_simulator_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    k_timer_start(&periodic_timer, K_MSEC(2000), K_MSEC(2000));   // period > 0 -> periodic
    k_timer_start(&timeout_timer, K_MSEC(3000), K_NO_WAIT);        // period = 0 -> one-shot
    return 0;
}
```

## Run & Verify

- Confirm `PeriodicTimer` keeps printing "tick" every 2 seconds
- Confirm `ActivitySimulatorThread` reports activity 3 times (1.5s apart), resetting `TimeoutTimer` each time
- Confirm that **about 3 seconds after** the last activity, `TimeoutTimer: no activity for 3s - timeout!` prints exactly once (since it's one-shot, it doesn't auto-restart after that)

## Things to Notice

- Because the callback runs in a genuine **ISR context**, the constraints are strict: blocking calls are outright forbidden, priority doesn't apply at all (interrupts sit outside the thread priority system), and the callback must finish as close to instantly as possible
- If a callback needs to do heavy work, structure it the same way as the ISR pattern from Lab 07: **the callback only sends a signal (a semaphore/Poll Signal), and the actual processing happens in a separate thread**
- You can also poll `k_timer_status_get(&timer)` to check how many times the timer has expired since the last check — useful for situations where you just need to check occasionally, without a callback

## Next

Lab 15 (`15_STACK_MONITORING_LAB.md`) covers monitoring thread stack usage.
