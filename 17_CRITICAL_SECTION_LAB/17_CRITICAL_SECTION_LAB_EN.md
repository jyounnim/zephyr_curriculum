# 17. Critical Section — irq_lock / k_sched_lock / k_spinlock

## What You'll Learn

Using the fact you learned in Lab 14 — "Zephyr's timer callback runs in an ISR context" — this lab reproduces a genuine **race condition where a thread and an ISR touch the same variable at the same time.** The competitor here is "an ISR" rather than "another core," but the conclusion is the same: short pieces of shared data need protection.

## Code (The Problem — No Protection)

```c
#include <zephyr/kernel.h>

volatile int32_t shared_counter = 0;
volatile int32_t isr_increment_count = 0;
#define ITERATIONS 300000

void timer_expiry(struct k_timer *timer_id) {
    shared_counter++;        // unprotected - runs in ISR context (see lab 14)
    isr_increment_count++;   // only the ISR touches this one, so it's safe on its own
}

K_TIMER_DEFINE(counter_timer, timer_expiry, NULL);

void counting_thread_entry(void *p1, void *p2, void *p3) {
    for (int i = 0; i < ITERATIONS; i++) {
        shared_counter++;    // unprotected - races with the ISR above
    }
    k_timer_stop(&counter_timer);
    k_sleep(K_MSEC(10));     // let any in-flight ISR settle

    int32_t expected = ITERATIONS + isr_increment_count;
    printk("Expected: %d, Actual: %d\n", expected, shared_counter);
}

K_THREAD_DEFINE(counting_id, 1024, counting_thread_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    k_timer_start(&counter_timer, K_USEC(50), K_USEC(50));
    return 0;
}
```

### Run & Verify

- Compare `Expected` and `Actual` — in most runs, **Actual comes out lower**. If the timer ISR cuts in and touches the same variable while the thread is in the middle of reading `shared_counter`, adding 1, and writing it back, one side's result gets lost

## Code (Fix — Protect With irq_lock)

```c
void timer_expiry(struct k_timer *timer_id) {
    unsigned int key = irq_lock();
    shared_counter++;
    irq_unlock(key);
    isr_increment_count++;
}

void counting_thread_entry(void *p1, void *p2, void *p3) {
    for (int i = 0; i < ITERATIONS; i++) {
        unsigned int key = irq_lock();
        shared_counter++;
        irq_unlock(key);
    }
    k_timer_stop(&counter_timer);
    k_sleep(K_MSEC(10));

    int32_t expected = ITERATIONS + isr_increment_count;
    printk("Expected: %d, Actual: %d\n", expected, shared_counter);
}
```

### Run & Verify

- This time, confirm `Expected` and `Actual` match exactly — `irq_lock()` briefly disables interrupts themselves during that section, so the ISR can't cut in

## Things to Notice

- **`k_sched_lock()` does NOT solve this problem** — `k_sched_lock` only guarantees "not preempted by another thread"; **interrupts (ISRs) still fire.** Since the competitor here is an ISR, you must use `irq_lock()` (or something stronger). Try swapping `irq_lock` for `k_sched_lock` in the code and you'll see `Actual` is still off
- `irq_lock()` is a very powerful (heavy) tool — it blocks **every interrupt** for the duration of the call, so keep the section short to minimize impact on system responsiveness
- Modern, portability-conscious Zephyr code recommends **`struct k_spinlock` + `k_spin_lock`/`k_spin_unlock`** instead of `irq_lock` — this API accounts for genuine SMP (symmetric multicore) environments too: on a single core it behaves the same as `irq_lock`, and on SMP it automatically adds spinlock handling as well:
  ```c
  struct k_spinlock lock;

  k_spinlock_key_t key = k_spin_lock(&lock);
  shared_counter++;
  k_spin_unlock(&lock, key);
  ```

## Next

Lab 18 (`18_MULTICORE_REALITY_LAB.md`) covers how Zephyr's multicore model actually works on the ESP32-S3.
