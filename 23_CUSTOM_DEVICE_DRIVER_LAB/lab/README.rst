.. zephyr:code-sample:: aht20_driver_example
   :name: Custom AHT20 sensor driver (out-of-tree)
   :relevant-api: sensor_interface i2c_interface

   Read temperature and humidity from an AHT20 sensor through a hand-written out-of-tree Zephyr sensor driver.

Overview
********

This sample demonstrates writing a minimal **out-of-tree** Zephyr
sensor driver from scratch for the AHT20 I2C temperature/humidity
sensor, and using it through Zephyr's hardware-agnostic sensor API
(``sensor_sample_fetch()`` / ``sensor_channel_get()``) instead of a
sensor-specific library call. See
``23_CUSTOM_DEVICE_DRIVER_LAB_KR.md`` / ``_EN.md`` for the full
step-by-step build-up (devicetree binding, Kconfig, driver
implementation, and why the driver source is added directly to the
``app`` CMake target instead of its own ``zephyr_library()``).

Hardware setup
**************

* AHT20 module on I2C0: SDA = GPIO1, SCL = GPIO2, VCC = 3V3, GND = GND
  (same I2C0 pins as the DevKitC's board-default pinctrl - see
  ``24_SSD1306_DISPLAY_LAB`` for the same wiring facts).
* I2C address: ``0x38`` (fixed for the AHT20, not configurable).

Devicetree notes
*****************

* ``&i2c0`` ships disabled at the SoC level; this lab's overlay
  (``boards/esp32s3_devkitc_esp32s3_procpu.overlay``) enables it and
  declares the ``aht20@38`` child node with ``compatible = "zds,aht20"``
  (the vendor prefix ``zds`` is arbitrary, defined in
  ``dts/bindings/sensor/zds,aht20.yaml``).
* This overlay replaces an earlier ``boards/sr100_rdk_sr100_m55.overlay``
  that was mistakenly left in this lab from a different (SR100 RDK)
  board and targeted ``&i2c1`` - it never matched what this lab's own
  docs describe or applied to the ESP32-S3.

Building and Running
*********************

.. code-block:: console

   west build -b esp32s3_devkitc/esp32s3/procpu samples/aht20_driver_example
   west flash
   west espressif monitor

Sample Output
*************

Open the console at 115200bps 8N1.

.. code-block:: console

   Temperature: 24.500 C, Humidity: 45.200 %

New Temperature/Humidity lines should print every 2 seconds.
