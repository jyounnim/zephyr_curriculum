// Source: 00_ZEPHYR_CURRICULUM_LAB.md

#include <zephyr/kernel.h>

int main(void) {
    while (1) {
        printk("Hello, ESP32-S3! (Zephyr)\n");
        k_sleep(K_SECONDS(1));
    }
    return 0;
}
