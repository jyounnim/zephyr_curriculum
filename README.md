# Zephyr RTOS Curriculum Overview

A 23-lab series for learning Zephyr RTOS's API and design philosophy, in order. Each lab is a standalone `.md` file, designed so the concepts build on each other if you go through them in numeric order. It covers everything from thread creation, synchronization/communication tools, and timing control, to multicore and power management on the ESP32-S3, and finally writing a custom device driver.

> ⚠️ **PlatformIO does not support the Zephyr framework for ESP32 chips.** Even if you set `framework = zephyr` in `platformio.ini`, it won't build on the espressif32 platform — this has been confirmed in the PlatformIO community and on GitHub issues. So this curriculum uses Zephyr's own official tool, **west**, and VS Code is used as an editor plus build/flash GUI via the "Zephyr IDE" extension.

> 📄 **If you hit an error during setup**: Steps 1–2 below describe the flow "when everything goes smoothly." In practice you may run into a number of issues along the way (missing host tools, Python version compatibility, permission problems, etc.) — these individual errors are covered separately in **`ZEPHYR_SETUP_TROUBLESHOOTING.md`**. Keep that document open alongside this one.

## Prerequisites

- A PC (Windows / macOS / Linux; WSL2 is recommended for Windows)
- Python 3 and Git installed
- An ESP32-S3 board (a USB cable that supports data transfer)
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

### Method A — Using the Zephyr IDE Extension (GUI-based)

> ⚠️ In practice, you'll hit several points that require manual intervention, as shown below. Rather than expecting "the extension does everything in a few clicks," it's best to work through each step below and confirm it as you go. Most sticking points are covered in `ZEPHYR_SETUP_TROUBLESHOOTING.md`.

1. **You must open an empty folder first.** Every command in the Zephyr IDE extension operates relative to "the folder currently open in VS Code" — running a command without a folder open produces the error `No workspace folder open. Please open a folder first.`
   - Use `File → Open Folder` to open a newly created **empty folder**
   - We recommend a path with **no non-ASCII characters or spaces** (e.g., `C:\zephyrproject`) — paths containing non-ASCII characters or spaces have caused problems later in the west/CMake build process

2. Click the **Zephyr IDE icon** in the left Activity Bar (the extension's official name is "**IDE for Zephyr**"; if the icon doesn't appear right after installing, restart VS Code)

3. In the Command Palette (`Ctrl+Shift+P`), run **`Zephyr IDE: Setup Standard Workspace`**
   - ⚠️ There are several similarly named commands (`Setup West Environment`, `Setup Workspace from Git`, `Setup Workspace from Current/External Directory`, `Re-run West Setup`, `Skip West Setup`, and so on). If you're starting fresh, be sure to pick **Setup Standard Workspace**

4. A **Host Tools check screen** appears — a list of required tools (Python3, Git, CMake, Ninja, DTC/Devicetree Compiler, gperf, wget, etc.), each marked Installed or Not Available
   - **Everything marked Not Available must be installed before continuing** — skipping ahead without installing them can cause the process to silently stall at a later step with no error message
   - On Windows, install everything at once with `winget` (run PowerShell **as Administrator** first):
     ```powershell
     winget install Kitware.CMake Ninja-build.Ninja oss-winget.dtc oss-winget.gperf wget 7zip.7zip
     ```
   - After installing, you must **fully quit and reopen VS Code** for the PATH change to take effect (simply reloading the window isn't enough)
   - After restarting, return to the Host Tools screen and confirm everything now shows Installed

5. Once all Host Tools pass, it automatically proceeds through creating the west workspace → downloading Zephyr sources → installing the SDK (this can take anywhere from a few minutes to tens of minutes, depending on your network speed)

6. ⚠️ **Important — always verify the SDK/board that gets auto-selected.** We've confirmed cases where the automatic setup process picks an unintended default SDK (for example, instead of the official `zephyr-sdk-x.y.z`, it pulls down something named `SRSDK` — a sample SDK for a Cortex-M-based board like `sr110_cm55` that has nothing to do with the ESP32-S3). If this happens, `west boards` won't list any esp32s3-family boards at all afterward.
   - In the Output panel (channel: "IDE for Zephyr"), check that the SDK name being downloaded is the official one, starting with `zephyr-sdk-`
   - If you see an unfamiliar name, **stop the automated flow and start over from scratch using Method B (manual CLI) instead** — when this problem actually occurred, the CLI proved far more predictable and reliable than the GUI's automatic flow

7. **Connecting the Python Interpreter**
   - VS Code needs to know which Python to use for west/pip-related scripts
   - Command Palette → **`Python: Select Interpreter`** → select the virtual environment (`.venv`, usually inside the workspace folder) the extension created. If it's not in the list, use "Enter interpreter path..." → "Find..." and point directly to `.venv\Scripts\python.exe`
   - The Zephyr IDE extension may also have its own separate Python path setting — `Ctrl+,` (Settings) → Workspace tab → search `zephyr-ide` → if there's a Python path field, set it the same way

8. Once everything above completes successfully, move on to **Step 3 (Creating a New Project)**

> In practice, **step 4 (Host Tools)** and **step 6 (the wrong SDK)** are where things get stuck most often. If you keep getting stuck, rather than forcing repeated GUI attempts, it's more reliable to first build the west workspace with the CLI using **Method B** below, then use the `Zephyr IDE: Setup Workspace from Current Directory` command to have the extension recognize that workspace.

### Method B — Manual Installation (Terminal CLI; More Reliable When Things Go Wrong · Recommended)

> 💡 This is the sequence that emerged after several rounds of real-world troubleshooting. It worked all the way through even on a Windows setup with a non-ASCII account name, so **we recommend using this method from the start.**

**Core principle**: point the install locations for Python and the Zephyr SDK toolchain **directly at ASCII-only paths** (this works regardless of whether your account name itself contains non-ASCII characters).

#### B-1. Install Python to an ASCII-Only Path (Required if Your Windows Account Name Contains Non-ASCII Characters)

Installing via `winget` puts it in the default location (under your user folder), which can reintroduce the non-ASCII path problem. Instead, download the installer for **Python 3.12** (3.13 and above have been observed to fail installing some packages, like `hidapi`, because prebuilt wheels aren't available yet) directly from [python.org](https://www.python.org/downloads/).

1. Run the installer → select **"Customize installation"** (do not click Install Now — it won't let you set a custom path)
2. Optional Features → leave the defaults, click Next
3. Advanced Options → enter an ASCII-only path under **"Customize install location"** (e.g., `D:\winapp\python\Python312`)
4. Install

Verify:
```powershell
D:\winapp\python\Python312\python.exe --version
```

(If your account name is already ASCII-only, you can skip this step and just install conveniently with `winget install Python.Python.3.12`)

#### B-2. Create a Workspace Folder + Dedicated Virtual Environment (venv)

```powershell
mkdir D:\work\zephyrproject
cd D:\work\zephyrproject
D:\winapp\python\Python312\python.exe -m venv .venv
.venv\Scripts\Activate.ps1        # macOS/Linux: source .venv/bin/activate
```

(You'll know it's active when `(.venv)` appears in front of your prompt — run every subsequent command in this state)

#### B-3. Install west

```powershell
python -m pip install west
```

#### B-4. Initialize the west Workspace + Download Zephyr Source/HAL Modules

```powershell
python -m west init -m https://github.com/zephyrproject-rtos/zephyr --mr v1.0.0 .
python -m west update              # hundreds of MB to 1GB+, can take a while depending on your network
```

#### B-5. Install Zephyr's Script Dependencies

```powershell
# Must be run "from inside" the zephyr/scripts folder
# (because requirements.txt references other requirements-*.txt files in the same folder via relative paths)
cd zephyr\scripts
python -m pip install -r requirements.txt
cd ..\..
```

#### B-6. Install the Zephyr SDK (to an ASCII-Only Path)

```powershell
python -m west sdk install --install-base D:\work\zephyr_toolchains
```

> ⚠️ If you've ever run `west sdk install` even once before without `--install-base`, west may find an SDK already installed at the default location (`C:\Users\<account name>\zephyr-sdk-x.y.z` — including your non-ASCII account path, if applicable) and reuse it, making `--install-base` appear to be ignored. If that happens, delete the existing install (`Remove-Item -Recurse -Force`) and also remove its CMake package registry entry (`Remove-Item -Path "HKCU:\Software\Kitware\CMake\Packages\Zephyr-sdk" -Recurse -Force`), then try again. See `ZEPHYR_SETUP_TROUBLESHOOTING.md` for details.

#### B-7. Install Flashing Tools (esptool, etc.)

```powershell
python -m west packages pip --install
```

#### Verify the Installation

```powershell
python -m west --version
python -m west boards | findstr esp32s3    # macOS/Linux: grep esp32s3
```

If ESP32-S3-related board targets like `esp32s3_devkitc` show up, everything is working.

> 💡 `west zephyr-export` is intentionally absent from this sequence — for the normal case of building with `west build` from inside a west workspace, west automatically passes `ZEPHYR_BASE` to CMake, making this step **unnecessary** (confirmed in an official Zephyr GitHub discussion). It's only needed in the special case of building a "freestanding app" that lives outside the workspace, directly with CMake alone.

> ⚠️ If you get `PermissionError: Access is denied` during `west init`/`west update`, or `hidapi` fails to build while installing `requirements.txt`, or `west sdk install` complains it can't find `7z` — these are all real, confirmed issues. **See `ZEPHYR_SETUP_TROUBLESHOOTING.md` for the causes and fix order.**

### Connecting VS Code to a west Workspace (If You Installed via CLI Using Method B)

If you built the workspace directly via the CLI using Method B, you'll need to connect it so the VS Code Zephyr IDE extension recognizes it.

1. In VS Code, open the workspace folder you just created (`zephyrproject`)
2. Command Palette → run **`Zephyr IDE: Setup Workspace from Current Directory`**
3. So the GUI and CLI point at the same Python/SDK, set these two values in Settings (`Ctrl+,`) to match the paths you used on the CLI:
   ```json
   {
     "zephyr-ide.venvFolder": "D:\\work\\zephyrproject\\.venv",
     "zephyr-ide.toolchainDirectory": "D:\\work\\zephyr_toolchains"
   }
   ```
   If you don't align these, the GUI may end up referencing a different venv/SDK than the CLI, leading to confusing situations like "it works in PowerShell but not in the GUI."

---

## Step 3. Creating a New Project

> 💡 **Rather than creating files by hand, we recommend using one of the official samples that's already complete.** Mistakes like forgetting `prj.conf` are common, so it's much safer to just use `zephyr/samples/hello_world`, which is already included in the west workspace.

### Recommended — Use an Existing Sample (CLI)

```powershell
cd zephyr\samples\hello_world
```

This folder already has `CMakeLists.txt`, `prj.conf`, and `src/main.c` fully set up. Go straight to the build command in Step 4.

### If Using the Zephyr IDE Extension (GUI)

1. Zephyr IDE panel → click **Create Project**
2. When the **Select Sample Project** search box appears, don't assume it's missing just because the list looks empty — **type `hello` directly** to filter it. `hello_world` is a core Zephyr sample, so it's always included in the workspace
3. If it still doesn't show up, use the **`Zephyr IDE: Add Project`** command and point it directly at:
   ```
   <workspace folder>\zephyr\samples\hello_world
   ```

### If You Want to Create a New Project by Hand (Advanced, Not Recommended at First)

Once you're comfortable, if you want to build the structure yourself, you must include **every one of these**, with nothing missing — in particular, `prj.conf` can be empty, but the build will fail if the file doesn't exist at all.

```
my_app/
├── CMakeLists.txt
├── prj.conf              ← kernel config (Kconfig) — this is where this curriculum's CONFIG_EVENTS, CONFIG_PM, etc. go. If this file is missing, you get "No prj.conf file(s) was found"
├── boards/
│   └── esp32s3_devkitc.overlay   ← board-specific hardware config (used in Lab 07 and others; not needed for Hello World)
└── src/
    └── main.c
```

Minimal `CMakeLists.txt` example:
```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(my_app)
target_sources(app PRIVATE src/main.c)
```

> ⚠️ If your board has a specific memory configuration like 16MB Flash / 8MB PSRAM (N16R8), you'll need to add snippet flags like `-S flash-16M -S psram-8M` to the `west build` command, or reflect them in an overlay file. Check your board's datasheet first.

---

## Step 4. Writing & Running Hello World

If you're using the existing sample (`zephyr/samples/hello_world`) as-is, there's no code to write — the following is just for reference.

### `src/main.c`

```c
#include <zephyr/kernel.h>

int main(void) {
    while (1) {
        printk("Hello, ESP32-S3! (Zephyr)\n");
        k_sleep(K_SECONDS(1));
    }
    return 0;
}
```

### Build · Flash · Monitor

```bash
west build -b esp32s3_devkitc/esp32s3/procpu
west flash
west espressif monitor
```

> ⚠️ **You must write out the board name in full: `esp32s3_devkitc/esp32s3/procpu`.** If you only write `esp32s3_devkitc`, you'll get the error below — on Zephyr, the ESP32-S3 builds a **separate image per core (an AMP architecture)** (see `18_MULTICORE_REALITY_LAB.md`), so you must specify which core to target (`procpu` or `appcpu`). For a typical case that needs a serial console, like Hello World, always use **`procpu`**.
> ```
> Board qualifiers `esp32s3` for board `esp32s3_devkitc` not found.
> Valid board targets for esp32s3_devkitc are:
>   esp32s3_devkitc/esp32s3/procpu
>   esp32s3_devkitc/esp32s3/appcpu
> ```

If you're using the Zephyr IDE extension, you can run the same steps via the GUI's **Build** / **Flash** / **Monitor** buttons.

### Run & Verify

- Confirm `Hello, ESP32-S3! (Zephyr)` prints in the serial monitor once per second
- If nothing appears, try pressing the board's RESET button; if that still doesn't work, check your USB cable/port

### Note — If You Get a `build` Folder Error

If a previous failed build attempt left the `build` folder in an incomplete state, you might see an error like `ninja: error: loading 'build.ninja': The system cannot find the file specified.` In that case, just reconfigure from scratch (a pristine build).

```bash
west build -p always -b esp32s3_devkitc/esp32s3/procpu
```

Or delete the `build` folder yourself (`Remove-Item -Recurse -Force build`) and retry — same result.

## Setup Checklist

- [ ] (If your Windows account name contains non-ASCII characters) Install Python to an ASCII-only path
- [ ] Open an empty folder in VS Code (ASCII-only path, no spaces)
- [ ] Install the Zephyr IDE extension and restart VS Code
- [ ] Confirm every item shows Installed on the Host Tools check screen (if anything shows Not Available, install it with winget and restart VS Code)
- [ ] Finish setting up the west workspace — confirm the SDK was installed to an ASCII-only path (`--install-base` or `zephyr-ide.toolchainDirectory`)
- [ ] Confirm the board target name with `west boards | findstr esp32s3` (Windows) or `grep esp32s3` (macOS/Linux)
- [ ] Confirm the Python interpreter is set to the workspace's `.venv` (the GUI and CLI should use the same venv)
- [ ] Prepare a project from an existing sample like `zephyr/samples/hello_world` (Step 3)
- [ ] `west build -b esp32s3_devkitc/esp32s3/procpu` → `west flash` → confirm via the monitor (Step 4, **the board qualifier `/esp32s3/procpu` is required**)

> `west sdk install` and the initial `west update` download a lot of data and can take a while. Check your network beforehand.

## Troubleshooting (Build/Run Stage)

> For issues that occur during environment setup (Steps 1–2), see **`ZEPHYR_SETUP_TROUBLESHOOTING.md`**. The table below covers issues that come up at the build/flash/run stage, after the workspace is already set up correctly.

| Symptom | Cause / Fix |
|---|---|
| No esp32s3-related boards show up in `west boards` | `west update` didn't complete, or the Espressif HAL module is missing — rerun `west update` |
| `Board qualifiers 'esp32s3' for board 'esp32s3_devkitc' not found` | The core wasn't specified in the board name — write out the full `esp32s3_devkitc/esp32s3/procpu` |
| `No prj.conf file(s) was found` | You built the project by hand and left out `prj.conf` — it must exist even if empty, or better, use an existing sample (`hello_world`) |
| `ninja: error: loading 'build.ninja': ... cannot find the file` | A previous failed build left the `build` folder incomplete — do a pristine rebuild with `west build -p always -b <board>` |
| `esptool>=5.0.2 not found in PATH` | The flashing tool isn't installed — run `west packages pip --install` |
| A blob-related error from the Espressif HAL during the build | The Espressif HAL needs separate RF-related binary blobs — run `west blobs fetch hal_espressif` |
| `west build` fails (can't find the board) | Re-verify the exact board target name with `west boards | grep esp32s3` (the display format can vary by Zephyr version) |
| `west: command not found` in a new terminal | The venv isn't activated — rerun `.venv\Scripts\Activate.ps1` (or `source .venv/bin/activate`) |
| `cmake.exe` crashes with exit code `3221226505` (`0xC0000409`) | **A non-ASCII path problem** — your Python/SDK install path contains non-ASCII characters (e.g., from your account name). See the non-ASCII path issue in `ZEPHYR_SETUP_TROUBLESHOOTING.md` |
| No serial output visible | Instead of `west espressif monitor`, use `screen`/`minicom` at baud rate 115200, and restart via the RESET button |

---

## The Core of Zephyr's Scheduling — Priority Numbers and the Cooperative/Preemptible Split

**In Zephyr, the smaller the number, the higher the priority.** And Zephyr clearly splits threads into two categories: **cooperative (negative priority)** and **preemptible (priority 0 or above)**. This concept gets heavy focus early in the curriculum (Labs 02 and 04).

## Common Notes (Applies From Lab 01 Onward)

- Most labs can be verified with `printk()` alone; labs that need hardware wiring (Lab 07) say so explicitly
- The output strings in the code are written in English
- The build command for every subsequent lab is the same as in Step 4 above: `west build -b esp32s3_devkitc/esp32s3/procpu` → `west flash` → `west espressif monitor`

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
| 18 | `18_MULTICORE_REALITY_LAB.md` | Multicore on the ESP32-S3 — the AMP architecture |
| 19 | `19_POWER_MANAGEMENT_LAB.md` | Zephyr Power Management (prj.conf) |
| 20 | `20_RUNTIME_STATS_LAB.md` | Thread Runtime Stats — the CPU usage API |
| 21 | `21_PRODUCER_CONSUMER_LAB.md` | Producer-consumer synthesis pattern |
| 22 | `22_ZEPHYR_SUMMARY_LAB.md` | Full curriculum wrap-up |
| 23 | `23_CUSTOM_DEVICE_DRIVER_LAB.md` | Building a custom device driver (AHT20 example) |

## Learning Flow

- **01–06**: The characteristics of a thread itself — creation, lifecycle, cooperative/preemptible scheduling
- **07–13**: Synchronization/communication tools — Semaphore, Mutex, Message Queue, k_poll, k_event
- **14–17**: Timing control and resource protection — Timer, stack monitoring, deadlock, critical section
- **18–20**: Features specific to the ESP32-S3 — multicore (AMP), power management, runtime stats
- **21–23**: Applied synthesis — Producer-Consumer, a curriculum wrap-up, a custom driver
