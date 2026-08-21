// Source: 14_K_TIMER_LAB.md
// Section: 코드

#include <zephyr/kernel.h>

void periodic_expiry(struct k_timer *timer_id) {
    printk("PeriodicTimer: tick (every 2s) [running in ISR context]\n");
}

void timeout_expiry(struct k_timer *timer_id) {
    printk("TimeoutTimer: no activity for 3s - timeout! [running in ISR context]\n");
}

K_TIMER_DEFINE(periodic_timer, periodic_expiry, NULL);
K_TIMER_DEFINE(timeout_timer, timeout_expiry, NULL);

void activity_simulator_entry(void *p1, void *p2, void *p3) {
    for (int i = 0; i < 3; i++) {
        k_sleep(K_MSEC(1500));
        printk("ActivitySimulatorThread: activity detected, resetting timeout timer\n");
        k_timer_start(&timeout_timer, K_MSEC(3000), K_NO_WAIT);   // restart = reset
    }
    printk("ActivitySimulatorThread: no more activity from now on\n");
}

K_THREAD_DEFINE(activity_id, 1024, activity_simulator_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    k_timer_start(&periodic_timer, K_MSEC(2000), K_MSEC(2000));   // period > 0 -> periodic
    k_timer_start(&timeout_timer, K_MSEC(3000), K_NO_WAIT);        // period = 0 -> one-shot
    return 0;
}
