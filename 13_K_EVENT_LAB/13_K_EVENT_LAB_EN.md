# 13. k_event — Waiting on Multiple Conditions

## What You'll Learn

The Zephyr feature for waiting on multiple conditions is `k_event`. It manages multiple conditions with a 32-bit bitmask, and you can wait until "any one" (OR) or "all" (AND) of them are satisfied.

## Prerequisite Setup

`prj.conf` needs the following option:

```
CONFIG_EVENTS=y
```

## Key Concepts

| Function | Description |
|---|---|
| `K_EVENT_DEFINE(name)` | Statically defines an Event object |
| `k_event_post(&event, bits)` | **Adds** bits (OR) — turns on new bits while leaving existing ones alone |
| `k_event_wait(&event, bits, reset, timeout)` | Returns as soon as **any single one** of the specified bits is set |
| `k_event_wait_all(&event, bits, reset, timeout)` | Returns only once **all** of the specified bits are set |

## Code

```c
#include <zephyr/kernel.h>

#define WIFI_READY_BIT     BIT(0)
#define SENSOR_READY_BIT   BIT(1)
#define STORAGE_READY_BIT  BIT(2)
#define ALL_READY_BITS     (WIFI_READY_BIT | SENSOR_READY_BIT | STORAGE_READY_BIT)

K_EVENT_DEFINE(system_events);

void wifi_init_entry(void *p1, void *p2, void *p3) {
    printk("WifiInitThread: initializing...\n");
    k_sleep(K_MSEC(1500));
    printk("WifiInitThread: ready\n");
    k_event_post(&system_events, WIFI_READY_BIT);
}

void sensor_init_entry(void *p1, void *p2, void *p3) {
    printk("SensorInitThread: initializing...\n");
    k_sleep(K_MSEC(800));
    printk("SensorInitThread: ready\n");
    k_event_post(&system_events, SENSOR_READY_BIT);
}

void storage_init_entry(void *p1, void *p2, void *p3) {
    printk("StorageInitThread: initializing...\n");
    k_sleep(K_MSEC(2200));
    printk("StorageInitThread: ready\n");
    k_event_post(&system_events, STORAGE_READY_BIT);
}

void main_app_entry(void *p1, void *p2, void *p3) {
    printk("MainThread: waiting for all subsystems...\n");
    k_event_wait_all(&system_events, ALL_READY_BITS, false, K_FOREVER);
    printk("MainThread: all subsystems ready! starting main application...\n");
    while (1) {
        k_sleep(K_MSEC(5000));
        printk("MainThread: running normally\n");
    }
}

K_THREAD_DEFINE(main_app_id, 1024, main_app_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(wifi_id, 1024, wifi_init_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(sensor_id, 1024, sensor_init_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(storage_id, 1024, storage_init_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
```

## Run & Verify

- Even though the three initialization threads finish at different times (1.5s / 0.8s / 2.2s), confirm `MainThread` prints "all subsystems ready" **timed to the slowest one, StorageInitThread (2.2s)**

## Things to Notice

- Try changing `k_event_wait_all` to `k_event_wait` (any one) — you can confirm `MainThread` proceeds the instant `SensorInitThread` (the fastest, at 0.8s) finishes
- Passing `true` for the 3rd argument (`reset`) to `k_event_wait`/`k_event_wait_all` clears the event object's bits the moment the wait is released — but the official documentation warns that **using reset when multiple threads are waiting on the same Event object at once can create a race condition**. For that situation, there are also `k_event_wait_safe`/`k_event_wait_all_safe` variants that handle the reset atomically and safely
- Remember the difference between `k_event_post` (add/merge) and `k_event_set` (overwrite) — in a situation like this one, where "several places each set only their own bit," you must use `k_event_post`. Using `k_event_set` could wipe out bits that another thread had already turned on

## Next

Lab 14 (`14_K_TIMER_LAB.md`) covers `k_timer`, for periodic/one-time work.
