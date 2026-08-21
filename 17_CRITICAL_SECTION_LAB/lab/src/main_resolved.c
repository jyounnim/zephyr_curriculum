// Source: 17_CRITICAL_SECTION_LAB.md
// Section: resoved(Protect With irq_lock)

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
