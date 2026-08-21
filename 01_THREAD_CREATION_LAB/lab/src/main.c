// Source: 01_THREAD_CREATION_LAB.md

#include <zephyr/kernel.h>

#define STACK_SIZE 1024

void thread_a_entry(void *p1, void *p2, void *p3) {
    while (1) {
        printk("ThreadA running\n");
        k_sleep(K_MSEC(1000));
    }
}

void thread_b_entry(void *p1, void *p2, void *p3) {
    while (1) {
        printk("ThreadB running\n");
        k_sleep(K_MSEC(700));
    }
}

// Static, compile-time thread definitions
K_THREAD_DEFINE(thread_a_id, STACK_SIZE, thread_a_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(thread_b_id, STACK_SIZE, thread_b_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    printk("main: started, threads are already running\n");
    return 0;
}
