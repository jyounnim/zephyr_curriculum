# 18. Multicore on SR110 — a Heterogeneous AMP Architecture

## What You'll Learn

The Zephyr port for SR110 also uses an **AMP (Asymmetric Multi-Processing)** multicore model, like the ESP32-S3 — but the two cores aren't identical, the way ESP32-S3's PROCPU/APPCPU are. SR110 pairs an **Arm Cortex-M55** (with the Ethos-U55 NPU, the "performance domain") with an **Arm Cortex-M4** (the "efficiency domain"), confirmed on both the official Zephyr `sr100_rdk` board page and the Synaptics product brief. This lab focuses on understanding what that heterogeneous split means and how the cores actually talk to each other, more than on reproducing code.

## Understanding the AMP Structure

| Item | Description |
|---|---|
| Kernel image | A **separate** OS image per core (build one image targeting the M55, a separate one targeting the M4) |
| Thread placement | Each core only knows about the threads in its own image — it has no visibility into the other core's threads |
| Inter-core communication | **Confirmed**: Zephyr's standard **mbox API** over an 8-byte shared-memory region — not OpenAMP/RPMsg, unlike some other Zephyr AMP boards |
| Build target | The M55 target is `sr100_rdk/sr100/m55`; the M4 side would need its own target (see TODO/VERIFY below) |
| Console output | TODO/VERIFY: whether both cores can independently use `printk()` over the shared 230400bps 8N1 console, or whether only one core owns that UART by default |

## Why It's Designed This Way

Unlike ESP32-S3's AMP model — two *symmetric* Xtensa cores where the split (network stack vs. application) is mostly a software/product decision — SR110's M55/M4 split is a **hardware-level power/performance tiering** decision. The product brief describes this directly: the M55 (with Ethos-U55 NPU) is the "performance domain" for heavier AI inference workloads, and the M4 is the "efficiency domain" meant to handle lighter, more power-conscious tasks so the M55 can stay powered down more of the time. This maps onto SR110's four power tiers (active / low-power / ULP AON / power-down) — the M4 (and the AON island) are what let the SoC stay in a low-power state while still reacting to events, waking the M55 only when heavier compute is actually needed.

## If You Want to Try the Two Cores Together

TODO/VERIFY: this curriculum has not yet built and run an actual M55↔M4 mbox round-trip example on real SR110 hardware. What's confirmed so far (from prior hands-on SR110 work) is the mechanism — Zephyr's standard mbox driver API plus an 8-byte shared-memory handshake region — not a specific working sample path or exact devicetree node names for the mbox instances on this board. Before attempting this:

1. Check the vendor SDK (`syna_zephyr_sdk`) and the Astra MCU SDK example catalog (`system_manager` sample, which is documented with "RPMSG Support" in the Astra MCU SDK Application User Guide) for an existing IPC/mbox reference application — this is the most likely starting point rather than building one from scratch
2. Confirm the exact devicetree mbox node labels and West build target needed for the M4-side image (unlike ESP32-S3's `--sysbuild` two-image flow, SR110's exact sysbuild/build configuration for a dual-core application hasn't been verified in this curriculum)
3. Confirm which core owns the debug console UART by default, since both cores producing `printk()` output on a shared physical UART (if that's even how it's wired) would need to be disambiguated

## Things to Notice

- Every lab so far (01–17) ran on a **single core** (the M55) — you created multiple threads, but that was "multithreading within one core," not "actually using both cores at once"
- In an AMP structure, inter-core communication is itself an explicit design decision — what data, when, and over what protocol has to be defined up front. This is a fundamentally different approach from freely sharing the same memory space
- If a real project needs to use both SR110 cores, this AMP structure and the mbox-based IPC design need to be considered from the very start of the project — it's not something you bolt on with one line of code later; the project has to be structured from the beginning as two independent images that explicitly hand data back and forth

## Next

Lab 19 (`19_POWER_MANAGEMENT_LAB.md`) covers Zephyr's power management features.
