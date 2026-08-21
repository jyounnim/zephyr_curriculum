// Source: 15_STACK_MONITORING_LAB.md
// Section: 코드

#include <zephyr/kernel.h>
#include <string.h>

#define STACK_SIZE 2048

void light_entry(void *p1, void *p2, void *p3) {
    while (1) {
        int small_var = 0;
        small_var++;
        k_sleep(K_MSEC(1000));
    }
}

void recursive_work(int depth) {
    char buffer[256];   // consumes stack on every call
    memset(buffer, 0, sizeof(buffer));
    if (depth > 0) {
        recursive_work(depth - 1);
    }
}

void heavy_entry(void *p1, void *p2, void *p3) {
    while (1) {
        recursive_work(4);
        k_sleep(K_MSEC(1000));
    }
}

K_THREAD_DEFINE(light_id, STACK_SIZE, light_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(heavy_id, STACK_SIZE, heavy_entry, NULL, NULL, NULL, 5, 0, 0);

void monitor_entry(void *p1, void *p2, void *p3) {
    size_t light_unused, heavy_unused;
    while (1) {
        k_sleep(K_MSEC(2000));
        k_thread_stack_space_get(light_id, &light_unused);
        k_thread_stack_space_get(heavy_id, &heavy_unused);
        printk("Stack headroom (bytes) - LightThread: %u, HeavyThread: %u\n",
               (unsigned int)light_unused, (unsigned int)heavy_unused);
    }
}

K_THREAD_DEFINE(monitor_id, 1024, monitor_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
