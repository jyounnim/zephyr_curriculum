# 01. Thread Creation Basics — K_THREAD_DEFINE / k_thread_create

## What You'll Learn

In Zephyr, the concept corresponding to a task is a **Thread**. Zephyr much more commonly uses **`K_THREAD_DEFINE`, which defines a thread statically at compile time**. This is a good illustration of Zephyr's design philosophy of favoring static configuration — making the resources needed at runtime (threads, stacks, etc.) as predictable as possible ahead of time.

## Key Concepts

| Approach | Description |
|---|---|
| `K_THREAD_DEFINE(id, stack_size, function, p1, p2, p3, priority, options, start_delay)` | Defines a thread at compile time — it's already a scheduling candidate before `main()` even starts |
| `k_thread_create(&data, stack, stack_size, function, p1, p2, p3, priority, options, start_delay)` | Dynamically creates a thread at runtime |
| Stack size unit | **Bytes** |

## Code

```c
#include <zephyr/kernel.h>

#define STACK_SIZE 1024

void thread_a_entry(void *p1, void *p2, void *p3) {
    while (1) {
        printk("ThreadA running\n");
        k_sleep(K_MSEC(1000));
    }
}

void thread_b_entry(void *p1, void *p2, void *p3) {
    while (1) {
        printk("ThreadB running\n");
        k_sleep(K_MSEC(700));
    }
}

// Static, compile-time thread definitions
K_THREAD_DEFINE(thread_a_id, STACK_SIZE, thread_a_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(thread_b_id, STACK_SIZE, thread_b_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    printk("main: started, threads are already running\n");
    return 0;
}
```

## Run & Verify

- `west build -p always -b sr100_rdk/sr100/m55 <lab path>` → flash via `openocd_flash.py` → serial terminal at 230400bps 8N1
- Confirm `ThreadA` (1s interval) and `ThreadB` (0.7s interval) each print independently
- **Something to notice**: the `main: started...` log may appear *after* the first output from `ThreadA`/`ThreadB` — because threads created with `K_THREAD_DEFINE` are already scheduling candidates before `main()` even runs

## Things to Notice

- Try changing the last argument (start delay) from `0` to `2000` — you can confirm that thread only becomes eligible for scheduling 2 seconds later
- `K_THREAD_DEFINE` fits situations where the number of threads is fixed at the time you write the code (the most common case). If the count changes at runtime (like work-queue workers), you need `k_thread_create`, covered in Lab 03

## Next

Lab 02 (`02_THREAD_PRIORITY_LAB.md`) covers Zephyr's priority system and the cooperative/preemptible thread distinction.
