// Source: 00_ZEPHYR_CURRICULUM_LAB.md

#include <zephyr/kernel.h>

int main(void) {
    while (1) {
        printk("Hello, SR110! (Zephyr %s) \n", CONFIG_BOARD_TARGET);
        k_sleep(K_SECONDS(1));
    }
    return 0;
}
