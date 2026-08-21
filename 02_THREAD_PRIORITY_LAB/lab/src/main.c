// Source: 02_THREAD_PRIORITY_LAB.md

#include <zephyr/kernel.h>

#define STACK_SIZE 1024

/* ---- Part 1: two PREEMPTIBLE threads - normal preemption applies ---- */
void low_prio_preempt_entry(void *p1, void *p2, void *p3) {
    for (int i = 0; i < 5; i++) {
        printk("PreemptLow (prio 5): step %d\n", i);
        k_busy_wait(300000);   // 300ms busy-wait (no yield call, but tick interrupts still fire)
    }
    printk("PreemptLow: done\n");
}

void high_prio_preempt_entry(void *p1, void *p2, void *p3) {
    k_sleep(K_MSEC(500));      // let PreemptLow start first
    printk("PreemptHigh (prio 3): ready now - preempts PreemptLow immediately\n");
}

K_THREAD_DEFINE(low_id, STACK_SIZE, low_prio_preempt_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(high_id, STACK_SIZE, high_prio_preempt_entry, NULL, NULL, NULL, 3, 0, 0);

/* ---- Part 2: a COOPERATIVE thread that refuses to yield ---- */
void coop_low_entry(void *p1, void *p2, void *p3) {
    printk("CoopLow (prio -1): starting a long computation, will NOT yield\n");
    for (volatile long i = 0; i < 30000000; i++) { }   // long busy loop - no yield/sleep/blocking call
    printk("CoopLow: finished - only NOW does CoopHigh get a chance to run\n");
}

void coop_high_entry(void *p1, void *p2, void *p3) {
    k_sleep(K_MSEC(100));      // becomes "ready" after 100ms
    printk("CoopHigh (prio -5): this line had to wait for CoopLow to finish!\n");
}

K_THREAD_DEFINE(coop_low_id, STACK_SIZE, coop_low_entry, NULL, NULL, NULL, -1, 0, 0);
K_THREAD_DEFINE(coop_high_id, STACK_SIZE, coop_high_entry, NULL, NULL, NULL, -5, 0, 0);

int main(void) {
    return 0;
}
