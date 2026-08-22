// Source: 17_CRITICAL_SECTION_LAB.md
// Section: resoved(Protect With irq_lock)

#include <zephyr/kernel.h>

volatile int32_t shared_counter = 0;
volatile int32_t isr_increment_count = 0;
#define ITERATIONS 300000

void timer_expiry(struct k_timer *timer_id) {
    unsigned int key = irq_lock();
    shared_counter++;
    irq_unlock(key);
    isr_increment_count++;
}

K_TIMER_DEFINE(counter_timer, timer_expiry, NULL);

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

K_THREAD_DEFINE(counting_id, 1024, counting_thread_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    k_timer_start(&counter_timer, K_USEC(50), K_USEC(50));
    return 0;
}
