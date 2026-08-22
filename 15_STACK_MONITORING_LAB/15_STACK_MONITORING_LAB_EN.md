# 15. Stack Usage Monitoring

## What You'll Learn

The Zephyr API for checking a thread's stack headroom at runtime is `k_thread_stack_space_get`. The unit is **bytes**.

## Prerequisite Setup

`prj.conf` needs **both** of the following options.

```
CONFIG_THREAD_STACK_INFO=y
CONFIG_INIT_STACKS=y
```

Inside the Zephyr kernel source, the actual implementation of `k_thread_stack_space_get` is only compiled in when **both** options are enabled at the same time. If you enable only `CONFIG_THREAD_STACK_INFO` and leave out `CONFIG_INIT_STACKS`, the code compiles fine but fails at the link stage with `undefined reference to 'z_impl_k_thread_stack_space_get'`.

## Key Concepts

```c
int k_thread_stack_space_get(const struct k_thread *thread, size_t *unused_ptr);
```

- On success, `unused_ptr` is filled with the **number of free bytes** remaining (smaller = more dangerous)

## Code

```c
#include <zephyr/kernel.h>
#include <string.h>

#define STACK_SIZE 2048

void light_entry(void *p1, void *p2, void *p3) {
    while (1) {
        int small_var = 0;
        small_var++;
        k_sleep(K_MSEC(1000));
    }
}

void recursive_work(int depth) {
    char buffer[256];   // consumes stack on every call
    memset(buffer, 0, sizeof(buffer));
    if (depth > 0) {
        recursive_work(depth - 1);
    }
}

void heavy_entry(void *p1, void *p2, void *p3) {
    while (1) {
        recursive_work(4);
        k_sleep(K_MSEC(1000));
    }
}

K_THREAD_DEFINE(light_id, STACK_SIZE, light_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(heavy_id, STACK_SIZE, heavy_entry, NULL, NULL, NULL, 5, 0, 0);

void monitor_entry(void *p1, void *p2, void *p3) {
    size_t light_unused, heavy_unused;
    while (1) {
        k_sleep(K_MSEC(2000));
        k_thread_stack_space_get(light_id, &light_unused);
        k_thread_stack_space_get(heavy_id, &heavy_unused);
        printk("Stack headroom (bytes) - LightThread: %u, HeavyThread: %u\n",
               (unsigned int)light_unused, (unsigned int)heavy_unused);
    }
}

K_THREAD_DEFINE(monitor_id, 1024, monitor_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
```

## Run & Verify

- Confirm `HeavyThread`'s (recursion + a 256-byte buffer) headroom is noticeably smaller than `LightThread`'s

## Things to Notice

- Try shrinking `HeavyThread`'s `STACK_SIZE` from `2048` to `512` — the headroom will approach 0, or you may trigger a `Stack overflow`-class fatal error from Zephyr's built-in stack protection (a hardware MPU guard or a canary, depending on your board/config) (restore it to 2048 afterward)
- Always remember: Zephyr specifies stack size in **bytes** — this is worth double-checking whenever you're porting code from another embedded platform, where the unit may differ
- With the advanced option `CONFIG_THREAD_RUNTIME_STACK_SAFETY`, you can enable active monitoring that fires a callback the *moment* the stack headroom drops below a certain threshold — more immediate than the periodic polling approach used in this lab

## Troubleshooting

| Symptom | Cause / Fix |
|---|---|
| `undefined reference to 'z_impl_k_thread_stack_space_get'` | `CONFIG_INIT_STACKS=y` is missing from `prj.conf` — `CONFIG_THREAD_STACK_INFO=y` alone isn't enough, both options are required together. See "Prerequisite Setup" above |

## Next

Lab 16 (`16_DEADLOCK_LAB.md`) covers deadlock, which can occur when using multiple mutexes.
