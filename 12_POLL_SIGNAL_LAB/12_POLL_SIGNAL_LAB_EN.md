# 12. Poll Signal — A Lightweight Event

## What You'll Learn

The Zephyr feature for lightly passing a single value is **Poll Signal**. Without needing a separate queue or semaphore object, it pairs with `k_poll` to deliver a "lightweight, one-shot event that carries a value."

## Prerequisite Setup

Poll Signal is part of the `k_poll` feature, so it's disabled by default the same way. `prj.conf` needs the following option:

```
CONFIG_POLL=y
```

This single option enables both the `k_poll()` and `k_poll_signal_raise()` APIs — just like in Lab 11, leaving it out produces a link-stage error such as `undefined reference to 'z_impl_k_poll_signal_raise'`.

## Key Concepts

| Function/Macro | Description |
|---|---|
| `K_POLL_SIGNAL_INITIALIZER(obj)` | A compile-time initializer macro for a Poll Signal |
| `k_poll_signal_raise(&signal, result_value)` | Raises the signal and delivers an integer result value |
| `k_poll_signal_check(&signal, &signaled, &result)` | Checks whether the signal has fired, and what its result value is |
| `k_poll_signal_reset(&signal)` | Resets the signal to its initial state for reuse |

## Code

```c
#include <zephyr/kernel.h>

struct k_poll_signal my_signal = K_POLL_SIGNAL_INITIALIZER(my_signal);

void producer_entry(void *p1, void *p2, void *p3) {
    int counter = 0;
    while (1) {
        counter++;
        k_sleep(K_MSEC(1000));
        printk("ProducerThread: raising signal with value=%d\n", counter);
        k_poll_signal_raise(&my_signal, counter);
    }
}

void consumer_entry(void *p1, void *p2, void *p3) {
    struct k_poll_event event = K_POLL_EVENT_INITIALIZER(
        K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &my_signal);

    while (1) {
        k_poll(&event, 1, K_FOREVER);

        unsigned int signaled;
        int result;
        k_poll_signal_check(&my_signal, &signaled, &result);
        printk("ConsumerThread: received value=%d\n", result);

        k_poll_signal_reset(&my_signal);
        event.state = K_POLL_STATE_NOT_READY;
    }
}

K_THREAD_DEFINE(consumer_id, 1024, consumer_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(producer_id, 1024, producer_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
```

## Run & Verify

- Confirm `ConsumerThread` receives exactly the `counter` value `ProducerThread` sends every second

## Things to Notice

- `K_POLL_SIGNAL_INITIALIZER(obj)` is a **compile-time initializer macro** — there's no need to call `k_poll_signal_init()` separately at runtime. `producer_entry`/`consumer_entry` are already running before `main()` even executes (via `K_THREAD_DEFINE`), so if `my_signal` were initialized at runtime instead, you could get a race condition where a thread accesses it before it's initialized — the compile-time initializer avoids this entirely
- A single Poll Signal on its own might not seem very different from a semaphore, but its real value shows up **when combined with `k_poll` from Lab 11** — you can mix message queues, semaphores, and Poll Signals all into the same `k_poll` array, creating one unified wait point for "process whichever of these fires"
- `k_poll_signal_raise` can also be called from an ISR — like `k_sem_give` in Lab 07, you can use it whenever you want to pass a value from an interrupt to a thread
- You must call `k_poll_signal_reset` before reusing it — without resetting, the next `k_poll()` call will mistakenly think "the signal has already fired" and return immediately

## Troubleshooting

| Symptom | Cause / Fix |
|---|---|
| `undefined reference to 'z_impl_k_poll_signal_raise'` or `z_impl_k_poll` | `CONFIG_POLL=y` is missing from `prj.conf` — see "Prerequisite Setup" above |
| A compile error like `error: type defaults to 'int' in declaration of 'K_POLL_SIGNAL_DEFINE'` | `K_POLL_SIGNAL_DEFINE` is not a real macro — declare it as `struct k_poll_signal my_signal = K_POLL_SIGNAL_INITIALIZER(my_signal);` instead (see the code above) |

## Next

Lab 13 (`13_K_EVENT_LAB.md`) covers `k_event`, which manages multiple conditions as bits.
