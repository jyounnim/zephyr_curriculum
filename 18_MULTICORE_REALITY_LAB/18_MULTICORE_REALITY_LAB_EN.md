# 18. Multicore on the ESP32-S3 — the AMP Architecture

## What You'll Learn

The Zephyr port for the ESP32-S3 uses a multicore model called **AMP (Asymmetric Multi-Processing)**. This lab focuses less on hands-on coding and more on understanding exactly what this architecture is and why it was designed this way.

## Understanding the AMP Architecture

| Aspect | Description |
|---|---|
| Kernel image | **A separate** OS image per core (built independently for PROCPU and APPCPU) |
| Thread placement | Each core only knows about the threads in its own image — it's unaware the other core's threads even exist |
| Inter-core communication | **You must build a separate communication channel via an IPC mechanism (e.g., OpenAMP)** |
| Build process | Build both images at once with **west sysbuild**, then flash each to its respective core |
| Serial output | Only **PROCPU** can use `printk()`. APPCPU has to use the ESP32 ROM function (`ets_printf()`) instead |

## Why It Was Designed This Way

Zephyr was designed from the start to broadly support a wide range of architectures — from single-core MCUs all the way up to multicore chips. For chips like the ESP32 family, where it's common for "the two cores to play different roles" (e.g., one dedicated to communication, the other to the application), it's natural to load independent firmware onto each core and only exchange the information they explicitly need. In practice, this structure is used to run something like the Wi-Fi/BLE stack as a separate image on one core, while application logic runs on the other.

## If You Want to Actually Use Both Cores Together

Zephyr provides an official sample for this AMP configuration (`samples/drivers/ipm/ipm_esp32`). Build both images at once with the `--sysbuild` option, as shown below.

```bash
west build -b esp32s3_devkitc/esp32s3/procpu --sysbuild samples/drivers/ipm/ipm_esp32
west flash
west espressif monitor
```

This sample demonstrates a round-trip exchange, where PROCPU sends a message to APPCPU and APPCPU sends a reply back. The log looks roughly like this:

```
PRO_CPU is sending a request, waiting remote response...
PRO_CPU received a message from APP_CPU : APP_CPU uptime ticks 502
```

This lab series recommends trying this sample yourself as a "next step," but doesn't reproduce it in the main text — the IPC configuration (devicetree, sysbuild setup) is far more complex than anything covered so far, and the project structure itself is different.

## Things to Notice

- Every lab from 01 through 17 ran entirely on **a single PROCPU.** You created several threads, but that was "multithreading within one core," not "actually using two cores at once"
- In an AMP architecture, inter-core communication is itself an explicit design decision — you must define from the start exactly what data gets exchanged, when, and via what protocol. This is a fundamentally different approach from freely sharing the same memory space
- If you're working on a real ESP32-S3 + Zephyr project that needs both cores, you need to factor in this AMP architecture and IPC design from the very start of the project — this isn't something you can bolt on later with a one-line code change; the project structure itself needs to be designed around two independent images from day one

## Next

Lab 19 (`19_POWER_MANAGEMENT_LAB.md`) covers Zephyr's power management features.
