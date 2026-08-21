# 21. Producer-Consumer Synthesis Pattern

## What You'll Learn

Combining a Message Queue (Lab 10) and a mutex (Lab 09) to build a multi-producer, multi-consumer work queue.

## Code

```c
#include <zephyr/kernel.h>

#define STACK_SIZE 1024

struct work_item {
    int producer_id;
    int item_id;
};

K_MSGQ_DEFINE(work_msgq, sizeof(struct work_item), 10, 4);
K_MUTEX_DEFINE(stats_mutex);
static uint32_t total_processed = 0;

void producer_entry(void *p1, void *p2, void *p3) {
    int id = (int)(intptr_t)p1;
    int item_counter = 0;
    while (1) {
        struct work_item item = { id, item_counter++ };
        if (k_msgq_put(&work_msgq, &item, K_MSEC(100)) == 0) {
            printk("Producer%d: enqueued item %d\n", id, item.item_id);
        } else {
            printk("Producer%d: queue full, dropped item %d\n", id, item.item_id);
        }
        k_sleep(K_MSEC(300 + id * 100));
    }
}

void consumer_entry(void *p1, void *p2, void *p3) {
    int id = (int)(intptr_t)p1;
    struct work_item item;
    while (1) {
        k_msgq_get(&work_msgq, &item, K_FOREVER);
        printk("Consumer%d: processing item %d from Producer%d\n", id, item.item_id, item.producer_id);
        k_sleep(K_MSEC(200));

        k_mutex_lock(&stats_mutex, K_FOREVER);
        total_processed++;
        k_mutex_unlock(&stats_mutex);
    }
}

void stats_entry(void *p1, void *p2, void *p3) {
    while (1) {
        k_sleep(K_SECONDS(3));
        k_mutex_lock(&stats_mutex, K_FOREVER);
        printk("=== Stats: %u items processed so far ===\n", total_processed);
        k_mutex_unlock(&stats_mutex);
    }
}

K_THREAD_DEFINE(producer1_id, STACK_SIZE, producer_entry, (void *)(intptr_t)1, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(producer2_id, STACK_SIZE, producer_entry, (void *)(intptr_t)2, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(consumer1_id, STACK_SIZE, consumer_entry, (void *)(intptr_t)1, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(consumer2_id, STACK_SIZE, consumer_entry, (void *)(intptr_t)2, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(stats_id, STACK_SIZE, stats_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
```

## Run & Verify

- Confirm the two Producers push items on different cycles, and the two Consumers split up processing them
- Confirm `=== Stats: N items processed so far ===` prints every 3 seconds with a steadily increasing value

## Things to Notice

- Try removing one of the `Consumer` threads — confirm that once consumption can't keep up with production, `Producer%d: queue full, dropped item` starts appearing (a real-world reproduction of the queue-saturation scenario from Lab 10)
- This combination of passing data with `k_msgq_put`/`k_msgq_get` and protecting stats with `k_mutex_lock`/`k_mutex_unlock` is the basic skeleton reused across most real-world systems where work arrives from several places and multiple workers split it up

## Next

Lab 22 (`22_ZEPHYR_SUMMARY_LAB.md`) wraps up everything you've learned in this curriculum.
