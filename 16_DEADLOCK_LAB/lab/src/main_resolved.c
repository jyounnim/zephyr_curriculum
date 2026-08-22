// Source: 16_DEADLOCK_LAB.md
// Section: Resoved

#include <zephyr/kernel.h>

K_MUTEX_DEFINE(mutex_x);
K_MUTEX_DEFINE(mutex_y);

void thread_a_entry(void *p1, void *p2, void *p3) {
    while (1) {
        printk("ThreadA: taking MutexX\n");
        k_mutex_lock(&mutex_x, K_FOREVER);
        printk("ThreadA: got MutexX, now taking MutexY\n");
        k_sleep(K_MSEC(100));   // give ThreadB time to grab MutexY first
        k_mutex_lock(&mutex_y, K_FOREVER);

        printk("ThreadA: got both mutexes, working...\n");
        k_mutex_unlock(&mutex_y);
        k_mutex_unlock(&mutex_x);
        k_sleep(K_MSEC(1000));
    }
}

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

K_THREAD_DEFINE(a_id, 1024, thread_a_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(b_id, 1024, thread_b_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}

