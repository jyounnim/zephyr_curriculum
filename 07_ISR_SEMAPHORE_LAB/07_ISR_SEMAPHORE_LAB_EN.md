# 07. Interrupts (ISR) + k_sem

## What You'll Learn

The pattern here is: signal-only in the ISR, real processing in a thread. Zephyr's approach to GPIO declares hardware via **Devicetree** rather than writing pin numbers directly in code. And crucially, **`k_sem_give()` can be called as-is inside an ISR, with no separate dedicated function** — a single unified API handles both a thread context and an ISR context.

## What You'll Need

- Nothing extra — **this lab uses the physical button (SW8) that's already on the SR110 RDK board.** No wiring or devicetree overlay required.

> **Confirmed against the real board `sr100_rdk_m55.dts` (reviewed 2026-08)**: the board-level devicetree already defines a `sw0` alias pointing at the `user_button` node.
>
> ```dts
> aliases {
>     ...
>     sw0 = &user_button;
>     ...
> };
>
> buttons: keys {
>     compatible = "gpio-keys";
>
>     user_button: user_button {
>         label = "SW8";
>         gpios = <&gpio_exp0 11 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
>         zephyr,code = <INPUT_KEY_0>;
>     };
> };
> ```
>
> So the actual physical button is silkscreened **SW8**, and it's wired to pin 11 of `gpio_exp0` (the PCA6416A I2C GPIO expander, on I2C1 — see the I2C bus scanner lab). `gpio_exp0` wires its own INT pin (`&gpioa 3`) to a real SoC GPIO, so a button sitting behind the expander still works with Zephyr's standard GPIO interrupt API (`gpio_pin_interrupt_configure_dt`, `gpio_add_callback`, etc.) — being behind I2C is invisible to application code.
>
> **Bottom line: no new GPIO to define, no external button to wire — just use the existing `sw0` alias.** The code below only references `DT_ALIAS(sw0)`, so no overlay file is needed at all.

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

    printk("Ready. Press SW8.\n");
    return 0;
}
```

`prj.conf`:

```
CONFIG_GPIO=y
CONFIG_PRINTK=y
```

> Since `user_button` sits behind `gpio_exp0` (PCA6416A, `nxp,pcal6416a` compatible), confirm that this driver's Kconfig gets enabled automatically — the board dts already has `gpio_exp0` at `status = "okay"`, so this is usually picked up automatically, but if you hit a link error, you may need to enable the relevant `CONFIG_GPIO_PCAL6416A`-style Kconfig symbol explicitly.

## Run & Verify

Console at 230400bps 8N1.

- Confirm `ButtonHandlerThread: interrupt signal received...` prints every time you press SW8

## Things to Notice

- Zephyr handles both a thread context and an ISR context with **a single `k_sem_give()`** — the kernel decides on its own whether the woken thread needs to run immediately. This unified API is the key takeaway of this lab
- By contrast, `k_mutex` (covered in Lab 09) can **never** be used in an ISR (locking and unlocking are both forbidden) — because a mutex has the concept of an "owner," which doesn't naturally apply to an ISR context. Remember the principle: "use a semaphore for event signals, a mutex for protecting a resource"
- The Devicetree approach may seem cumbersome at first, but it has the advantage that pin numbers aren't hardcoded into your code — **as long as the alias/overlay lines up, application code can be reused as-is even across very different boards.** This lab is direct proof of that: going from ESP32-S3 to SR110, the application code (`main.c`) **didn't change at all**, even though the physical wiring went from a direct SoC GPIO button to a button sitting behind an I2C GPIO expander — `DT_ALIAS(sw0)` means the code never has to know the difference

## Next

Lab 08 (`08_COUNTING_SEMAPHORE_LAB.md`) covers the counting semaphore, for managing a resource pool.
