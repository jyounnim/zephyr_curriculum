# Zephyr RTOS 커리큘럼 개요

Zephyr RTOS의 API와 설계 철학을 순서대로 배우는 23개 랩 시리즈입니다. 각 랩은 독립된 `.md` 파일이며, 번호 순서대로 진행하면 개념들이 서로 쌓이도록 설계되어 있습니다. 스레드 생성, 동기화/통신 도구, 타이밍 제어부터 **Synaptics SR110(Astra Machina Micro)**에서의 멀티코어·전력관리, 마지막으로 커스텀 디바이스 드라이버 작성까지 다룹니다.

> 📄 **설정 중 에러가 나면**: 아래 Step 1~2는 "모든 게 순조로울 때"의 흐름입니다. 실제로는 다양한 문제(호스트 도구 누락, Python 버전 호환성, 권한 문제 등)를 겪을 수 있는데, 이런 개별 에러들은 **`ZEPHYR_SETUP_TROUBLESHOOTING.md`**에 따로 정리해뒀습니다. 이 문서와 함께 열어두세요.

## 사전 준비물

- PC (이 커리큘럼의 설정 단계는 Windows 기준으로 확인됨; macOS/Linux도 큰 틀은 비슷할 것으로 예상되나 SR110 기준으로 별도 재검증은 안 됨)
- Python 3, Git 설치
- SR110 RDK 보드(Astra Machina Micro), USB-C 케이블, 온보드 CMSIS-DAP 디버그 인터페이스(플래시/콘솔용)
- 최소 5GB 여유 디스크 공간 (Zephyr SDK + 모듈 소스 다운로드용)
- 인터넷 연결 (초기 설치 시 데이터 다운로드가 많음 — 네트워크 먼저 확인)

> 🚨 **Windows에서 계정명(사용자 폴더명, `C:\Users\???`)에 한글 등 non-ASCII 문자가 포함된 경우, 이것부터 읽으세요.** Python과 Zephyr SDK 툴체인은 기본적으로 사용자 폴더 아래(`C:\Users\<계정명>\...`)에 설치되는데, 이 경로에 non-ASCII 문자가 섞여 있으면 `cmake.exe`가 크래시(`STATUS_STACK_BUFFER_OVERRUN`, 종료코드 `3221226505`)하는 게 실제로 확인됐습니다. **계정명 자체를 바꿀 필요는 없고, Python과 SDK의 설치 위치만 ASCII 전용 경로로 지정**하면 됩니다. 아래 Step 2의 방법 B가 처음부터 이렇게 구성되어 있습니다.

---

## Step 1. VS Code에 Zephyr IDE 확장 설치

1. VS Code 확장(`Ctrl+Shift+X`)에서 `Zephyr IDE` 검색
2. **mylonics**가 만든 **Zephyr IDE** 확장 설치
3. 설치 후 VS Code 재시작

---

## Step 2. west 환경 & Zephyr SDK 설정

### 중요 — SR110은 mainline Zephyr가 아니라 Synaptics의 `syna_zephyr_sdk`를 씁니다

일반적인 Zephyr 보드와 달리, SR110 지원은 Synaptics 자체 포크/벤더 리포인 `syna_zephyr_sdk`에 들어있습니다 — Zephyr 커널 트리(`zephyr/`)와 SR100 계열 보드/SoC 정의(`zephyr_srsdk/`)를 함께 벤더링하고, 플래싱 도구(`srsdk_tools/`)도 같이 있습니다.

**이 커리큘럼 자체는 별도의 git 리포이며, `syna_zephyr_sdk`와 나란히(그 안에 중첩되지 않고) 독립적으로 clone합니다.**

```powershell
git clone https://github.com/jyounnim/zephyr_curriculum
```

두 리포를 같은 부모 폴더 아래 나란히 clone하면, 실제로 확인된 워크스페이스 구조는 다음과 같습니다.

```
<워크스페이스 루트>/           예: C:\02.work\syna_zephry\syna_zephry_sdk
├── zephyr/                 벤더링된 Zephyr 커널 트리 (syna_zephyr_sdk)
├── zephyr_srsdk/            SR100 계열 보드/SoC/pinctrl 정의 (syna_zephyr_sdk)
├── zephyr_curriculum/       이 커리큘럼, 별도로 clone (Step 3 참고)
├── srsdk_tools/              플래싱 스크립트 (openocd_flash.py 등)
└── build/                   빌드 산출물 (`west build`가 생성)
```

TODO/VERIFY: `syna_zephyr_sdk` 구조를 처음부터 만드는 정확한 `west init -m <매니페스트 리포 URL>` 명령은 이번 커리큘럼 작업에서 재확인하지 못했습니다 — 이 리포는 보통 공개 매니페스트 대상 `west init`이 아니라 Synaptics의 릴리스/체크아웃 형태로 받는 경우가 많습니다. 현재 공식 절차는 Astra MCU SDK의 공식 설치 가이드(`synaptics-astra-mcu.github.io`의 `Astra_MCU_SDK_Setup_and_Install_CLI`/`_VsCode`)를 확인하세요 — SDK 릴리스마다 바뀔 수 있습니다.

### 방법 A — Zephyr IDE 확장 사용 (GUI 기반)

> ⚠️ 실제로는 아래처럼 수동 개입이 필요한 지점이 여러 곳 있습니다. "확장이 몇 번 클릭으로 다 해준다"고 기대하기보다, 아래 단계를 하나씩 확인해가며 진행하는 게 좋습니다. 대부분의 걸림돌은 `ZEPHYR_SETUP_TROUBLESHOOTING.md`에 정리되어 있습니다.

1. **먼저 빈 폴더(또는 이미 있는 `syna_zephyr_sdk` 워크스페이스 루트)를 열어야 합니다.** Zephyr IDE 확장의 모든 명령은 "VS Code에 현재 열려있는 폴더"를 기준으로 동작합니다 — 폴더를 안 열고 명령을 실행하면 `No workspace folder open. Please open a folder first.` 에러가 납니다
   - 경로에 **한글/공백 없이** 잡는 걸 권장합니다(예: `C:\02.work\syna_zephry\syna_zephry_sdk`) — non-ASCII 문자나 공백이 섞인 경로는 이후 west/CMake 빌드 과정에서 문제를 일으킨 사례가 있습니다
2. 왼쪽 Activity Bar에서 **Zephyr IDE 아이콘** 클릭 (확장의 공식 이름은 "**IDE for Zephyr**"이며, 설치 직후 아이콘이 안 보이면 VS Code를 재시작하세요)
3. **Host Tools 체크 화면**이 뜹니다 — 필요한 도구들(Python3, Git, CMake, Ninja, DTC/Devicetree Compiler, gperf, wget 등) 목록이 Installed/Not Available로 표시됨
   - **Not Available로 표시된 항목은 모두 설치해야 진행할 수 있습니다** — 건너뛰면 이후 단계에서 에러 메시지 없이 조용히 멈출 수 있습니다
   - Windows에서는 `winget`으로 한 번에 설치 (PowerShell을 **관리자 권한**으로 먼저 실행):
     ```powershell
     winget install Kitware.CMake Ninja-build.Ninja oss-winget.dtc oss-winget.gperf wget 7zip.7zip
     ```
   - 설치 후 PATH 반영을 위해 **VS Code를 완전히 종료했다가 다시 열어야** 합니다 (창 새로고침만으로는 부족함)
4. 설치되는 툴체인이 실제로 SR110에 맞는 것인지 확인하세요. 실제 확인된 설정에서는 `zephyr-sdk-1.0.1`이 `<사용자 폴더>\.zephyr_ide\toolchains\zephyr-sdk-1.0.1`에 설치되어 있었고, `ZEPHYR_TOOLCHAIN_VARIANT`가 설정 안 된 상태에서 `west build`가 자동으로 이걸 찾아 `Found toolchain: zephyr 1.0.1`이라고 출력했습니다. 만약 안 맞는/일반 mainline `zephyr-sdk-x.y.z`가 대신 잡힌다면, SR100 pinctrl이나 DesignWare I2C/GPIO 드라이버 같은 보드/SoC 특화 요소들을 못 쓸 수 있습니다
5. **Python 인터프리터 연결**
   - VS Code가 west/pip 관련 스크립트에 쓸 Python을 알아야 합니다
   - Command Palette → **`Python: Select Interpreter`** → 확장이 만든 가상환경(보통 워크스페이스 폴더 안) 선택. 실제 확인된 설정에서는 `venv313`(Python 3.13) 가상환경을 `.venv\Scripts\Activate.ps1` 형태의 스크립트로 활성화했습니다
6. 위 과정이 모두 끝나면 **Step 3(새 프로젝트 만들기)**로 넘어갑니다

### 방법 B — 수동 설치 (터미널 CLI; 문제 생겼을 때 더 안정적 · 권장)

**핵심 원칙**: Python과 Zephyr SDK 툴체인의 설치 위치를 **ASCII 전용 경로로 직접 지정**합니다 (계정명 자체가 non-ASCII인지 여부와 무관하게 적용).

#### B-1. Python을 ASCII 전용 경로에 설치 (Windows 계정명에 non-ASCII 문자가 있으면 필수)

`winget`으로 설치하면 기본 위치(사용자 폴더 아래)에 들어가서 non-ASCII 경로 문제가 재발할 수 있습니다. 대신 [python.org](https://www.python.org/downloads/)에서 **Python 3.12 또는 3.13** 설치 파일을 직접 받으세요 (실제 확인된 설정은 Python 3.13.7을 `venv313`으로 문제없이 사용했습니다 — 이전 ESP32-S3 기준 커리큘럼에서는 3.13 이상에서 prebuilt wheel이 없어 일부 패키지 설치가 실패할 수 있다고 경고했었는데, SR110에서는 이 문제가 재현되지 않았습니다. 다만 참고는 해두세요).

1. 설치 파일 실행 → **"Customize installation"** 선택 (Install Now를 누르면 경로를 못 정합니다)
2. Optional Features → 기본값 그대로 Next
3. Advanced Options → **"Customize install location"**에 ASCII 전용 경로 입력
4. 설치

#### B-2. 워크스페이스 폴더 + 전용 가상환경(venv) 생성

```powershell
mkdir C:\02.work\syna_zephry
cd C:\02.work\syna_zephry
python -m venv venv313
venv313\Scripts\Activate.ps1        # macOS/Linux: source venv313/bin/activate
```

(프롬프트 앞에 `(venv313)`이 붙으면 활성화된 것 — 이후 모든 명령은 이 상태에서 실행)

#### B-3. west 설치

```powershell
python -m pip install west
```

#### B-4. `syna_zephyr_sdk` 받기

TODO/VERIFY: 현재 SDK 릴리스의 정확한 `west init`/clone 명령은 확인 필요. Astra MCU SDK의 공식 CLI 설치 가이드(`synaptics-astra-mcu.github.io/doc/v/latest/srsdk/docs/Astra_MCU_SDK_Setup_and_Install_CLI.html`)를 참고하세요 — 이번 커리큘럼에서 실제 작업할 때는 워크스페이스가 이미 구성되어 있는 상태에서 시작해서, 정확한 획득 명령은 재검증하지 못했습니다.

#### B-5. Zephyr 스크립트 의존성 설치

```powershell
cd zephyr\scripts
python -m pip install -r requirements.txt
cd ..\..
```

#### B-6. Zephyr SDK 설치 (ASCII 전용 경로에)

```powershell
python -m west sdk install --install-base D:\work\zephyr_toolchains
```

> ⚠️ 예전에 `--install-base` 없이 `west sdk install`을 한 번이라도 실행한 적 있다면, west가 기본 위치에 이미 설치된 SDK를 찾아서 재사용해버려 `--install-base`가 무시된 것처럼 보일 수 있습니다. 이 경우 기존 설치를 삭제하고, CMake 패키지 레지스트리 항목도 함께 지운 뒤(`Remove-Item -Path "HKCU:\Software\Kitware\CMake\Packages\Zephyr-sdk" -Recurse -Force`) 다시 시도하세요.

#### 설치 확인

```powershell
python -m west --version
python -m west boards | findstr sr100    # macOS/Linux: grep sr100
```

`sr100_rdk`가 보드 타깃으로 뜨면 정상입니다.

> 💡 `west zephyr-export`는 이 순서에 일부러 넣지 않았습니다 — west 워크스페이스 안에서 `west build`로 빌드하는 일반적인 경우엔, west가 빌드 시점에 `ZEPHYR_BASE`를 자동으로 CMake에 넘겨주기 때문에 **필요 없습니다** (Zephyr 공식 GitHub 토론에서 확인). 워크스페이스 밖의 "freestanding 앱"을 CMake만으로 직접 빌드하는 특수한 경우에만 필요합니다.

### VS Code를 west 워크스페이스에 연결 (방법 B로 CLI 설치한 경우)

1. VS Code에서 워크스페이스 폴더 열기
2. Command Palette → **`Zephyr IDE: Setup Workspace from Current Directory`** 실행
3. GUI와 CLI가 같은 Python/SDK를 가리키도록, Settings(`Ctrl+,`)에서 아래 두 값을 CLI에서 쓴 경로와 맞춰줍니다:
   ```json
   {
     "zephyr-ide.venvFolder": "C:\\02.work\\syna_zephry\\venv313",
     "zephyr-ide.toolchainDirectory": "D:\\work\\zephyr_toolchains"
   }
   ```

---

## Step 3. 커리큘럼 리포 받기 / 새 프로젝트 만들기

이 커리큘럼 자체의 리포를 워크스페이스 루트에 `syna_zephyr_sdk`의 폴더들과 나란히 clone합니다.

```powershell
cd <워크스페이스 루트>       # zephyr/, zephyr_srsdk/, srsdk_tools/의 부모 디렉토리
git clone https://github.com/jyounnim/zephyr_curriculum
```

모든 랩은 `zephyr_curriculum/<랩 이름>/lab/` 아래에 있습니다 — 실제 사용에서 확인된 이 관례는 각 랩의 `src/`, `CMakeLists.txt`, `prj.conf`, `boards/*.overlay`를 독립적으로 유지하고, 아래 west 빌드 명령이 (랩 디렉토리로 `cd`하지 않고) 워크스페이스 루트에서 경로 인자로 호출되는 방식과 맞습니다.

```
zephyr_curriculum/
└── <랩 이름>/
    ├── <랩 이름>_KR.md
    ├── <랩 이름>_EN.md
    └── lab/
        ├── CMakeLists.txt
        ├── prj.conf              ← 커널 설정(Kconfig) — 이 커리큘럼의 CONFIG_EVENTS, CONFIG_PM 등이 여기 들어감. 파일이 없으면 "No prj.conf file(s) was found" 에러
        ├── boards/
        │   └── sr100_rdk_sr100_m55.overlay   ← 보드별 하드웨어 설정 (07번, 23번 등에서 사용). 파일명을 board target(슬래시를 밑줄로 치환)과 정확히 맞추면 별도 CMake 플래그 없이 Zephyr가 자동 인식함
        └── src/
            └── main.c
```

기존 랩을 그대로 쓰지 않고 직접 새 랩을 만들고 싶다면, 최소 `CMakeLists.txt` 예시:
```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(my_app)
target_sources(app PRIVATE src/main.c)
```

---

## Step 4. Hello World 작성 & 실행

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

워크스페이스 루트(`zephyr_curriculum`의 부모 디렉토리)에서 실행합니다.

```powershell
west build -p always -b sr100_rdk/sr100/m55 .\zephyr_curriculum\<랩 이름>\lab\
```

> ⚠️ **board target은 반드시 `sr100_rdk/sr100/m55` 전체를 써야 합니다.** 실제 확인된 동작: 랩에서 devicetree 오버레이가 필요하면(예: 기본적으로 비활성화된 I2C0을 켜는 경우) 오버레이 파일명을 정확히 `boards/sr100_rdk_sr100_m55.overlay`로 지으면 자동 인식됩니다. 같은 파일을 가리키는 `-DEXTRA_DTC_OVERLAY_FILE=...`을 **추가로 붙이지 마세요** — 중복 지정 시 CMake 인자 처리에서 잘못 쪼개지면서 devicetree 전처리 자체가 실패하는 게 확인됐습니다.

### Flash

SR110 플래싱은 `west flash`가 아니라 `srsdk_tools`를 통해 진행합니다.

```powershell
cd srsdk_tools
python openocd_flash.py `
  --openocd <openocd.exe 경로> `
  --flash-offset 0x0 `
  --file-offset 0x0 `
  --cfg_path Input_Config\sr100_m55.cfg `
  --image <빌드/변환된 이미지 경로>.bin
```

(Zephyr 공식 upstream의 `sr100_rdk` 보드 문서에는 더 단순한 형태인 `python openocd_flash.py build/zephyr/zephyr.bin 0x0 0x0 1`가 나와 있습니다 — 정확한 플래그와 raw `zephyr.bin`을 쓰는지 변환/서명된 이미지를 쓰는지는 SDK 릴리스마다 다를 수 있습니다. TODO/VERIFY: `build\zephyr\zephyr.bin`에서 실제 `openocd_flash.py`가 기대하는 이미지로 변환하는 정확한 절차)

### Console

시리얼 터미널을 **230400bps 8N1**로 열고(여러 랩에서 실사용으로 확인됨) `Hello, SR110! (Zephyr)`가 1초마다 출력되는지 확인하세요.

### 참고 — `build` 폴더 에러가 날 때

이전 빌드가 실패해서 `build` 폴더가 불완전한 상태로 남아있으면 `ninja: error: loading 'build.ninja': The system cannot find the file specified.` 같은 에러가 날 수 있습니다. 이 경우 위 빌드 명령에 이미 포함된 `-p always`로 처음부터 다시 구성하거나, `build` 폴더를 직접 지우고 재시도하세요.

## 설정 체크리스트

- [ ] (Windows 계정명에 non-ASCII 문자가 있다면) Python을 ASCII 전용 경로에 설치
- [ ] Zephyr IDE 확장 설치 및 VS Code 재시작
- [ ] Host Tools 체크 화면에서 모든 항목이 Installed인지 확인 (Not Available이면 winget으로 설치 후 VS Code 재시작)
- [ ] 툴체인이 SR110에 맞는 것인지 확인 (실제 확인된 설정에서는 `zephyr-sdk-1.0.1`) — 안 맞는 일반 버전이 잡히지 않았는지
- [ ] `west boards | findstr sr100`(Windows) / `grep sr100`(macOS/Linux)로 `sr100_rdk` 확인
- [ ] Python 인터프리터가 워크스페이스의 venv로 설정되어 있는지 확인 (GUI/CLI가 같은 걸 써야 함)
- [ ] `zephyr_curriculum/<랩 이름>/lab/` 아래에 프로젝트 구성 (Step 3)
- [ ] `west build -p always -b sr100_rdk/sr100/m55 .\zephyr_curriculum\<랩 이름>\lab\` → `srsdk_tools\openocd_flash.py`로 플래시 → 시리얼 터미널(230400bps 8N1)로 확인 (Step 4)

> `west sdk install`과 최초 소스 다운로드는 시간이 걸릴 수 있습니다. 미리 네트워크를 확인하세요.

## 트러블슈팅 (빌드/실행 단계)

> 환경 설정(Step 1~2) 중 발생하는 문제는 **`ZEPHYR_SETUP_TROUBLESHOOTING.md`** 참고. 아래 표는 워크스페이스가 정상 구성된 이후, 실제 SR110 하드웨어에서 빌드/플래시/실행 단계에 확인된 문제들입니다.

| 증상 | 원인 / 해결 |
|---|---|
| `west boards`에 `sr100_rdk`가 안 뜸 | `syna_zephyr_sdk` 기준으로 워크스페이스가 구성되지 않음 (SR110 지원은 mainline `zephyrproject-rtos/zephyr` 매니페스트에 기본으로 없음) |
| `Board qualifiers 'sr100' for board 'sr100_rdk' not found` | 보드명에 코어를 명시 안 함 — `sr100_rdk/sr100/m55` 전체를 쓸 것 |
| `No prj.conf file(s) was found` | 프로젝트를 손으로 만들면서 `prj.conf`를 빠뜨림 — 비어있어도 파일 자체는 있어야 함 |
| 링크 단계에서 `undefined reference to __device_dts_ord_N` | devicetree 노드의 `status`가 `"okay"`가 아니거나, `compatible` 문자열이 Kconfig의 `depends on DT_HAS_..._ENABLED` 조건과 안 맞음 — 노드는 있지만 디바이스 인스턴스/드라이버가 컴파일 안 된 상태. 실제 확인된 원인: 기본 비활성화된 I2C0, 그리고 `compatible` 문자열에 불필요한 `fb` 접미사가 붙어 Kconfig가 기대하는 문자열과 안 맞았던 디스플레이 드라이버 |
| CMake가 `Ignoring extra path from command line: ".overlay"`를 출력하며 devicetree 전처리 실패 | 이미 파일명으로 자동 인식되는 오버레이를 `-DEXTRA_DTC_OVERLAY_FILE=...`로 중복 지정 — 이 플래그 제거 |
| 링크 단계에서 `undefined reference to k_malloc` | 일부 서브시스템(문자 디스플레이용 CFB 등)이 내부적으로 `k_malloc()`을 호출하는데 힙이 설정 안 됨 — `prj.conf`에 `CONFIG_HEAP_MEM_POOL_SIZE=<바이트 수>` 추가 |
| I2C 스캔/센서 probe에서 아무것도 안 잡히거나 개수가 이상함 | zero-length 또는 1바이트 dummy `i2c_write()` probe가 SR110의 DesignWare I2C 드라이버 포팅에서 신뢰할 수 없는 것으로 확인됨 — 1바이트 `i2c_read()` probe로 교체하면 정상 동작 확인됨 |
| `CONFIG_CONSOLE_GETLINE=y`인데도 `console_getline()`/`console_getline_init()`이 undefined reference로 링크 실패 | 이 SDK의 Zephyr 4.4.1 빌드에서 확인된 문제 — 대신 콘솔 UART 디바이스에 `uart_poll_in()`/`uart_poll_out()`을 직접 사용 (범용 serial 드라이버 API만 필요) |
| `ninja: error: loading 'build.ninja': ... cannot find the file` | 이전 빌드 실패로 `build` 폴더가 불완전 — `west build -p always -b <보드> <경로>`로 새로 구성 |
| `cmake.exe`가 종료코드 `3221226505`(`0xC0000409`)로 크래시 | **한글 경로 문제** — Python/SDK 설치 경로에 한글 계정명이 섞여 있음. `ZEPHYR_SETUP_TROUBLESHOOTING.md`의 한글 경로 이슈 참고 |
| 시리얼 출력이 안 보임 | 터미널이 230400bps 8N1로 설정됐는지 확인 (이 커리큘럼의 이전 ESP32-S3 버전 기본값이던 115200이 아님) |

---

## Zephyr 스케줄링의 핵심 — 우선순위 숫자와 협조적/선점형 구분

**Zephyr에서는 숫자가 작을수록 우선순위가 높습니다.** 그리고 Zephyr는 스레드를 **협조적(negative priority)**과 **선점형(priority 0 이상)** 두 범주로 명확히 나눕니다. 이 개념은 커리큘럼 초반(02번, 04번)에서 집중적으로 다룹니다. 이건 Zephyr 커널의 핵심 동작이라 어느 보드에서 실행하든 동일합니다.

## 공통 사항 (01번부터 적용)

- 대부분의 랩은 `printk()`만으로 확인 가능합니다. 하드웨어 배선이 필요한 랩(07번, 23번)은 명시적으로 표시했습니다
- 코드의 출력 문자열은 영어로 작성되어 있습니다
- 이후 실습들의 빌드 명령은 위 Step 4와 동일하게 `west build -p always -b sr100_rdk/sr100/m55 .\zephyr_curriculum\<랩 이름>\lab\` → `srsdk_tools\openocd_flash.py`로 플래시 → 시리얼 터미널(230400bps 8N1) 순서입니다

## 목차

| # | 파일 | 주제 |
|---|---|---|
| 01 | `01_THREAD_CREATION_LAB.md` | 스레드 생성 기초 (K_THREAD_DEFINE / k_thread_create) |
| 02 | `02_THREAD_PRIORITY_LAB.md` | 우선순위 체계와 협조적/선점형 스레드 |
| 03 | `03_THREAD_LIFECYCLE_LAB.md` | 동적 스레드 생성/종료 |
| 04 | `04_COOPERATIVE_YIELD_LAB.md` | 협조적 스레드와 k_yield — 왜 항상 양보해야 하는가 |
| 05 | `05_PRIORITY_INVERSION_LAB.md` | 우선순위 역전 재현 |
| 06 | `06_IDLE_THREAD_LAB.md` | Idle Thread와 CPU 유휴 시간 |
| 07 | `07_ISR_SEMAPHORE_LAB.md` | 인터럽트(ISR) + k_sem |
| 08 | `08_COUNTING_SEMAPHORE_LAB.md` | Counting Semaphore |
| 09 | `09_MUTEX_LAB.md` | k_mutex vs k_sem, 우선순위 상속 |
| 10 | `10_MSGQ_BASICS_LAB.md` | Message Queue 기초 (k_msgq) |
| 11 | `11_K_POLL_LAB.md` | k_poll — 여러 커널 객체를 동시에 대기 |
| 12 | `12_POLL_SIGNAL_LAB.md` | Poll Signal — 경량 이벤트 |
| 13 | `13_K_EVENT_LAB.md` | k_event — 여러 조건 동시 대기 |
| 14 | `14_K_TIMER_LAB.md` | k_timer (일회성/주기적) |
| 15 | `15_STACK_MONITORING_LAB.md` | 스택 사용량 모니터링 |
| 16 | `16_DEADLOCK_LAB.md` | 데드락 재현과 회피 |
| 17 | `17_CRITICAL_SECTION_LAB.md` | irq_lock / k_sched_lock / k_spinlock |
| 18 | `18_MULTICORE_REALITY_LAB.md` | SR110 멀티코어 — 이종(M55/M4) AMP 아키텍처 |
| 19 | `19_POWER_MANAGEMENT_LAB.md` | Zephyr 전력 관리 (prj.conf) |
| 20 | `20_RUNTIME_STATS_LAB.md` | Thread Runtime Stats — CPU 사용률 API |
| 21 | `21_PRODUCER_CONSUMER_LAB.md` | 생산자-소비자 종합 패턴 |
| 22 | `22_ZEPHYR_SUMMARY_LAB.md` | 커리큘럼 전체 정리 |
| 23 | `23_CUSTOM_DEVICE_DRIVER_LAB.md` | 커스텀 디바이스 드라이버 만들기 (AHT20 예제) |

## 학습 흐름

- **01~06**: 스레드 자체의 특성 — 생성, 라이프사이클, 협조적/선점형 스케줄링
- **07~13**: 동기화/통신 도구 — Semaphore, Mutex, Message Queue, k_poll, k_event
- **14~17**: 타이밍 제어와 자원 보호 — Timer, 스택 모니터링, 데드락, critical section
- **18~20**: SR110 특화 기능 — 멀티코어(이종 AMP), 전력관리, runtime stats
- **21~23**: 응용 종합 — Producer-Consumer, 커리큘럼 마무리, 커스텀 드라이버
