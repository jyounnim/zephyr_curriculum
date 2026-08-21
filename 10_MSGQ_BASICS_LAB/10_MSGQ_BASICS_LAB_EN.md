# 10. Message Queue Basics (k_msgq)

## What You'll Learn

The basic Zephyr data structure for passing real data between threads is `k_msgq`. It exchanges fixed-size items in FIFO order.

## Key Concepts

```c
K_MSGQ_DEFINE(msgq, item_size, count, alignment_bytes);
```

| Function | Description |
|---|---|
| `k_msgq_put(&msgq, &data, timeout)` | Adds an item (returns 0 on success) |
| `k_msgq_get(&msgq, &buffer, timeout)` | Pulls an item out |

## Code

```c
#include <zephyr/kernel.h>

struct sensor_data {
    int id;
    int value_x10;   // fixed-point: real value * 10 (e.g. 205 means 20.5)
};

K_MSGQ_DEFINE(data_msgq, sizeof(struct sensor_data), 5, 4);   // 5 slots

void sensor_entry(void *p1, void *p2, void *p3) {
    int counter = 0;
    while (1) {
        struct sensor_data data;
        data.id = counter++;
        data.value_x10 = 200 + (counter % 10) * 5;

        if (k_msgq_put(&data_msgq, &data, K_MSEC(100)) == 0) {
            printk("SensorThread: sent id=%d value=%d.%d\n",
                   data.id, data.value_x10 / 10, data.value_x10 % 10);
        } else {
            printk("SensorThread: queue full, send failed\n");
        }
        k_sleep(K_MSEC(500));
    }
}

void process_entry(void *p1, void *p2, void *p3) {
    struct sensor_data received;
    while (1) {
        k_msgq_get(&data_msgq, &received, K_FOREVER);
        printk("ProcessThread: received id=%d value=%d.%d\n",
               received.id, received.value_x10 / 10, received.value_x10 % 10);
    }
}

K_THREAD_DEFINE(sensor_id, 1024, sensor_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(process_id, 1024, process_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
```

## Run & Verify

- Confirm `id` and `value` are delivered in exactly the order sent

## Things to Notice

- We handled a decimal value as a scaled integer (`value_x10`, ×10) — by default, Zephyr's `printk()` **doesn't support floating-point formats like `%f`** (you'd need to enable `CONFIG_CBPRINTF_FP_SUPPORT`, which increases code size). This kind of integer-based fixed-point trick is common in real-world embedded practice
- We gave `k_msgq_put` a timeout of `K_MSEC(100)` — try running it with `ProcessThread` removed, and confirm that once all 5 slots fill up, you start seeing "queue full, send failed"
- Beyond `k_msgq`, Zephyr also has a more flexible data structure called **`k_queue`** — while `k_msgq` can only hold fixed-size items, `k_queue` can hold items of varying sizes (via a linked-list approach). That said, it's correspondingly more low-level to use, and `k_msgq` is sufficient for most cases

## Next

Lab 11 (`11_K_POLL_LAB.md`) covers `k_poll`, which lets a single thread wait on multiple kernel objects at once.
