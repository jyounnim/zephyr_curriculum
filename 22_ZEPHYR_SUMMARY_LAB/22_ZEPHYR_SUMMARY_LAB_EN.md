# 22. Full Curriculum Wrap-up

## What You'll Learn

This ties together the core Zephyr concepts covered from Lab 01 through Lab 21. Zephyr isn't just "another RTOS" — it has a consistent design philosophy: **it favors static configuration, splits threads into distinct kinds, and is built as an ecosystem you compile directly from source.** This lab revisits what you've built so far and shows exactly how that philosophy shows up in the concrete API.

## Three Pillars of Zephyr's Design Philosophy

### 1. It Favors Static Configuration

Zephyr commonly uses an approach like `K_THREAD_DEFINE`, where **everything is decided at compile time**. Even the stack must be declared ahead of time with `K_THREAD_STACK_DEFINE`. This design comes from a philosophy of "make required resources as predictable as possible before execution," in order to support extremely memory-constrained small chips alongside larger ones.

### 2. It Splits Threads Into "Kinds"

You can **choose between two entirely different scheduling rules per thread** — cooperative and preemptible (Labs 02, 04). This becomes a powerful tool when you want a clear distinction between "short work that truly must not be interrupted" and "ordinary work."

### 3. It's an Ecosystem Built Directly From Source

Because you always build directly from source via `west`, `prj.conf` (Kconfig) options can be freely toggled. As you saw in Lab 19 (Power Management), this one characteristic alone makes "automatic power saving with a single option, no code changes" possible.

## Core API Reference

| Area | Key API | One-line takeaway |
|---|---|---|
| Thread creation | `K_THREAD_DEFINE` / `k_thread_create` | Two creation styles — static and dynamic |
| Priority | Smaller number = higher priority; negative = cooperative, 0+ = preemptible | Zephyr's own dual scheduling model |
| Events/signals | `k_sem` | A unified API, callable as-is from an ISR |
| Mutual exclusion | `k_mutex` | Priority inheritance built in by default |
| Passing data | `k_msgq` | Delivers fixed-size items in FIFO order |
| Waiting on multiple sources | `k_poll` | A general-purpose API that watches semaphores, queues, and signals at once |
| Lightweight event | `k_poll_signal` | Lightly delivers a single value |
| Bit-based conditions | `k_event` | Waiting on multiple AND/OR conditions |
| Timer | `k_timer` | The callback runs in an ISR context (requires care) |
| Checking the stack | `k_thread_stack_space_get` | Reports headroom in bytes |
| Protecting data an ISR contends for | `irq_lock` / `k_spinlock` | Use this, not a scheduler lock, for data an ISR also touches |
| Power saving | `CONFIG_PM=y` | Automatic power saving with a single Kconfig line |
| CPU usage | `k_thread_runtime_stats_get` | A standardized, portable API |

## The Whole Curriculum at a Glance

| # | One-line takeaway |
|---|---|
| 01 | `K_THREAD_DEFINE` creates a thread at compile time; `k_thread_create` does it at runtime |
| 02 | The smaller the number, the higher the priority; negative is cooperative, 0 and above is preemptible |
| 03 | A thread ends itself via `return`, or gets forcibly terminated with `k_thread_abort` |
| 04 | Cooperative threads don't automatically rotate even at the same priority — `k_yield()` is required |
| 05 | If a low-priority thread holds a resource, even a high-priority one can end up waiting a long time (priority inversion) |
| 06 | The Idle Thread only runs when there's truly nothing else to do |
| 07 | `k_sem_give` can be called as-is from an ISR, with no separate version needed |
| 08 | Changing only `k_sem`'s initial/max values turns it into a counting semaphore |
| 09 | `k_mutex` has priority inheritance built in by default |
| 10 | `k_msgq` passes fixed-size data between threads |
| 11 | `k_poll` can watch multiple kinds of kernel objects at once |
| 12 | Poll Signal is a lightweight value-passing event that pairs with `k_poll` |
| 13 | `k_event` expresses AND/OR multi-conditions with a bitmask |
| 14 | A `k_timer` callback runs in an ISR context, not a thread |
| 15 | `k_thread_stack_space_get` reports stack headroom in bytes |
| 16 | Multiple mutexes must always be locked in the same order to avoid deadlock |
| 17 | Data contended by an ISR is protected with `irq_lock`/`k_spinlock` |
| 18 | The ESP32-S3 uses an AMP architecture — a separate OS image per core |
| 19 | A single `CONFIG_PM=y` enables automatic power saving with no code changes |
| 20 | `k_thread_runtime_stats_get` is a portable, standard CPU usage API |
| 21 | Combining `k_msgq` + `k_mutex` gives you a real-world Producer-Consumer pattern |
| 22 | (This file) Favoring static configuration, splitting threads into kinds, and a source-built ecosystem — these three pillars run through every API |

## Closing Exercise

- Try rebuilding some of your earlier labs (including the non-OS curriculum's GPIO, I2C, Wi-Fi examples, etc.) the Zephyr way — for instance, a project that reads a sensor and displays it on an OLED could be restructured around a `k_msgq` + `k_mutex` + `k_timer` combination
- Lab 23 (`23_CUSTOM_DEVICE_DRIVER_LAB.md`) covers Zephyr's own driver model, which cleanly separates Devicetree + Kconfig + driver code — the final applied topic in this curriculum
