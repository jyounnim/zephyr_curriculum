# Zephyr Environment Setup Troubleshooting

This document lists, in the order they were actually encountered, the issues that came up while working through Steps 1–2 (setting up the west environment & Zephyr SDK) of `00_ZEPHYR_CURRICULUM_LAB.md`. Written from a Windows perspective, but the underlying causes mostly apply to macOS/Linux as well.

> 💡 **FAQ — Do I really need to run `west zephyr-export`?** No. It's not needed for the normal case of building with `west build` from inside a west workspace, because west automatically passes `ZEPHYR_BASE` to CMake at build time (confirmed in an official Zephyr GitHub discussion, #61039). It's only needed when building a "freestanding app" that lives outside the workspace, directly with CMake alone.

---

## 1. "Zephyr IDE: Setup Zephyr IDE" Doesn't Exist in the Command Palette

### Symptom

There's no command with exactly this name in the Command Palette; instead, several similarly named ones appear: `Setup Standard Workspace`, `Setup West Environment`, `Setup Workspace from Git`, `Re-run West Setup`, `Skip West Setup`, and so on

### Cause

The command structure was further broken down as the extension's version advanced.

### Fix

If you're starting fresh, run **`Zephyr IDE: Setup Standard Workspace`**. What each command does:

| Command | Role |
|---|---|
| **Setup Standard Workspace** | Creates a new workspace from scratch (sets up the west environment + downloads the SDK, all at once) — **use this when starting out** |
| Setup West Environment | Sets up only the west environment (assumes a workspace already exists) |
| Setup Workspace from Current Directory | Recognizes a workspace you already built via CLI, in the currently open folder |
| Setup Workspace from Git | Clones an existing project from git to use as the workspace |
| Re-run West Setup | Reruns the west setup on an existing workspace (for retrying after a problem) |
| Skip West Setup | Skips the west environment setup step (if you already installed it manually) |

---

## 2. `No workspace folder open. Please open a folder first.`

### Symptom

This error appears in a notification when you run a Setup command

### Cause

Every command in the Zephyr IDE extension assumes **a folder is open in VS Code**. This happens if you run a command without a folder open (i.e., on the start screen).

### Fix

1. Create an empty folder (with **no non-ASCII characters or spaces** in the path — e.g., `C:\zephyrproject`)
2. Use `File → Open Folder` to open it
3. With the folder open, rerun the Setup command

---

## 3. Some Items Show "Not Available" on the Host Tools Check Screen

### Symptom

Python3, Git, and DTC show Installed, but some others like `gperf`, `wget`, `cmake`, `ninja` show Not Available

### Cause

Some of the host tools Zephyr's build needs aren't present on the system, or aren't on PATH. The extension doesn't install them automatically — you have to install them yourself.

### Fix (Windows, `winget`)

```powershell
winget install Kitware.CMake Ninja-build.Ninja oss-winget.dtc oss-winget.gperf wget 7zip.7zip
```

After installing, you must **fully quit and reopen VS Code** for the PATH change to take effect. Return to the Host Tools screen after restarting and recheck.

---

## 4. Setup Shows Names Like `sr110_cm55`, `SRSDK` — This Is Normal for SR110

### Symptom

- A warning appears in the Problems panel that `c_cpp_properties.json` references a path like `sr110_cm55_fw`
- Notifications appear like `SRSDK_DIR set to: ...\srsdk\srsdk-main\srsdk`, `No GCC_TOOLCHAIN_* key found in settings.json`, etc.
- You're unsure whether `west boards` showing `sr100_rdk` is correct, or whether something else should be there

### What to Check

An earlier (ESP32-S3-based) version of this curriculum treated this as "a warning sign the wrong board/SDK got configured" — because seeing Cortex-M55-targeted `sr110_cm55` or `SRSDK` while working on the Xtensa-based ESP32-S3 was indeed wrong.

**Working on SR110, it's the opposite.** Seeing `sr110_cm55` (or `sr100_rdk/sr100/m55`) and `SRSDK`-related names is **expected** — that's exactly the board/SDK ecosystem this curriculum now targets. What to actually verify instead:

1. Whether `west boards | findstr sr100` lists `sr100_rdk`
2. Whether the toolchain is the confirmed `zephyr-sdk-1.0.1` (or an equivalent SR110 build) rather than an unrelated ecosystem — check the `Found toolchain:` and `Board:` lines in the `west build` log
3. Conversely, if **ESP32-related names** (`esp32s3_devkitc`, `esptool`, etc.) show up in this workspace, the workspace may have been set up against the mainline Zephyr manifest instead of `syna_zephyr_sdk` — only in that case should you rebuild it

### Bottom Line

The point isn't "unfamiliar names always mean something's wrong" — **what counts as normal here depends entirely on which board/SDK you're actually targeting.**

---

## 5. `pip: The term 'pip' is not recognized...`

### Symptom

Python itself is recognized (`python --version` works fine), but the `pip` command isn't recognized

### Cause

On Windows, the main Python executable is often on PATH, but the `Scripts` folder that contains pip isn't.

### Fix

Use `python -m pip` instead of `pip`:

```powershell
python -m pip install west
```

If the `west` command also isn't recognized afterward, substitute `python -m west` the same way. To fix this at the root:

```powershell
python -c "import sysconfig; print(sysconfig.get_path('scripts'))"
```

Add the path this prints directly to your system's PATH environment variable.

---

## 6. `PermissionError: [WinError 5] Access is denied` During `west init`

### Symptom

```
PermissionError: [WinError 5] Access is denied: '...\.west\manifest-tmp\.git\objects\pack\...'
```

### Cause

In most cases, this is Windows Defender (or another antivirus) real-time scanning a just-created git temp file and momentarily locking it. A similar problem can occur when working inside a OneDrive-synced folder.

### Fix (Try in this order)

1. **Sometimes simply retrying fixes it**:
   ```powershell
   Remove-Item -Recurse -Force .west -ErrorAction SilentlyContinue
   # TODO/VERIFY: the exact west init manifest command for syna_zephyr_sdk (see 00_ZEPHYR_CURRICULUM_LAB.md, Step 2 Method B-4)
   ```
2. **Run PowerShell as Administrator** and retry (this has actually resolved the issue in practice)
3. **Add a Windows Defender exclusion**: Windows Security → Virus & threat protection → Manage settings → Add an exclusion → add your working folder (e.g., `C:\02.work`)
4. **Check whether your working folder is outside any OneDrive-synced path** — if it's inside a synced folder, move it outside (e.g., to `C:\dev\...`) and retry

---

## 7. `ModuleNotFoundError: No module named 'jsonschema'` When Running `west boards`

### Symptom

west itself works, but running an extension command defined inside the Zephyr repository, like `west boards`, produces this error

### Cause

The Python packages listed in `zephyr/scripts/requirements.txt` aren't installed (this is a separate step from building the west workspace — `west update` only fetches the source, and the Python packages that source needs must be installed separately).

### Fix

```powershell
cd zephyr\scripts
python -m pip install -r requirements.txt
cd ..\..
```

> ⚠️ If this install fails too, check item 8 (`hidapi` build failure) below — the two commonly occur together.

---

## 8. `hidapi` Build Failure During `requirements.txt` Install Rolls Back the Entire Install

### Symptom

```
LINK : fatal error LNK1104: cannot open file '...hid.cp314-win_amd64.exp'
error: command '...\link.exe' failed with exit code 1104
ERROR: Failed building wheel for hidapi
```

Afterward, none of the other packages (including `jsonschema`) end up installed either (item 7 recurs)

### Cause

- pip installs the packages in `requirements.txt` **atomically — all-or-nothing** — if `hidapi` alone fails, the entire install, including `jsonschema`, gets rolled back
- The root cause of the `hidapi` failure is usually one of two things:
  1. A too-new Python version (e.g., 3.14) with no prebuilt wheel yet for `hidapi`, so it attempts to compile from source
  2. The Visual Studio linker used for compilation is a **preview/Insiders build**, causing it to fail at generating the `.exp` file, etc.

### Fix — Root Cause Fix (Recommended)

**Install Python 3.11 or 3.12 separately, dedicated to this workspace.** These versions have prebuilt wheels for most packages, including `hidapi`, so local compilation is never needed in the first place.

```powershell
winget install Python.Python.3.12
cd <workspace folder>
py -3.12 -m venv .venv
.venv\Scripts\Activate.ps1
python -m pip install west
python -m pip install -r zephyr\scripts\requirements.txt
```

From now on, always work with `.venv` activated in this workspace (also point VS Code's Python interpreter at it — see the Python interpreter connection step in `00_ZEPHYR_CURRICULUM_LAB.md`).

### Fix — Temporary Workaround (Not Recommended, Similar Problems May Recur)

You can try installing with just the `hidapi` line excluded from `requirements.txt`, but it's been confirmed that `hidapi` is **also referenced separately inside a nested requirements file** (`requirements-*.txt`) that the top-level file pulls in, not just at the top level — so a single filtering pass doesn't fully solve it. Also, if you place the filtered file at the workspace root, the original `requirements.txt`'s structure of referencing other files in the same folder via relative paths can produce a new error like `Could not open requirements file: requirements-base.txt` (you must filter/install **from inside** the `zephyr\scripts` folder). For these reasons, the "root cause fix" above (switching Python versions) is recommended instead.

SR110 uses CMSIS-DAP/OpenOCD-based flashing via `srsdk_tools`' `openocd_flash.py`, so `hidapi` (mainly used for USB-HID-based debug probes) usually isn't needed for building/flashing right now anyway.

---

## 9. `Please install '7z' and run this script again` When Running `west sdk install`

### Symptom

```
Zephyr SDK setup requires '7z' to be installed and available in the PATH.
FATAL ERROR: command "...\zephyr-sdk-x.y.z\setup.cmd /c" failed
```

### Cause

Extracting the Zephyr SDK requires 7-Zip, which is either not installed, or installed but not on PATH.

### Fix

```powershell
winget install 7zip.7zip
```

⚠️ **Installing 7-Zip often doesn't automatically add it to PATH.** After installing, check with `7z --help` in a new terminal. If it's not recognized, add it to PATH manually:

1. `Win + R` → `sysdm.cpl` → Enter
2. Advanced tab → Environment Variables → select **Path** under System variables → Edit
3. New → add `C:\Program Files\7-Zip` → OK
4. **Restart VS Code/your terminal** and recheck with `7z --help`
5. Retry `python -m west sdk install`

---

## 10. VS Code Doesn't Recognize the Workspace's Python (.venv)

### Symptom

`.venv` works fine in the terminal, but the Zephyr IDE extension's build/config process behaves as if it's using a different (system default) Python

### Fix

1. Command Palette → **`Python: Select Interpreter`** → select `.venv\Scripts\python.exe` (if it's not in the list, use "Enter interpreter path..." → "Find..." to specify it directly)
2. `Ctrl+,` (Settings) → Workspace tab → search `zephyr-ide` → if there's a Python-related path setting, set it to the same `.venv` path
3. **Restart VS Code** after configuring

---

## 11. `Board qualifiers 'sr100' for board 'sr100_rdk' not found`

### Symptom

```
CMake Error ... Board qualifiers `sr100` for board `sr100_rdk` not found.
Valid board targets for sr100_rdk are:
  sr100_rdk/sr100/m55
  ...
```

### Cause

SR110 uses a **heterogeneous M55/M4 AMP architecture** on Zephyr (see `18_MULTICORE_REALITY_LAB.md`) — since a separate OS image is built per core, you must specify which core to target right in the board name.

### Fix

```powershell
west build -b sr100_rdk/sr100/m55
```

A typical application (using a serial console) always uses **`m55`**. `m4` is only needed for the special dual-core IPC scenario covered in `18_MULTICORE_REALITY_LAB.md`.

> TODO/VERIFY: the exact board qualifier for the M4 side (presumably `sr100_rdk/sr100/m4`) hasn't actually been built in this curriculum pass — check the `Valid board targets` output from `west boards` for the real spelling.

---

## 12. `No prj.conf file(s) was found in the ... folder`

### Symptom

This error appears when you build a project you created by hand

### Cause

The `prj.conf` file is missing from the project folder. A Zephyr application requires this file to exist (its content can be empty).

### Fix

```powershell
New-Item -ItemType File -Name prj.conf
```

**A more reliable approach**: instead of creating the project by hand from the start, just use an official sample already included in the west workspace (`zephyr/samples/hello_world`), or register that folder with `Zephyr IDE: Add Project`. All the necessary files are already there, so this kind of mistake can't happen.

---

## 13. `ninja: error: loading 'build.ninja': The system cannot find the file specified.`

### Symptom

```
ninja: error: loading 'build.ninja': The system cannot find the file specified.
FATAL ERROR: command exited with status 1: '...\cmake.EXE' --build '...\build'
```

### Cause

The `build` folder already exists but `build.ninja` is missing or corrupted inside it (left over from a previous build attempt that failed at the CMake configuration stage). When the `build` folder exists, west tries to skip the configuration step and go straight to running `ninja`, producing this error.

### Fix

```powershell
west build -p always -b sr100_rdk/sr100/m55
```

`-p always` (pristine) ignores the existing build folder and reconfigures from scratch. Alternatively, delete the `build` folder yourself (`Remove-Item -Recurse -Force build`) and retry — same result.

---

## 14. `west flash` Doesn't Find the Right Runner for SR110, or Behaves Unexpectedly

### Symptom

Running `west flash` doesn't flash as expected, or you get errors about image path/offset

### Cause

SR110's confirmed real flashing method is running `srsdk_tools`' `openocd_flash.py` separately, rather than relying on the standard `west flash` flow (Zephyr's official `sr100_rdk` board docs also describe `west debugserver` plus running `openocd_flash.py` in a separate terminal). This is a different flashing procedure than ESP32-S3, where `west flash` alone (via esptool) is sufficient.

### Fix

```powershell
cd srsdk_tools
python openocd_flash.py `
  --openocd <path to openocd.exe> `
  --flash-offset 0x0 `
  --file-offset 0x0 `
  --cfg_path Input_Config\sr100_m55.cfg `
  --image <path to the built/converted image>.bin
```

Whether `--image` should point directly at the raw build output (`build\zephyr\zephyr.bin`) or at an image that's gone through separate signing/packaging can differ by SDK release — see the TODO/VERIFY note in the Flash section of `00_ZEPHYR_CURRICULUM_LAB.md`.

---

## 15. `cmake.exe` Crashes With Exit Code `3221226505` (`0xC0000409`, STATUS_STACK_BUFFER_OVERRUN)

### Symptom

CMake configuration proceeds quite far (sometimes even succeeding at identifying the compiler), then crashes with no explanation. `FATAL ERROR: command exited with status 3221226505`

### Cause

This happens when **your Windows account name contains non-ASCII characters** (e.g., Korean), and Python or the Zephyr SDK toolchain is installed at the default location (`C:\Users\<non-ASCII account name>\...`). This appears to occur because embedded toolchains (particularly native binaries like the Xtensa cross-compiler) don't fully support non-ASCII paths.

### Fix

Reinstall both Python and the SDK to ASCII-only paths.

**Python** — using the installer from [python.org](https://www.python.org/downloads/), choose "Customize installation" → set a custom install path (e.g., `D:\winapp\python\Python312`)

**SDK**:
```powershell
python -m west sdk install --install-base D:\zephyr_toolchains
```

After changing both paths, recreate the workspace's venv using the new Python.

```powershell
cd <workspace folder>
Remove-Item -Recurse -Force .venv
D:\winapp\python\Python312\python.exe -m venv .venv
.venv\Scripts\Activate.ps1
python -m pip install west
cd zephyr\scripts
python -m pip install -r requirements.txt
cd ..\..
```

> You don't need to change the Windows account name itself — it's enough to point the Python/SDK "install locations" at a separate ASCII-only path.

---

## 16. `west sdk install --install-base <path>` Installs to a Different (Non-ASCII) Path Instead

### Symptom

```
Zephyr SDK version 1.0.1 is already installed at C:\Users\<non-ASCII account>\zephyr-sdk-1.0.1. Using it.
```

Even though you specified a new path with `--install-base`, the log shows `Using it.` and the existing path is used as-is

### Cause

If you've ever run `west sdk install` even once before, without `--install-base` (or with a different path), west finds an SDK already installed and, due to its "reuse what's already there instead of installing fresh" logic, appears to ignore the newly specified `--install-base`.

### Fix

Delete the existing install and its registration first, then retry.

```powershell
# 1. Delete the existing SDK folder (check the actual path in the error log)
Remove-Item -Recurse -Force "C:\Users\<account name>\zephyr-sdk-1.0.1"

# 2. Also remove the leftover CMake package registry entry
Remove-Item -Path "HKCU:\Software\Kitware\CMake\Packages\Zephyr-sdk" -Recurse -Force -ErrorAction SilentlyContinue

# 3. Reinstall to the desired path
python -m west sdk install --install-base D:\zephyr_toolchains
```

---

## Troubleshooting Order Summary (Checklist for When You're Stuck)

- [ ] Is a folder open in VS Code? (#2)
- [ ] Does the folder path contain any non-ASCII characters or spaces?
- [ ] Are all Host Tools showing Installed? (#3)
- [ ] Does the downloaded SDK have an official name starting with `zephyr-sdk-`? (#4)
- [ ] If `pip`/`west` commands don't work, did you try `python -m pip`/`python -m west`? (#5)
- [ ] If you hit a permission error, did you try Administrator privileges + a Defender exclusion? (#6)
- [ ] Are you using a dedicated Python 3.11/3.12 venv? (#8 — prevents most cascading problems)
- [ ] Is 7-Zip installed and on PATH? (#9)
- [ ] Does VS Code's Python interpreter point at the workspace's `.venv`? (#10)
- [ ] Did you specify the board core in `west build` (`sr100_rdk/sr100/m55`)? (#11)
- [ ] If you built the project by hand, does `prj.conf` exist? (#12)
- [ ] If you get a `build` folder error, did you try a pristine rebuild (`-p always`)? (#13)
- [ ] Did you flash via `srsdk_tools\openocd_flash.py` instead of relying on `west flash`? (#14)
- [ ] **If your Windows account name contains non-ASCII characters, did you point Python's and the SDK's install paths at an ASCII-only location?** (#15 — the most fundamental preventive measure)
- [ ] If `west sdk install --install-base` seems ignored, did you delete the existing install first? (#16)
