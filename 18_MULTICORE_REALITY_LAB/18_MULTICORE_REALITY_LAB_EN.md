# 18. Multicore on the ESP32-S3 — the AMP Architecture and IPC

## What You'll Learn

The Zephyr port for the ESP32-S3 uses a multicore model called **AMP (Asymmetric Multi-Processing)**. This lab goes beyond just understanding what this architecture is — you'll **actually build and run the official IPM (Inter-Processor Mailbox) sample, and dig into the IPC API it uses.**

## Understanding the AMP Architecture

| Aspect | Description |
|---|---|
| Kernel image | **A separate** OS image per core (built independently for PROCPU and APPCPU) |
| Thread placement | Each core only knows about the threads in its own image — it's unaware the other core's threads even exist |
| Inter-core communication | **You must build a separate communication channel via an IPC mechanism** (IPM, covered in this lab, is one such mechanism) |
| Build process | Build both images at once with **west sysbuild**, then flash each to its respective core |
| Bootloader | AMP builds **require MCUboot** — the default Simple Boot mechanism cannot run AMP at all |
| Serial output | Only **PROCPU** can use `printk()`. APPCPU has to use the ESP32 ROM function (`ets_printf()`) instead |

## Why It Was Designed This Way

Zephyr was designed from the start to broadly support a wide range of architectures — from single-core MCUs all the way up to multicore chips. For chips like the ESP32 family, where it's common for "the two cores to play different roles" (e.g., one dedicated to communication, the other to the application), it's natural to load independent firmware onto each core and only exchange the information they explicitly need. In practice, this structure is used to run something like the Wi-Fi/BLE stack as a separate image on one core, while application logic runs on the other.

## Inter-Core Communication on the ESP32-S3 — IPM (Inter-Processor Mailbox)

The ESP32 family has **4 hardware-backed inter-core messaging channels** between PRO_CPU and APP_CPU. **Channels 0 and 1 are reserved for the Wi-Fi/BT stack**, leaving **channels 2 and 3 free for application use**. Zephyr wraps this hardware in an IPM (Inter-Processor Mailbox) driver called `ipm_esp32`, giving you access through the standard IPM API.

> ⚠️ Each channel can carry **at most 64 bytes** of data per message — anything larger needs to be split into multiple chunks at the application level.

### Core IPM API

| Function/Type | Description |
|---|---|
| `ipm_send(dev, wait, id, data, size)` | Sends a message to the other core. `id` distinguishes the kind of message; `size` is at most 64 bytes |
| `ipm_register_callback(dev, callback, user_data)` | Registers a callback to be called when a message arrives |
| `ipm_callback_t` | `void (*)(const struct device *ipmdev, void *user_data, uint32_t id, volatile void *data)` — **this callback runs in interrupt context** |
| `ipm_set_enabled(dev, enable)` | Enables/disables reception |

Notice that the callback runs in interrupt context — **the principle you learned in Lab 07 (button interrupts) — "signal only inside a callback/ISR, do the real processing elsewhere" — applies here just as directly.**

## Hands-On — Build & Run the Official Sample

Zephyr provides an official sample demonstrating this IPM communication (`samples/drivers/ipm/ipm_esp32`). Build both the PROCPU and APPCPU images at once with the `--sysbuild` option.

```bash
west build -b esp32s3_devkitc/esp32s3/procpu --sysbuild samples/drivers/ipm/ipm_esp32
west flash
west espressif monitor
```

### Expected Output

```
PRO_CPU is sending a request, waiting remote response...
PRO_CPU received a message from APP_CPU : APP_CPU uptime ticks 502
PRO_CPU is sending a request, waiting remote response...
PRO_CPU received a message from APP_CPU : APP_CPU uptime ticks 502
```

This is a round-trip exchange: PRO_CPU (PROCPU) sends a request over the channel, APP_CPU (APPCPU) replies with its own uptime value, and PRO_CPU receives and prints it.

## Hands-On — Reading the Sample Source

The sample source lives in your workspace under `zephyr/samples/drivers/ipm/ipm_esp32/`, structured like this:

```
ipm_esp32/
├── src/            ← source for PROCPU (the main application)
├── remote/         ← a separate project for APPCPU (the remote core), with its own CMakeLists.txt and prj.conf
└── sysbuild.cmake  ← tells sysbuild to build both images together
```

Notice that these are **two completely independent applications** sitting side by side in one folder — `remote/` isn't pulled in via `add_subdirectory` from `src/`; it's a separate project with its own build system. This is the "AMP means an independent image per core" principle showing up directly in the project structure.

### Exercises

1. Change the `id` value of the request message sent from `src/main.c` (the PROCPU side), and add branching logic on the `remote/` (APPCPU) side so it replies differently depending on that `id`
2. Right now it's "one request → one response." Extend it so PROCPU sends a counter value and APPCPU doubles it and sends it back — actually putting the exchanged data to use (staying within the 64-byte limit)
3. Inside the callback registered with `ipm_register_callback`, call `k_sem_give()`, and move the real processing (parsing the message, logging, etc.) into a separate thread that wakes up via `k_sem_take()` — practice applying the pattern from Lab 07 directly to an IPM callback

## Things to Notice

- Labs 01 through 17 all ran entirely on **a single PROCPU.** You created several threads, but that was "multithreading within one core," not "actually using two cores at once"
- In an AMP architecture, inter-core communication is itself an explicit design decision — you must define from the start exactly what data gets exchanged, when, and via what protocol. This is a fundamentally different approach from freely sharing the same memory space
- IPM is a relatively simple mechanism with a "64 bytes max per message" constraint — for larger data or a more elaborate protocol, Zephyr also supports a heavier IPC stack based on **OpenAMP + shared memory + rpmsg** (the `ipc_service` API). Think of it as: IPM for "lightweight, fast signals or small data exchanges," OpenAMP for "large, structured message exchange"
- If you're working on a real ESP32-S3 + Zephyr project that needs both cores, you need to factor in this AMP architecture and IPC design from the very start of the project — this isn't something you can bolt on later with a one-line code change; the project structure itself needs to be designed around two independent images from day one

## Troubleshooting

| Symptom | Cause / Fix |
|---|---|
| Build succeeds, but no logs show up from APPCPU | This is expected — APPCPU can't use Zephyr's `printk()`. Use `ets_printf()` (an ESP32 ROM function), or check exactly what output method the `remote/` project's source actually uses |
| Building without `--sysbuild` fails, or only builds one image | AMP requires building both images together — make sure you didn't drop the `--sysbuild` option |
| Nothing happens after flashing | AMP builds require MCUboot — if a leftover Simple Boot (the default) configuration is still active, the board may fail to boot at all. Check the sysbuild log to confirm an MCUboot image was built alongside the application |
| `ipm_send` fails (e.g., `-EBUSY`) | You may be trying to send while still waiting on a response to a previous message — make sure you're following the request → response → next request sequence, handling the response in the callback before sending again |

## Next

Lab 19 (`19_POWER_MANAGEMENT_LAB.md`) covers Zephyr's power management features.
