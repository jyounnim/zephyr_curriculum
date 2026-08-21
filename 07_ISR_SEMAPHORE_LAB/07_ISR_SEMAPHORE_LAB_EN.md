# 07. Interrupts (ISR) + k_sem

## What You'll Learn

The pattern here is: signal-only in the ISR, real processing in a thread. Zephyr's approach to GPIO declares hardware via **Devicetree** rather than writing pin numbers directly in code. And crucially, **`k_sem_give()` can be called as-is inside an ISR, with no separate dedicated function** — a single unified API handles both a thread context and an ISR context.

## What You'll Need

- 1 tactile switch (button) — GPIO5, GND (same wiring as `GPIO_LAB.md`)
- A Devicetree overlay file (see below)

## Devicetree Overlay

Add the following to your project's `boards/esp32s3_devkitc.overlay` file (create it if it doesn't exist yet).

```dts
/ {
    aliases {
        sw0 = &button0;
    };

    buttons {
        compatible = "gpio-keys";
        button0: button_0 {
            gpios = <&gpio0 5 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
            label = "User button";
        };
    };
};
```

This overlay tells Zephyr "there's a button on GPIO5, pulled up, that goes LOW when pressed," and lets your code reference it easily via the alias `sw0`.

## Key Concepts

| Function | Description |
|---|---|
| `GPIO_DT_SPEC_GET(node, gpios)` | Converts GPIO information declared in the devicetree into a struct usable in code |
| `gpio_pin_interrupt_configure_dt(...)` | Configures an interrupt (edge detection, etc.) on that pin |
| `gpio_init_callback` / `gpio_add_callback` | Connects a callback function to the interrupt |
| `k_sem_give(&sem)` | **Can be called directly from an ISR** — no separate `FromISR` version needed |

## Code

```c
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

    printk("Ready. Press the button connected to GPIO5.\n");
    return 0;
}
```

## Run & Verify

- Confirm `ButtonHandlerThread: interrupt signal received...` prints every time you press the button

## Things to Notice

- Zephyr handles both a thread context and an ISR context with **a single `k_sem_give()`** — the kernel decides on its own whether the woken thread needs to run immediately. This unified API is the key takeaway of this lab
- By contrast, `k_mutex` (covered in Lab 09) can **never** be used in an ISR (locking and unlocking are both forbidden) — because a mutex has the concept of an "owner," which doesn't naturally apply to an ISR context. Remember the principle: "use a semaphore for event signals, a mutex for protecting a resource"
- The Devicetree approach may seem cumbersome at first, but it has the advantage that pin numbers aren't hardcoded into your code — **even if you switch boards, you only need to swap out the overlay file, and your application code can be reused as-is**

## Next

Lab 08 (`08_COUNTING_SEMAPHORE_LAB.md`) covers the counting semaphore, for managing a resource pool.
