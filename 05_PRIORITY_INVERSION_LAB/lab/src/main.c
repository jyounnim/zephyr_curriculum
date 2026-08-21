// Source: 05_PRIORITY_INVERSION_LAB.md

#include <zephyr/kernel.h>

#define STACK_SIZE 1024

K_SEM_DEFINE(lock_sem, 1, 1);   // binary semaphore used as a simple lock (not priority-aware)

void thread_l_entry(void *p1, void *p2, void *p3) {   // priority 8 (low) - holds the resource
    while (1) {
        k_sem_take(&lock_sem, K_FOREVER);
        printk("ThreadL: acquired the resource\n");
        for (int i = 0; i < 20; i++) {
            k_busy_wait(50000);   // simulate real work, 50ms chunks
        }
        printk("ThreadL: releasing the resource\n");
        k_sem_give(&lock_sem);
        k_sleep(K_MSEC(500));
    }
}

void thread_m_entry(void *p1, void *p2, void *p3) {    // priority 5 (medium) - does NOT need the resource
    while (1) {
        k_sleep(K_MSEC(300));
        printk("ThreadM: doing unrelated work (preempts ThreadL)\n");
        for (int i = 0; i < 5; i++) {
            k_busy_wait(100000);
        }
    }
}

void thread_h_entry(void *p1, void *p2, void *p3) {    // priority 2 (high) - urgently needs the resource
    while (1) {
        k_sleep(K_MSEC(1000));
        int64_t start = k_uptime_get();
        printk("ThreadH: requesting the resource...\n");
        k_sem_take(&lock_sem, K_FOREVER);
        int64_t waited_ms = k_uptime_get() - start;
        printk("ThreadH: acquired after waiting %lld ms\n", waited_ms);
        k_sem_give(&lock_sem);
    }
}

K_THREAD_DEFINE(l_id, STACK_SIZE, thread_l_entry, NULL, NULL, NULL, 8, 0, 0);
K_THREAD_DEFINE(m_id, STACK_SIZE, thread_m_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(h_id, STACK_SIZE, thread_h_entry, NULL, NULL, NULL, 2, 0, 0);

int main(void) {
    return 0;
}
