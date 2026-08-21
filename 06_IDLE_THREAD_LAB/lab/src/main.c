// Source: 06_IDLE_THREAD_LAB.md

#include <zephyr/kernel.h>

#define STACK_SIZE 1024

volatile uint32_t idle_counter = 0;

void idle_counter_entry(void *p1, void *p2, void *p3) {   // priority 10 - very low (close to system idle)
    while (1) {
        idle_counter++;
    }
}

void reporter_entry(void *p1, void *p2, void *p3) {        // priority 5
    while (1) {
        k_sleep(K_MSEC(1000));
        printk("IdleCounterThread incremented %u times in the last second\n", idle_counter);
        idle_counter = 0;
    }
}

void busy_burst_entry(void *p1, void *p2, void *p3) {       // priority 2 - occasionally hogs the CPU
    while (1) {
        k_sleep(K_MSEC(3000));
        printk("BusyBurstThread: starting a 1s CPU burst\n");
        int64_t start = k_uptime_get();
        while (k_uptime_get() - start < 1000) { }
        printk("BusyBurstThread: burst done\n");
    }
}

K_THREAD_DEFINE(idle_counter_id, STACK_SIZE, idle_counter_entry, NULL, NULL, NULL, 10, 0, 0);
K_THREAD_DEFINE(reporter_id, STACK_SIZE, reporter_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(busy_id, STACK_SIZE, busy_burst_entry, NULL, NULL, NULL, 2, 0, 0);

int main(void) {
    return 0;
}
