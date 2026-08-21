// Source: 20_RUNTIME_STATS_LAB.md

#include <zephyr/kernel.h>

#define STACK_SIZE 1024

void busy_entry(void *p1, void *p2, void *p3) {
    while (1) {
        for (volatile int i = 0; i < 500000; i++) { }   // CPU-bound work
        k_sleep(K_MSEC(50));
    }
}

void idle_ish_entry(void *p1, void *p2, void *p3) {
    while (1) {
        k_sleep(K_MSEC(500));   // mostly sleeping
    }
}

K_THREAD_DEFINE(busy_id, STACK_SIZE, busy_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(idlish_id, STACK_SIZE, idle_ish_entry, NULL, NULL, NULL, 5, 0, 0);

void monitor_entry(void *p1, void *p2, void *p3) {
    while (1) {
        k_sleep(K_SECONDS(2));

        struct k_thread_runtime_stats busy_stats, idlish_stats, cpu_stats;
        k_thread_runtime_stats_get(busy_id, &busy_stats);
        k_thread_runtime_stats_get(idlish_id, &idlish_stats);
        k_thread_runtime_stats_all_get(&cpu_stats);

        uint32_t busy_pct = (uint32_t)((busy_stats.execution_cycles * 100ULL) / cpu_stats.execution_cycles);
        uint32_t idlish_pct = (uint32_t)((idlish_stats.execution_cycles * 100ULL) / cpu_stats.execution_cycles);

        printk("CPU usage - BusyThread: %u%%, IdleIshThread: %u%%\n", busy_pct, idlish_pct);
    }
}

K_THREAD_DEFINE(monitor_id, STACK_SIZE, monitor_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
