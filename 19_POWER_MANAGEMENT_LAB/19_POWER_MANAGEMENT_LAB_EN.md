# 19. Zephyr Power Management (prj.conf)

## What You'll Learn

Zephyr is an **ecosystem you build directly from source** — flip a single option in `prj.conf`, and the system automatically enters power-saving mode with no code changes at all.

## Prerequisite Setup

Add the following option to `prj.conf`.

```
CONFIG_PM=y
```

## Key Concepts

- With `CONFIG_PM=y` enabled, Zephyr's Idle Thread calculates "how much time remains until the next thread needs to wake up," and **automatically selects and enters the deepest power state it can safely use** for that duration
- Application code just uses `k_sleep()` as usual — **no separate API calls are needed to enter/exit low-power mode.** This is the key takeaway of this lab

## Code

```c
#include <zephyr/kernel.h>

void periodic_entry(void *p1, void *p2, void *p3) {
    while (1) {
        printk("PeriodicThread: awake, doing some work...\n");
        k_sleep(K_SECONDS(3));   // just a normal sleep - no PM-related code
    }
}

K_THREAD_DEFINE(periodic_id, 1024, periodic_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
```

## Run & Verify

- Run once without `CONFIG_PM=y` and once with it, and confirm the visible log output is identical either way — **the application's behavior itself is exactly the same.** The difference isn't visible on the surface; it's whether the system is actually in a low-power state during that 3-second wait
- If you have precision current-measurement equipment (a current probe, etc.), you could compare whether current draw during the wait period differs with `CONFIG_PM=y` on vs. off (this document alone can't confirm it — take this as background info)

## Things to Notice

- Rather than the developer explicitly deciding "when, and for how long, to enter power-saving mode," Zephyr **lets the kernel figure it out** — this design is possible because the scheduler already knows the next wakeup point (the next `k_sleep` expiry, the next timer expiry, etc.)
- That said, which power states are actually supported **depends on the SoC's Power Management driver implementation**. SR110's product brief lists four power tiers: active (sub-100 mW), low-power (sub-10 mW), ULP AON mode (sub-100 µW), and power-down — but whether all four are exposed through Zephyr's `CONFIG_PM` state machine on this specific SDK build is a separate question from whether the SoC itself supports them at the hardware level. TODO/VERIFY: confirm which of these four states `CONFIG_PM=y` actually drives into on `sr100_rdk/sr100/m55` versus which require the vendor-specific power APIs instead
- For finer control, you can force a specific power state from code with `pm_state_force()`, or turn individual peripherals (sensors, communication modules, etc.) on/off separately with `pm_device_action_run()` — but the core takeaway of this lab is the automation itself: **power saving just works by default, with no extra effort**

## Next

Lab 20 (`20_RUNTIME_STATS_LAB.md`) covers Zephyr's officially provided per-thread CPU usage API.
