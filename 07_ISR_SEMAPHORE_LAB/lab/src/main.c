// Source: 07_ISR_SEMAPHORE_LAB.md

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define BUTTON_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static struct gpio_callback button_cb_data;

K_SEM_DEFINE(button_sem, 0, 1);

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    k_sem_give(&button_sem);   // safe to call directly from ISR context
}

void button_handler_entry(void *p1, void *p2, void *p3) {
    while (1) {
        k_sem_take(&button_sem, K_FOREVER);
        printk("ButtonHandlerThread: interrupt signal received, handling button press\n");
    }
}

K_THREAD_DEFINE(handler_id, 1024, button_handler_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    if (!gpio_is_ready_dt(&button)) {
        printk("Error: button device not ready\n");
        return 0;
    }

    gpio_pin_configure_dt(&button, GPIO_INPUT);
    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);

    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    printk("Ready. Press the buttons.\n");
    return 0;
}
