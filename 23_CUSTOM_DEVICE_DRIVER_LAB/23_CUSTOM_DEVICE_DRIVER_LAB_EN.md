# 23. Building a Custom Device Driver — AHT20 Example

## What You'll Learn

In `non_os/10_I2C_TEMP_HUMIDITY_LAB.md`, the Arduino approach to AHT20 pulled in the `Adafruit_AHTX0` library and called `aht.getEvent(...)` directly inside `main.cpp` — **a structure where the sensor communication logic is tangled together with the application code.**

Zephyr does this differently. It **completely separates Devicetree (declaring the hardware) + Kconfig (whether it's enabled) + driver code (controlling the hardware)**, and the application only calls `sensor_sample_fetch()`/`sensor_channel_get()` — a **common API that has nothing to do with the specific sensor.** In this lab, you'll build a driver for the AHT20 from scratch, to see exactly how this structure works in practice.

## Arduino Approach vs. Zephyr's Driver Model

| | Arduino (`non_os/10`) | Zephyr (this lab) |
|---|---|---|
| Declaring hardware | `Wire.begin()` in code, creating a library object | A Devicetree overlay (`aht20@38 { compatible = "..."; reg = <0x38>; }`) |
| Enabling/disabling | Decided by whether the code has an `#include` | `CONFIG_AHT20=y` in `prj.conf` |
| Communication logic | Mixed into the app code (`main.cpp`) as library calls | Completely separated into its own driver file (`aht20.c`) |
| Reading values from the app | `aht.getEvent(&humidity, &temp)` (a sensor-specific API) | `sensor_sample_fetch()` + `sensor_channel_get()` (a common API shared by every sensor) |
| Swapping the sensor | Rewrite the app code against a different library | Only change `compatible` in the overlay; the app code stays the same |

## What You'll Need

- An AHT20 module (same wiring as `non_os/10`, I2C address `0x38`)

## Overall Structure

Zephyr supports placing a driver "out-of-tree" inside your project — without touching the Zephyr source itself, you keep the driver inside your own project folder.

```
my_app/
├── CMakeLists.txt
├── Kconfig                              ← app-level Kconfig (pulls in the driver's Kconfig)
├── prj.conf
├── dts/
│   └── bindings/
│       └── sensor/
│           └── zds,aht20.yaml           ← Devicetree binding
├── boards/
│   └── sr100_rdk_sr100_m55.overlay      ← where the real hardware is placed
├── drivers/
│   └── sensor/
│       └── aht20/
│           ├── CMakeLists.txt
│           ├── Kconfig
│           └── aht20.c                  ← driver implementation
└── src/
    └── main.c                           ← app code (doesn't know what kind of sensor this is)
```

---

## Step 1. Write the Devicetree Binding

A binding is a specification declaring "what properties does this piece of hardware have." AHT20 has no special properties beyond its I2C address, so it's enough to inherit the common I2C device binding (`i2c-device.yaml`).

**`dts/bindings/sensor/zds,aht20.yaml`**

```yaml
description: AHT20 I2C temperature and humidity sensor

compatible: "zds,aht20"

include: i2c-device.yaml
```

`zds` is an arbitrary vendor prefix chosen for this lab (in a real project, you'd use your company or personal name). Inheriting `include: i2c-device.yaml` automatically brings in the `reg` property, which represents the I2C address.

## Step 2. Write the Kconfig — Turning the Driver On and Off

**`drivers/sensor/aht20/Kconfig`**

```
config AHT20
    bool "AHT20 temperature and humidity sensor"
    default y
    depends on I2C
    help
      Enables the AHT20 I2C temperature/humidity sensor driver.
```

**`Kconfig`** (top-level, app-level)

```
source "Kconfig.zephyr"
rsource "drivers/sensor/aht20/Kconfig"
```

## Step 3. Add the Driver Source Directly to the App Target

Don't turn the driver into a separate `zephyr_library()`. Calling `zephyr_library()` inside a subdirectory pulled in via the app's `add_subdirectory()` gets processed in **"application mode"** — by that point, Zephyr's final list of libraries to link has already been finalized, so the `zephyr_library()` call becomes a silent no-op. The source still compiles, but the result never gets linked into the final image, leading to an `undefined reference` error later when `main.c` looks for the device symbol that `SENSOR_DEVICE_DT_INST_DEFINE` created (confirmed as intentional warning behavior in Zephyr GitHub PR #91143).

Instead, add the driver source **directly to the `app` target** — `app` is a special target that's always guaranteed to be linked into the final image, so this problem never comes up.

**`CMakeLists.txt`** (top-level app file — replace the Step 5 CMakeLists.txt below with this)

```cmake
cmake_minimum_required(VERSION 3.20.0)

list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})   # also search this project's dts/bindings

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(aht20_driver_example)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_AHT20 app PRIVATE drivers/sensor/aht20/aht20.c)
```

`target_sources_ifdef` is a Zephyr-provided CMake extension function that adds the source to the given target (`app`) only when `CONFIG_AHT20` is `y` — the reasoning behind the `_ifdef` suffix (an unused driver stays out of the build) is the same as before.

## Step 4. Implement the Driver

**`drivers/sensor/aht20/aht20.c`**

```c
#define DT_DRV_COMPAT zds_aht20

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(aht20, CONFIG_SENSOR_LOG_LEVEL);

struct aht20_config {
    struct i2c_dt_spec i2c;
};

struct aht20_data {
    uint32_t humidity_raw;
    uint32_t temp_raw;
};

static int aht20_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    const struct aht20_config *config = dev->config;
    struct aht20_data *data = dev->data;
    uint8_t trigger_cmd[3] = {0xAC, 0x33, 0x00};   // AHT20 measurement trigger command (per datasheet)
    uint8_t buf[6];
    int ret;

    ret = i2c_write_dt(&config->i2c, trigger_cmd, sizeof(trigger_cmd));
    if (ret < 0) {
        LOG_ERR("Failed to trigger measurement: %d", ret);
        return ret;
    }

    k_sleep(K_MSEC(80));   // wait for the conversion to complete (datasheet recommends 80ms)

    ret = i2c_read_dt(&config->i2c, buf, sizeof(buf));
    if (ret < 0) {
        LOG_ERR("Failed to read sensor data: %d", ret);
        return ret;
    }

    // buf[0] is a status byte; buf[1..5] hold a 20-bit humidity value + a 20-bit temperature value
    data->humidity_raw = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | (buf[3] >> 4);
    data->temp_raw = (((uint32_t)buf[3] & 0x0F) << 16) | ((uint32_t)buf[4] << 8) | buf[5];

    return 0;
}

static int aht20_channel_get(const struct device *dev, enum sensor_channel chan,
                              struct sensor_value *val) {
    struct aht20_data *data = dev->data;

    if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
        // temp(C) = raw / 2^20 * 200 - 50   (computed in milli-degree units, then split into integer/fractional parts)
        int64_t milli_c = ((int64_t)data->temp_raw * 200000 / 1048576) - 50000;
        val->val1 = (int32_t)(milli_c / 1000);
        val->val2 = (int32_t)((milli_c % 1000) * 1000);
    } else if (chan == SENSOR_CHAN_HUMIDITY) {
        // humidity(%) = raw / 2^20 * 100
        int64_t milli_pct = (int64_t)data->humidity_raw * 100000 / 1048576;
        val->val1 = (int32_t)(milli_pct / 1000);
        val->val2 = (int32_t)((milli_pct % 1000) * 1000);
    } else {
        return -ENOTSUP;
    }
    return 0;
}

static const struct sensor_driver_api aht20_api = {
    .sample_fetch = aht20_sample_fetch,
    .channel_get = aht20_channel_get,
};

static int aht20_init(const struct device *dev) {
    const struct aht20_config *config = dev->config;

    if (!device_is_ready(config->i2c.bus)) {
        LOG_ERR("I2C bus not ready");
        return -ENODEV;
    }

    k_sleep(K_MSEC(40));   // stabilization delay after power-on (per datasheet)
    return 0;
}

#define AHT20_INIT(inst)                                                  \
    static struct aht20_data aht20_data_##inst;                          \
    static const struct aht20_config aht20_config_##inst = {             \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                                \
    };                                                                    \
    SENSOR_DEVICE_DT_INST_DEFINE(inst, aht20_init, NULL,                  \
                  &aht20_data_##inst, &aht20_config_##inst,               \
                  POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,                \
                  &aht20_api);

DT_INST_FOREACH_STATUS_OKAY(AHT20_INIT)
```

## Step 5. Devicetree Overlay — Placing the Real Hardware

**`boards/sr100_rdk_sr100_m55.overlay`** (this file name is auto-detected because it matches the board target — see the I2C bus scanner lab)

On SR110, I2C0's SoC-level devicetree default is `status = "disabled"` with no pinctrl assignment, so just adding `status = "okay"` isn't enough — pinctrl has to be specified too (confirmed in the I2C bus scanner lab, using the `i2c0_ms_scl`/`i2c0_ms_sda` pin groups). If you use I2C1 instead, it's already enabled in the board dts and needs no overlay.

```dts
&i2c0 {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&i2c0_ms_scl &i2c0_ms_sda>;
    clock-frequency = <100000>;

    aht20_sensor: aht20@38 {
        compatible = "zds,aht20";
        reg = <0x38>;
        status = "okay";
    };
};
```

> `&i2c0` is the board's I2C controller node label — confirmed for SR110 in `sr100_m55.dtsi`. On other boards/Zephyr versions the label may differ — check the actual I2C node name in the merged devicetree (`build/zephyr/zephyr.dts`) generated by `west build -t devicetree`.

## Step 6. prj.conf

```
CONFIG_I2C=y
CONFIG_I2C_DW=y
CONFIG_SENSOR=y
CONFIG_AHT20=y
CONFIG_LOG=y
```

> `CONFIG_I2C_DW=y` may be required explicitly since SR110's I2C controller is a Synopsys DesignWare part (`snps,designware-i2c`) — the same pattern seen in the I2C bus scanner lab, where the devicetree node alone wasn't sufficient.

## Step 7. App Code — Code That Doesn't Need to Know What Kind of Sensor This Is

**`src/main.c`**

```c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

int main(void) {
    const struct device *aht20 = DEVICE_DT_GET_ANY(zds_aht20);

    if (aht20 == NULL || !device_is_ready(aht20)) {
        printk("AHT20 device not ready\n");
        return 0;
    }

    while (1) {
        struct sensor_value temp, humidity;

        sensor_sample_fetch(aht20);
        sensor_channel_get(aht20, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        sensor_channel_get(aht20, SENSOR_CHAN_HUMIDITY, &humidity);

        printk("Temperature: %d.%03d C, Humidity: %d.%03d %%\n",
               temp.val1, temp.val2 / 1000,
               humidity.val1, humidity.val2 / 1000);

        k_sleep(K_SECONDS(2));
    }
    return 0;
}
```

## Build & Run

```bash
west build -p always -b sr100_rdk/sr100/m55 <lab path>
# Flash: cd srsdk_tools && python openocd_flash.py ... (see the SR110 flashing guide)
# Console: open a serial terminal at 230400bps 8N1
```

## Run & Verify

- Confirm temperature/humidity print every 2 seconds — the values themselves should match what you saw in `non_os/10` (it's the same sensor and the same protocol, just implemented differently)

## Things to Notice

- **`main.c` doesn't even need to know the name "AHT20"** (aside from the single line that fetches the device with `DEVICE_DT_GET_ANY(zds_aht20)`). `sensor_sample_fetch`/`sensor_channel_get` are the exact same functions you'd call whether it's a BME280 or an MPU6050 — **swapping the sensor never changes this two-line calling pattern.** This is a concrete example of why Zephyr is "advantageous if you have plans to expand to multiple pieces of hardware" (see `22_ZEPHYR_VS_FREERTOS_LAB.md`)
- Thanks to `DT_INST_FOREACH_STATUS_OKAY(AHT20_INIT)`, if you declare **multiple** AHT20 instances in the overlay (on different I2C buses or addresses), multiple instances are automatically created with no code changes
- The biggest lesson from this lab isn't really about the driver code itself — it's about **how the build system actually works.** Even a Zephyr-specific CMake function like `zephyr_library()` behaves differently depending on when and where it's called (kernel mode vs. application mode). If you want to turn this into a genuinely reusable, standalone component, the officially recommended next step is packaging it as a proper Zephyr module (with its own `zephyr/module.yml`) and registering it via `ZEPHYR_EXTRA_MODULES`
- Why `sensor_value` is split into two fields, `val1` (integer part) and `val2` (fractional part, in micro-units): the Zephyr kernel tends to avoid floating-point arithmetic by default (some MCUs lack an FPU, and even those that have one often restrict floating-point use in interrupt contexts) — so sensor values are also standardly represented as fixed-point, using two integers

## Troubleshooting

| Symptom | Cause / Fix |
|---|---|
| `undefined reference to '__device_dts_ord_N'` or `aht20_init`, etc. | The driver source isn't linked into the final image — if you're using a separate `zephyr_library()` + `add_subdirectory()`, this is likely the "application mode no-op" issue described in Step 3. Switch to adding the source directly to the `app` target with `target_sources_ifdef(CONFIG_AHT20 app PRIVATE ...)` (Step 3) |
| Can't find the Devicetree binding (`'zds,aht20' compatible not found`) | Confirm `list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})` comes **before** `find_package(Zephyr...)` (order matters) |
| `device_is_ready()` keeps returning false | The overlay's I2C bus label (e.g. `&i2c0`, `&i2c1`) may not match your actual board — check the correct I2C node name in the merged dts via `west build -t devicetree` |
| Values differ from `non_os/10` (Arduino) | Double-check the trigger command (`0xAC 0x33 0x00`) and the raw-value bit math — since both implementations use the same protocol, the values should match |
| `CONFIG_AHT20` isn't taking effect | Check whether the app's top-level `Kconfig` file has `rsource "drivers/sensor/aht20/Kconfig"`, and that the filename is exactly `Kconfig` (no extension) |
| Kconfig circular dependency error (`...depends on I2C ... select I2C ...`) | Caused by using both `depends on I2C` and `select I2C` on the same symbol — this is a Kconfig anti-pattern that can create a circular dependency once it interacts with I2C's own reverse-dependency chain (other drivers that `select` it). Remove `select I2C` and keep only `depends on I2C` |
| `fatal error: zephyr/heap_constants.h: No such file or directory` | A Zephyr 4.4.0 issue that shows up when using a separate `zephyr_library()`, which can get compiled before `zephyr_generated_headers` finishes — switching to the `target_sources_ifdef(app ...)` approach in Step 3 avoids this entirely, since the `app` target already has this dependency wired correctly |

## Ideas for Further Extension

- Add a power management callback (`pm_device_action_cb`) — combine it with `19_POWER_MANAGEMENT_LAB.md` to let the sensor enter low-power mode too
- Add trigger/interrupt support — extend the driver to work off a data-ready interrupt instead of polling
- Split this driver out into a separate west module (`zephyr/module.yml`), turning it into an independent, reusable component across multiple projects
