# Zephyr RTOS Curriculum Overview

A 23-lab series for learning Zephyr RTOS's API and design philosophy, in order. Each lab is a standalone `.md` file, designed so the concepts build on each other if you go through them in numeric order. It covers everything from thread creation, synchronization/communication tools, and timing control, to multicore and power management on **Synaptics SR110 (Astra Machina Micro)**, and finally writing a custom device driver.

> 📄 **If you hit an error during setup**: Steps 1–2 below describe the flow "when everything goes smoothly." In practice you may run into a number of issues along the way (missing host tools, Python version compatibility, permission problems, etc.) — these individual errors are covered separately in **`ZEPHYR_SETUP_TROUBLESHOOTING.md`**. Keep that document open alongside this one.

## Prerequisites

- A PC (this curriculum's setup steps were confirmed on Windows; macOS/Linux paths should be broadly similar but haven't been re-verified for SR110 specifically)
- Python 3 and Git installed
- An SR110 RDK board (Astra Machina Micro), plus a USB-C cable and the on-board CMSIS-DAP debug interface for flashing/console
- At least 5GB of free disk space (for the Zephyr SDK + module source downloads)
- An internet connection (the initial install downloads a lot of data — check your network first)

> 🚨 **If you're on Windows and your account name (your user folder name, `C:\Users\???`) contains non-ASCII characters (e.g., Korean), read this first.** Python and the Zephyr SDK toolchain are installed under your user folder by default (`C:\Users\<account name>\...`), and when that path contains non-ASCII characters, `cmake.exe` has actually been observed to crash (`STATUS_STACK_BUFFER_OVERRUN`, exit code `3221226505`). **You don't need to change your account name itself — you just need to point Python's and the SDK's install locations at an ASCII-only path.** Method B in Step 2 below builds this in from the start.

---

## Step 1. Install the Zephyr IDE Extension in VS Code

1. In VS Code Extensions (`Ctrl+Shift+X`), search for `Zephyr IDE`
2. Install the **Zephyr IDE** extension by **mylonics**
3. Restart VS Code after installing

---

## Step 2. Setting Up the west Environment & Zephyr SDK

### Important — SR110 uses Synaptics' `syna_zephyr_sdk`, not the mainline Zephyr repo

Unlike a generic Zephyr board, SR110 support lives in Synaptics' own fork/vendor repo — `syna_zephyr_sdk` — which vendors both the Zephyr kernel tree (`zephyr/`) and SR100-family board/SoC definitions (`zephyr_srsdk/`) together, plus flashing tools (`srsdk_tools/`).

**This curriculum itself lives in a separate git repo, cloned independently alongside `syna_zephyr_sdk`** — not nested inside it:

```powershell
git clone https://github.com/jyounnim/zephyr_curriculum
```

A confirmed real workspace layout, with both repos cloned side by side under the same parent folder, looks like this:

```
<workspace root>/           e.g. C:\02.work\syna_zephry\syna_zephry_sdk
├── zephyr/                 vendored Zephyr kernel tree (from syna_zephyr_sdk)
├── zephyr_srsdk/            SR100-family board/SoC/pinctrl definitions (from syna_zephyr_sdk)
├── zephyr_curriculum/       this curriculum, cloned separately (see Step 3)
├── srsdk_tools/              flashing scripts (openocd_flash.py, etc.)
└── build/                   build output (created by `west build`)
```

TODO/VERIFY: the exact `west init -m <manifest-repo-url>` command that produces the `syna_zephyr_sdk` layout from scratch hasn't been re-confirmed in this curriculum pass — that repo is typically obtained as a release/checkout from Synaptics rather than a public `west init` against a generic manifest. Check the Astra MCU SDK's official install guide (`Astra_MCU_SDK_Setup_and_Install_CLI` / `_VsCode`, at `synaptics-astra-mcu.github.io`) for the current official instructions, since this can change between SDK releases.

### Method A — Using the Zephyr IDE Extension (GUI-based)

> ⚠️ In practice, you'll hit several points that require manual intervention, as shown below. Rather than expecting "the extension does everything in a few clicks," it's best to work through each step below and confirm it as you go. Most sticking points are covered in `ZEPHYR_SETUP_TROUBLESHOOTING.md`.

1. **You must open an empty folder first (or the `syna_zephyr_sdk` workspace root once you have it).** Every command in the Zephyr IDE extension operates relative to "the folder currently open in VS Code" — running a command without a folder open produces the error `No workspace folder open. Please open a folder first.`
   - We recommend a path with **no non-ASCII characters or spaces** (e.g., `C:\02.work\syna_zephry\syna_zephry_sdk`) — paths containing non-ASCII characters or spaces have caused problems later in the west/CMake build process
2. Click the **Zephyr IDE icon** in the left Activity Bar (the extension's official name is "**IDE for Zephyr**"; if the icon doesn't appear right after installing, restart VS Code)
3. A **Host Tools check screen** appears — a list of required tools (Python3, Git, CMake, Ninja, DTC/Devicetree Compiler, gperf, wget, etc.), each marked Installed or Not Available
   - **Everything marked Not Available must be installed before continuing** — skipping ahead without installing them can cause the process to silently stall at a later step with no error message
   - On Windows, install everything at once with `winget` (run PowerShell **as Administrator** first):
     ```powershell
     winget install Kitware.CMake Ninja-build.Ninja oss-winget.dtc oss-winget.gperf wget 7zip.7zip
     ```
   - After installing, you must **fully quit and reopen VS Code** for the PATH change to take effect (simply reloading the window isn't enough)
4. Confirm the toolchain that gets installed matches what SR110 actually needs. A confirmed real setup used `zephyr-sdk-1.0.1`, installed under `<user folder>\.zephyr_ide\toolchains\zephyr-sdk-1.0.1` — if `ZEPHYR_TOOLCHAIN_VARIANT` isn't set, `west build` locates this automatically and reports `Found toolchain: zephyr 1.0.1` in its output. If a mismatched or generic mainline `zephyr-sdk-x.y.z` gets picked up instead, the build's board/SoC-specific pieces (SR100 pinctrl, DesignWare I2C/GPIO drivers, etc.) may not be available.
5. **Connecting the Python Interpreter**
   - VS Code needs to know which Python to use for west/pip-related scripts
   - Command Palette → **`Python: Select Interpreter`** → select the virtual environment (`.venv` or similarly named, usually inside the workspace folder). A confirmed real setup used a venv at `venv313` (Python 3.13) activated via `.venv\Scripts\Activate.ps1`-style scripts
6. Once everything above completes successfully, move on to **Step 3 (Creating a New Project)**

### Method B — Manual Installation (Terminal CLI; More Reliable When Things Go Wrong · Recommended)

**Core principle**: point the install locations for Python and the Zephyr SDK toolchain **directly at ASCII-only paths** (this works regardless of whether your account name itself contains non-ASCII characters).

#### B-1. Install Python to an ASCII-Only Path (Required if Your Windows Account Name Contains Non-ASCII Characters)

Installing via `winget` puts it in the default location (under your user folder), which can reintroduce the non-ASCII path problem. Instead, download the installer for **Python 3.12 or 3.13** directly from [python.org](https://www.python.org/downloads/) (a confirmed real setup used Python 3.13.7 successfully, via `venv313`; earlier guidance in this curriculum for ESP32-S3 warned that 3.13+ could fail installing some packages without prebuilt wheels — worth keeping in mind if you hit a similar issue, though it didn't come up in confirmed SR110 use).

1. Run the installer → select **"Customize installation"** (do not click Install Now — it won't let you set a custom path)
2. Optional Features → leave the defaults, click Next
3. Advanced Options → enter an ASCII-only path under **"Customize install location"**
4. Install

#### B-2. Create a Workspace Folder + Dedicated Virtual Environment (venv)

```powershell
mkdir C:\02.work\syna_zephry
cd C:\02.work\syna_zephry
python -m venv venv313
venv313\Scripts\Activate.ps1        # macOS/Linux: source venv313/bin/activate
```

(You'll know it's active when `(venv313)` appears in front of your prompt — run every subsequent command in this state)

#### B-3. Install west

```powershell
python -m pip install west
```

#### B-4. Obtain `syna_zephyr_sdk`

TODO/VERIFY: the exact `west init`/clone command for the current SDK release. Consult the Astra MCU SDK's official CLI install guide for this step (`synaptics-astra-mcu.github.io/doc/v/latest/srsdk/docs/Astra_MCU_SDK_Setup_and_Install_CLI.html`), since the confirmed real workspace in this curriculum was already set up by the time hands-on work started, and the exact acquisition command wasn't re-verified here.

#### B-5. Install Zephyr's Script Dependencies

```powershell
cd zephyr\scripts
python -m pip install -r requirements.txt
cd ..\..
```

#### B-6. Install the Zephyr SDK (to an ASCII-Only Path)

```powershell
python -m west sdk install --install-base D:\work\zephyr_toolchains
```

> ⚠️ If you've ever run `west sdk install` even once before without `--install-base`, west may find an SDK already installed at the default location and reuse it, making `--install-base` appear to be ignored. If that happens, delete the existing install and also remove its CMake package registry entry (`Remove-Item -Path "HKCU:\Software\Kitware\CMake\Packages\Zephyr-sdk" -Recurse -Force`), then try again.

#### Verify the Installation

```powershell
python -m west --version
python -m west boards | findstr sr100    # macOS/Linux: grep sr100
```

If `sr100_rdk` shows up as a board target, everything is working.

> 💡 `west zephyr-export` is intentionally absent from this sequence — for the normal case of building with `west build` from inside a west workspace, west automatically passes `ZEPHYR_BASE` to CMake, making this step **unnecessary** (confirmed in an official Zephyr GitHub discussion). It's only needed in the special case of building a "freestanding app" that lives outside the workspace, directly with CMake alone.

### Connecting VS Code to a west Workspace (If You Installed via CLI Using Method B)

1. In VS Code, open the workspace folder
2. Command Palette → run **`Zephyr IDE: Setup Workspace from Current Directory`**
3. So the GUI and CLI point at the same Python/SDK, set these two values in Settings (`Ctrl+,`) to match the paths you used on the CLI:
   ```json
   {
     "zephyr-ide.venvFolder": "C:\\02.work\\syna_zephry\\venv313",
     "zephyr-ide.toolchainDirectory": "D:\\work\\zephyr_toolchains"
   }
   ```

---

## Step 3. Getting the Curriculum Repo / Creating a New Project

Clone this curriculum's own repo directly into the workspace root, alongside `syna_zephyr_sdk`'s folders:

```powershell
cd <workspace root>       # the parent of zephyr/, zephyr_srsdk/, srsdk_tools/
git clone https://github.com/jyounnim/zephyr_curriculum
```

Every lab lives under `zephyr_curriculum/<LAB_NAME>/lab/` — this convention (confirmed in real use) keeps each lab's `src/`, `CMakeLists.txt`, `prj.conf`, and any `boards/*.overlay` self-contained, and matches how the west build command below expects to be invoked (as a path argument from the workspace root, rather than `cd`-ing into the lab directory first).

```
zephyr_curriculum/
└── <LAB_NAME>/
    ├── <LAB_NAME>_KR.md
    ├── <LAB_NAME>_EN.md
    └── lab/
        ├── CMakeLists.txt
        ├── prj.conf              ← kernel config (Kconfig) — CONFIG_EVENTS, CONFIG_PM, etc. go here. If this file is missing, you get "No prj.conf file(s) was found"
        ├── boards/
        │   └── sr100_rdk_sr100_m55.overlay   ← board-specific hardware config (used in Labs 07, 23, and others). Naming this file exactly this way — the board target with `/` replaced by `_` — makes Zephyr auto-detect and apply it, with no extra CMake flags needed
        └── src/
            └── main.c
```

If you want to create a new lab of your own rather than using one that's already here, minimal `CMakeLists.txt` example:
```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(my_app)
target_sources(app PRIVATE src/main.c)
```

---

## Step 4. Writing & Running Hello World

### `src/main.c`

```c
#include <zephyr/kernel.h>

int main(void) {
    while (1) {
        printk("Hello, SR110! (Zephyr)\n");
        k_sleep(K_SECONDS(1));
    }
    return 0;
}
```

### Build

Run from the workspace root (the parent of `zephyr_curriculum/`):

```powershell
west build -p always -b sr100_rdk/sr100/m55 .\zephyr_curriculum\<LAB_NAME>\lab\
```

> ⚠️ **You must write out the board target in full: `sr100_rdk/sr100/m55`.** Confirmed real behavior: if a lab needs a devicetree overlay (e.g. to enable I2C0, which ships disabled by default), name the overlay file exactly `boards/sr100_rdk_sr100_m55.overlay` and it's picked up automatically. **Do not** additionally pass `-DEXTRA_DTC_OVERLAY_FILE=...` pointing at that same file — the duplicate reference has been observed to get mis-split by CMake's argument parsing and break devicetree preprocessing entirely.

### Flash

SR110 flashing goes through `srsdk_tools`, not `west flash` directly:

```powershell
cd srsdk_tools
python openocd_flash.py `
  --openocd <path to openocd.exe> `
  --flash-offset 0x0 `
  --file-offset 0x0 `
  --cfg_path Input_Config\sr100_m55.cfg `
  --image <path to the built/converted image>.bin
```

(Zephyr's own upstream board docs for `sr100_rdk` show a simpler invocation, `python openocd_flash.py build/zephyr/zephyr.bin 0x0 0x0 1` — the exact flags and whether the raw `zephyr.bin` or a converted/signed image is expected can differ by SDK release. TODO/VERIFY the exact image-conversion step between `build\zephyr\zephyr.bin` and whatever `openocd_flash.py` in your SDK version actually expects.)

### Console

Open a serial terminal at **230400bps 8N1** (confirmed across multiple labs in hands-on use) and confirm `Hello, SR110! (Zephyr)` prints once per second.

### Note — If You Get a `build` Folder Error

If a previous failed build attempt left the `build` folder in an incomplete state, you might see an error like `ninja: error: loading 'build.ninja': The system cannot find the file specified.` In that case, just reconfigure from scratch with `-p always` (already included in the build command above), or delete the `build` folder yourself and retry.

## Setup Checklist

- [ ] (If your Windows account name contains non-ASCII characters) Install Python to an ASCII-only path
- [ ] Install the Zephyr IDE extension and restart VS Code
- [ ] Confirm every item shows Installed on the Host Tools check screen (if anything shows Not Available, install it with winget and restart VS Code)
- [ ] Confirm the toolchain is the SR110-appropriate one (`zephyr-sdk-1.0.1` in a confirmed real setup), not a mismatched generic version
- [ ] Confirm `sr100_rdk` shows up via `west boards | findstr sr100` (Windows) or `grep sr100` (macOS/Linux)
- [ ] Confirm the Python interpreter is set to the workspace's venv (the GUI and CLI should use the same one)
- [ ] Set up a project under `zephyr_curriculum/<LAB_NAME>/lab/` (Step 3)
- [ ] `west build -p always -b sr100_rdk/sr100/m55 .\zephyr_curriculum\<LAB_NAME>\lab\` → flash via `srsdk_tools\openocd_flash.py` → confirm via serial terminal at 230400bps 8N1 (Step 4)

> `west sdk install` and the initial source download can take a while. Check your network beforehand.

## Troubleshooting (Build/Run Stage)

> For issues that occur during environment setup (Steps 1–2), see **`ZEPHYR_SETUP_TROUBLESHOOTING.md`**. The table below covers issues confirmed at the build/flash/run stage on real SR110 hardware, after the workspace is already set up correctly.

| Symptom | Cause / Fix |
|---|---|
| `sr100_rdk` doesn't show up in `west boards` | The workspace wasn't set up against `syna_zephyr_sdk` (SR110 support isn't in the mainline `zephyrproject-rtos/zephyr` manifest by default) |
| `Board qualifiers 'sr100' for board 'sr100_rdk' not found` | The core wasn't specified in the board name — write out the full `sr100_rdk/sr100/m55` |
| `No prj.conf file(s) was found` | You built the project by hand and left out `prj.conf` — it must exist even if empty |
| `undefined reference to __device_dts_ord_N` at link time | A devicetree node's `status` isn't `"okay"`, or its `compatible` string doesn't match what a Kconfig `depends on DT_HAS_..._ENABLED` is looking for — the node exists but no device instance/driver got compiled in. Confirmed real cause for I2C0 (ships `disabled` by default) and for a display driver where the compatible string had a stray `fb` suffix that didn't match the Kconfig's expected string |
| CMake reports `Ignoring extra path from command line: ".overlay"` and devicetree preprocessing fails | An `-DEXTRA_DTC_OVERLAY_FILE=...` was passed pointing at a file that's already auto-detected by its board-target-matching name — remove the redundant flag |
| `undefined reference to k_malloc` at link time | A subsystem (e.g. CFB, for driving character displays) calls `k_malloc()` internally, but no heap is configured — add `CONFIG_HEAP_MEM_POOL_SIZE=<bytes>` to `prj.conf` |
| An I2C scan or sensor probe finds nothing, or the wrong number of devices | A zero-length or 1-byte dummy `i2c_write()` probe was confirmed unreliable on SR110's DesignWare I2C driver port for detecting real devices — a 1-byte `i2c_read()` probe was confirmed to work instead |
| `console_getline()` / `console_getline_init()` link as undefined references even with `CONFIG_CONSOLE_GETLINE=y` | Confirmed on this SDK's Zephyr 4.4.1 build — use `uart_poll_in()`/`uart_poll_out()` directly on the console UART device instead, which only needs the generic serial driver API |
| `ninja: error: loading 'build.ninja': ... cannot find the file` | A previous failed build left the `build` folder incomplete — do a pristine rebuild with `west build -p always -b <board> <path>` |
| `cmake.exe` crashes with exit code `3221226505` (`0xC0000409`) | **A non-ASCII path problem** — your Python/SDK install path contains non-ASCII characters (e.g., from your account name). See the non-ASCII path issue in `ZEPHYR_SETUP_TROUBLESHOOTING.md` |
| No serial output visible | Confirm the terminal is set to 230400bps 8N1 (not 115200, which was the ESP32-S3 default in earlier versions of this curriculum) |

---

## The Core of Zephyr's Scheduling — Priority Numbers and the Cooperative/Preemptible Split

**In Zephyr, the smaller the number, the higher the priority.** And Zephyr clearly splits threads into two categories: **cooperative (negative priority)** and **preemptible (priority 0 or above)**. This concept gets heavy focus early in the curriculum (Labs 02 and 04). This is core Zephyr kernel behavior and doesn't depend on which board you're running on.

## Common Notes (Applies From Lab 01 Onward)

- Most labs can be verified with `printk()` alone; labs that need hardware wiring (Lab 07, Lab 23) say so explicitly
- The output strings in the code are written in English
- The build command for every subsequent lab is the same as in Step 4 above: `west build -p always -b sr100_rdk/sr100/m55 .\zephyr_curriculum\<LAB_NAME>\lab\`, then flash via `srsdk_tools\openocd_flash.py`, then open a serial terminal at 230400bps 8N1

## Table of Contents

| # | File | Topic |
|---|---|---|
| 01 | `01_THREAD_CREATION_LAB.md` | Thread creation basics (K_THREAD_DEFINE / k_thread_create) |
| 02 | `02_THREAD_PRIORITY_LAB.md` | The priority system and cooperative/preemptible threads |
| 03 | `03_THREAD_LIFECYCLE_LAB.md` | Dynamic thread creation/termination |
| 04 | `04_COOPERATIVE_YIELD_LAB.md` | Cooperative threads and k_yield — why you must always yield |
| 05 | `05_PRIORITY_INVERSION_LAB.md` | Reproducing priority inversion |
| 06 | `06_IDLE_THREAD_LAB.md` | The Idle Thread and CPU idle time |
| 07 | `07_ISR_SEMAPHORE_LAB.md` | Interrupts (ISR) + k_sem |
| 08 | `08_COUNTING_SEMAPHORE_LAB.md` | Counting Semaphore |
| 09 | `09_MUTEX_LAB.md` | k_mutex vs k_sem, priority inheritance |
| 10 | `10_MSGQ_BASICS_LAB.md` | Message Queue basics (k_msgq) |
| 11 | `11_K_POLL_LAB.md` | k_poll — waiting on multiple kernel objects at once |
| 12 | `12_POLL_SIGNAL_LAB.md` | Poll Signal — a lightweight event |
| 13 | `13_K_EVENT_LAB.md` | k_event — waiting on multiple conditions |
| 14 | `14_K_TIMER_LAB.md` | k_timer (one-shot / periodic) |
| 15 | `15_STACK_MONITORING_LAB.md` | Stack usage monitoring |
| 16 | `16_DEADLOCK_LAB.md` | Reproducing and avoiding deadlock |
| 17 | `17_CRITICAL_SECTION_LAB.md` | irq_lock / k_sched_lock / k_spinlock |
| 18 | `18_MULTICORE_REALITY_LAB.md` | Multicore on SR110 — the heterogeneous M55/M4 AMP architecture |
| 19 | `19_POWER_MANAGEMENT_LAB.md` | Zephyr Power Management (prj.conf) |
| 20 | `20_RUNTIME_STATS_LAB.md` | Thread Runtime Stats — the CPU usage API |
| 21 | `21_PRODUCER_CONSUMER_LAB.md` | Producer-consumer synthesis pattern |
| 22 | `22_ZEPHYR_SUMMARY_LAB.md` | Full curriculum wrap-up |
| 23 | `23_CUSTOM_DEVICE_DRIVER_LAB.md` | Building a custom device driver (AHT20 example) |

## Learning Flow

- **01–06**: The characteristics of a thread itself — creation, lifecycle, cooperative/preemptible scheduling
- **07–13**: Synchronization/communication tools — Semaphore, Mutex, Message Queue, k_poll, k_event
- **14–17**: Timing control and resource protection — Timer, stack monitoring, deadlock, critical section
- **18–20**: Features specific to SR110 — multicore (heterogeneous AMP), power management, runtime stats
- **21–23**: Applied synthesis — Producer-Consumer, a curriculum wrap-up, a custom driver
