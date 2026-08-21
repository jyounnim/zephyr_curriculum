# 11. k_poll — Waiting on Multiple Kernel Objects at Once

## What You'll Learn

Zephyr solves the "process whichever event source arrives first" problem with a general-purpose API called **`k_poll`**. It can wait on a mix of **many different kinds of kernel objects at once**: message queues, semaphores, Poll Signals (Lab 12), and more.

## Prerequisite Setup

`k_poll` is disabled by default. `prj.conf` needs the following option:

```
CONFIG_POLL=y
```

Without this option, the code compiles fine but fails at the link stage with `undefined reference to 'z_impl_k_poll'` — the `k_poll` API itself is excluded from the kernel build, so its actual implementation can't be found.

## Key Concepts

| Function/Macro | Description |
|---|---|
| `K_POLL_EVENT_STATIC_INITIALIZER(type, mode, object, tag)` | Defines a single event to watch (at compile time) |
| `k_poll(events, count, timeout)` | Waits until any one of the events in the array fires |
| `event.state` | After the wait is released, tells you which event actually fired |

## Code

```c
#include <zephyr/kernel.h>

K_MSGQ_DEFINE(temp_msgq, sizeof(int), 3, 4);
K_MSGQ_DEFINE(humidity_msgq, sizeof(int), 3, 4);

void temp_sensor_entry(void *p1, void *p2, void *p3) {
    int temp_x10 = 200;
    while (1) {
        temp_x10 += 3;
        k_msgq_put(&temp_msgq, &temp_x10, K_FOREVER);
        k_sleep(K_MSEC(1500));
    }
}

void humidity_sensor_entry(void *p1, void *p2, void *p3) {
    int humidity_x10 = 400;
    while (1) {
        humidity_x10 += 10;
        k_msgq_put(&humidity_msgq, &humidity_x10, K_FOREVER);
        k_sleep(K_MSEC(900));
    }
}

void dispatcher_entry(void *p1, void *p2, void *p3) {
    struct k_poll_event events[2] = {
        K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
                                         K_POLL_MODE_NOTIFY_ONLY, &temp_msgq, 0),
        K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
                                         K_POLL_MODE_NOTIFY_ONLY, &humidity_msgq, 0),
    };

    while (1) {
        k_poll(events, 2, K_FOREVER);

        if (events[0].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
            int value;
            k_msgq_get(&temp_msgq, &value, K_NO_WAIT);
            printk("DispatcherThread: [TEMP] %d.%d C\n", value / 10, value % 10);
            events[0].state = K_POLL_STATE_NOT_READY;   // reset for the next k_poll call
        }
        if (events[1].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
            int value;
            k_msgq_get(&humidity_msgq, &value, K_NO_WAIT);
            printk("DispatcherThread: [HUMIDITY] %d.%d %%\n", value / 10, value % 10);
            events[1].state = K_POLL_STATE_NOT_READY;
        }
    }
}

K_THREAD_DEFINE(temp_id, 1024, temp_sensor_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(humidity_id, 1024, humidity_sensor_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(dispatcher_id, 1024, dispatcher_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
```

## Run & Verify

- Confirm a single `DispatcherThread` watches both the temperature message queue (1.5s cycle) and the humidity message queue (0.9s cycle) simultaneously, printing `[TEMP]`/`[HUMIDITY]` in whatever order they arrive

## Things to Notice

- You must **manually reset** each processed event's `state` to `K_POLL_STATE_NOT_READY` on every loop iteration — since the `events` array is reused, failing to reset it can make the next `k_poll()` call mistakenly think an already-handled event has "fired" again
- `k_poll` can wait on **a mix of different event types in the same array** — `K_POLL_TYPE_SEM_AVAILABLE` (semaphore), `K_POLL_TYPE_SIGNAL` (the Poll Signal covered in Lab 12), and so on, all at once
- `k_poll` is conceptually similar to POSIX's `poll()`/`select()` — the pattern of "check at once whether any of several file descriptors (or here, kernel objects) is ready" is something Zephyr applies equally at the kernel-level IPC layer

## Troubleshooting

| Symptom | Cause / Fix |
|---|---|
| `undefined reference to 'z_impl_k_poll'` | `CONFIG_POLL=y` is missing from `prj.conf` — see "Prerequisite Setup" above |

## Next

Lab 12 (`12_POLL_SIGNAL_LAB.md`) covers Poll Signal, the lightweight event type that pairs with `k_poll`.
