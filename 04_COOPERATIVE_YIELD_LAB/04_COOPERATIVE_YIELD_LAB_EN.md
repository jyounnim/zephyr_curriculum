# 04. Cooperative Threads and k_yield — Why You Must Always Yield

## What You'll Learn

In Lab 02, you confirmed that "a cooperative thread is never preempted until it yields on its own." This time, you'll see that this holds **even between two cooperative threads at the same priority.** Zephyr's automatic time-slicing (rotating between threads of equal priority) only applies to preemptible threads — **cooperative threads are entirely excluded from this automatic rotation.**

## Key Concepts

- `k_yield()`: the current thread voluntarily gives up the CPU — handing its turn to another Ready thread of equal (or higher) priority
- Zephyr's time-slicing (`CONFIG_TIMESLICING`) only applies to **preemptible threads** — a cooperative thread, by definition, "keeps running until it yields on its own," so it was never a candidate for forced rotation to begin with

## Code (The Problem — No Yielding)

```c
#include <zephyr/kernel.h>

#define STACK_SIZE 1024

void coop_a_entry(void *p1, void *p2, void *p3) {
    long count = 0;
    while (1) {
        count++;
        if (count % 2000000 == 0) {
            printk("CoopA: count=%ld\n", count);
        }
        // no yield - if this thread got the CPU first, it keeps it forever
    }
}

void coop_b_entry(void *p1, void *p2, void *p3) {
    long count = 0;
    while (1) {
        count++;
        if (count % 2000000 == 0) {
            printk("CoopB: count=%ld\n", count);
        }
    }
}

K_THREAD_DEFINE(coop_a_id, STACK_SIZE, coop_a_entry, NULL, NULL, NULL, -1, 0, 0);
K_THREAD_DEFINE(coop_b_id, STACK_SIZE, coop_b_entry, NULL, NULL, NULL, -1, 0, 0);

int main(void) {
    return 0;
}
```

### Run & Verify

- Even though both are priority `-1` (cooperative, identical), confirm that **only `CoopA` keeps printing while `CoopB` never prints a single line** — because whichever one grabbed the CPU first never lets go. (This isn't a crash or a reboot — `CoopB` is simply, quietly starving)

## Code (Fix — Call k_yield Periodically)

```c
void coop_a_entry(void *p1, void *p2, void *p3) {
    long count = 0;
    while (1) {
        count++;
        if (count % 2000000 == 0) {
            printk("CoopA: count=%ld\n", count);
            k_yield();   // voluntarily let CoopB (equal priority) have a turn
        }
    }
}

void coop_b_entry(void *p1, void *p2, void *p3) {
    long count = 0;
    while (1) {
        count++;
        if (count % 2000000 == 0) {
            printk("CoopB: count=%ld\n", count);
            k_yield();
        }
    }
}
```

### Run & Verify

- Confirm `CoopA` and `CoopB` now print alternately — because calling `k_yield()` is an explicit signal that says, "give other threads a turn now"

## Things to Notice

- You can get a similar effect by using `k_sleep(K_MSEC(0))` or `k_sleep(K_MSEC(10))` instead of `k_yield()` — though there's a difference: `k_sleep` guarantees "wait at least this long," while `k_yield()` means "just give up my turn right now; if there's no one else, resume immediately"
- Starvation among Zephyr's cooperative threads **simply halts quietly by default, with no automatic detection or recovery mechanism.** This means the developer carries significant responsibility for carefully designing yield points when using cooperative threads
- In practice, it's safer to reserve cooperative priority for "very short work that truly must not be interrupted," and use preemptible (0 or above) as the default for everything else — Zephyr guides from Nordic and others also recommend using cooperative threads only in a limited way

## Next

Lab 05 (`05_PRIORITY_INVERSION_LAB.md`) reproduces the priority inversion problem in Zephyr.
