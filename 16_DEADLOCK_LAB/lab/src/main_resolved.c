// Source: 16_DEADLOCK_LAB.md
// Section: 코드 (해결 — 잠금 순서 통일)

void thread_b_entry(void *p1, void *p2, void *p3) {
    while (1) {
        printk("ThreadB: taking MutexX\n");
        k_mutex_lock(&mutex_x, K_FOREVER);   // ThreadA와 동일하게 X를 먼저
        printk("ThreadB: got MutexX, now taking MutexY\n");
        k_sleep(K_MSEC(100));
        k_mutex_lock(&mutex_y, K_FOREVER);

        printk("ThreadB: got both mutexes, working...\n");
        k_mutex_unlock(&mutex_y);
        k_mutex_unlock(&mutex_x);
        k_sleep(K_MSEC(1000));
    }
}
