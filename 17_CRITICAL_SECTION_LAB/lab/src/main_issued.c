// Source: 17_CRITICAL_SECTION_LAB.md
// Section: issued case(No Protection)

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
