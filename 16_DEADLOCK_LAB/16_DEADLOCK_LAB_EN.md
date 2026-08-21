# 16. Reproducing and Avoiding Deadlock

## What You'll Learn

This reproduces a classic deadlock problem using Zephyr's `k_mutex`. The underlying principle — that locking two mutexes in different orders can lead to a circular wait (deadlock) — is a universal problem that has nothing to do with which RTOS you're using.

> ⚠️ The first piece of code below is intentionally written to hang.

## Code (Reproducing the Problem)

```c
#include <zephyr/kernel.h>

K_MUTEX_DEFINE(mutex_x);
K_MUTEX_DEFINE(mutex_y);

void thread_a_entry(void *p1, void *p2, void *p3) {
    while (1) {
        printk("ThreadA: taking MutexX\n");
        k_mutex_lock(&mutex_x, K_FOREVER);
        printk("ThreadA: got MutexX, now taking MutexY\n");
        k_sleep(K_MSEC(100));   // give ThreadB time to grab MutexY first
        k_mutex_lock(&mutex_y, K_FOREVER);

        printk("ThreadA: got both mutexes, working...\n");
        k_mutex_unlock(&mutex_y);
        k_mutex_unlock(&mutex_x);
        k_sleep(K_MSEC(1000));
    }
}

void thread_b_entry(void *p1, void *p2, void *p3) {
    while (1) {
        printk("ThreadB: taking MutexY\n");
        k_mutex_lock(&mutex_y, K_FOREVER);
        printk("ThreadB: got MutexY, now taking MutexX\n");
        k_sleep(K_MSEC(100));   // give ThreadA time to grab MutexX first
        k_mutex_lock(&mutex_x, K_FOREVER);

        printk("ThreadB: got both mutexes, working...\n");
        k_mutex_unlock(&mutex_x);
        k_mutex_unlock(&mutex_y);
        k_sleep(K_MSEC(1000));
    }
}

K_THREAD_DEFINE(a_id, 1024, thread_a_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(b_id, 1024, thread_b_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
```

### Run & Verify

- Confirm the log freezes right at this pattern:
  ```
  ThreadA: taking MutexX
  ThreadB: taking MutexY
  ThreadA: got MutexX, now taking MutexY
  ThreadB: got MutexY, now taking MutexX
  (stuck here forever)
  ```

## Code (Fix — Consistent Lock Ordering)

```c
void thread_b_entry(void *p1, void *p2, void *p3) {
    while (1) {
        printk("ThreadB: taking MutexX\n");
        k_mutex_lock(&mutex_x, K_FOREVER);   // X first, same as ThreadA
        printk("ThreadB: got MutexX, now taking MutexY\n");
        k_sleep(K_MSEC(100));
        k_mutex_lock(&mutex_y, K_FOREVER);

        printk("ThreadB: got both mutexes, working...\n");
        k_mutex_unlock(&mutex_y);
        k_mutex_unlock(&mutex_x);
        k_sleep(K_MSEC(1000));
    }
}
```

### Run & Verify

- Confirm both threads now reach "got both mutexes, working..." normally and repeat forever

## Things to Notice

- This problem and its fix demonstrate that **lock ordering** (keeping locking order consistent) is a universal avoidance strategy that works regardless of which RTOS you're using
- Zephyr also offers a timeout for `k_mutex_lock` (e.g., `K_MSEC(500)` instead of `K_FOREVER`), which you can use to avoid deadlock the same way — treat a failed acquisition as a failure and retry
- The priority inheritance you learned about in Lab 09 is unrelated to this deadlock problem — since the two threads are each permanently waiting on a resource the other holds, boosting either one's priority does nothing to break the cycle

## Next

Lab 17 (`17_CRITICAL_SECTION_LAB.md`) compares the different ways to protect a very short piece of shared data.
