# Zephyr RTOS 커리큘럼 개요

Zephyr RTOS의 API와 설계 철학을 순서대로 익히는 23개 실습 시리즈입니다. 각 실습은 독립된 `.md` 파일로 구성되어 있고, 번호 순서대로 진행하면 개념이 이어지도록 설계했습니다. 스레드 생성부터 동기화·통신 수단, 타이밍 제어, ESP32-S3에서의 멀티코어·전력 관리, 그리고 커스텀 디바이스 드라이버 작성까지 다룹니다.

> ⚠️ **PlatformIO는 ESP32 계열의 Zephyr 프레임워크를 지원하지 않습니다.** `platformio.ini`에 `framework = zephyr`를 지정해도 espressif32 플랫폼에서는 빌드되지 않는다는 점이 PlatformIO 커뮤니티 및 GitHub 이슈에서 확인되었습니다. 따라서 이 커리큘럼은 Zephyr 공식 도구인 **west** 기반으로 진행하며, VS Code는 "Zephyr IDE" 확장을 통해 에디터 겸 빌드/플래시 GUI로 사용합니다.

> 📄 **환경 구축 중 에러가 나면**: 아래 Step 1~2는 "정상적으로 진행됐을 때"의 흐름입니다. 실제로는 중간에 여러 이슈(Host Tools 누락, Python 버전 호환성, 권한 문제 등)를 만날 수 있는데, 이런 개별 에러 대응은 **`ZEPHYR_SETUP_TROUBLESHOOTING.md`**에 별도로 정리해뒀습니다. 이 문서와 같이 보세요.

## 사전 준비물

- PC (Windows / macOS / Linux, WSL2 권장 for Windows)
- Python 3, Git 설치되어 있을 것
- ESP32-S3 보드 (데이터 전송 지원 USB 케이블)
- 디스크 여유 공간 5GB 이상 (Zephyr SDK + 모듈 소스 다운로드용)
- 인터넷 연결 (최초 설치 시 다운로드 용량이 큼 — 네트워크 상태 확인 권장)

> 🚨 **Windows 사용 중이고 계정명(사용자 폴더명, `C:\Users\???`)이 한글이면 반드시 먼저 읽어주세요.** Python과 Zephyr SDK 툴체인은 기본적으로 사용자 폴더 밑(`C:\Users\<계정명>\...`)에 설치되는데, 이 경로에 한글이 섞이면 이후 `cmake.exe`가 크래시(`STATUS_STACK_BUFFER_OVERRUN`, 종료코드 `3221226505`)하는 문제가 실제로 확인됐습니다. **계정명 자체는 바꿀 필요 없고, Python 설치 위치와 SDK 설치 위치만 한글 없는 경로로 지정하면 됩니다** — 아래 Step 2의 방법 B에 이걸 처음부터 반영한 순서가 있습니다.

---

## Step 1. VS Code에 Zephyr IDE 확장 설치

1. VS Code Extensions(`Ctrl+Shift+X`)에서 `Zephyr IDE` 검색
2. 제작사 **mylonics**의 **Zephyr IDE** 확장 설치
3. 설치 후 VS Code 재시작

---

## Step 2. west 환경 & Zephyr SDK 설치

### 방법 A — Zephyr IDE 확장 사용 (GUI 기반)

> ⚠️ 실제로 진행해보면 아래처럼 중간에 수동으로 처리해야 하는 단계가 여러 번 나옵니다. "확장 하나로 클릭 몇 번에 끝난다"는 기대보다는, 아래 단계들을 하나씩 확인하며 진행하시는 걸 권장합니다. 막히는 지점은 대부분 `ZEPHYR_SETUP_TROUBLESHOOTING.md`에 정리되어 있습니다.

1. **먼저 빈 폴더를 하나 열어야 합니다.** Zephyr IDE 확장의 모든 명령은 "현재 VS Code에 열려있는 폴더"를 기준으로 동작합니다 — 폴더를 안 연 상태로 명령을 실행하면 `No workspace folder open. Please open a folder first.` 에러가 납니다.
   - `File → Open Folder`로 새로 만든 **빈 폴더**를 엽니다
   - 폴더 경로에 **한글이나 공백이 없는** 위치를 권장합니다 (예: `C:\zephyrproject`) — 이후 west/CMake 빌드 과정에서 경로에 한글/공백이 있으면 문제가 생기는 경우가 있습니다

2. 좌측 Activity Bar에서 **Zephyr IDE 아이콘** 클릭 (정식 확장명은 "**IDE for Zephyr**"이고, 설치 직후 아이콘이 안 보이면 VS Code 재시작 필요)

3. Command Palette(`Ctrl+Shift+P`)에서 **`Zephyr IDE: Setup Standard Workspace`** 실행
   - ⚠️ 비슷한 이름의 명령이 여러 개 있습니다(`Setup West Environment`, `Setup Workspace from Git`, `Setup Workspace from Current/External Directory`, `Re-run West Setup`, `Skip West Setup` 등). 처음 시작하는 경우 반드시 **Setup Standard Workspace**를 선택하세요

4. **Host Tools 체크 화면**이 뜹니다 — Python3, Git, CMake, Ninja, DTC(Devicetree Compiler), gperf, wget 등 필수 도구 목록이 각각 Installed / Not Available로 표시됩니다
   - **Not Available로 표시된 항목은 전부 설치 후 진행해야 합니다** — 설치 없이 넘어가면 이후 단계에서 에러 메시지 없이 조용히 멈추는 경우가 있습니다
   - Windows 기준 `winget`으로 한 번에 설치 (PowerShell을 **관리자 권한**으로 실행 후):
     ```powershell
     winget install Kitware.CMake Ninja-build.Ninja oss-winget.dtc oss-winget.gperf wget 7zip.7zip
     ```
   - 설치 후 **VS Code를 완전히 종료했다가 다시 열어야** PATH 변경이 반영됩니다 (창 새로고침만으로는 반영 안 됨)
   - 재시작 후 Host Tools 화면으로 돌아와 전부 Installed인지 재확인

5. Host Tools가 전부 통과되면 west workspace 생성 → Zephyr 소스 다운로드 → SDK 설치가 자동으로 이어집니다 (수 분~수십 분, 네트워크 속도에 따라 다름)

6. ⚠️ **중요 — 자동으로 잡힌 SDK/보드를 반드시 확인하세요.** 자동 설정 과정에서 의도와 다른 기본 SDK가 잡히는 경우가 확인됐습니다 (예: 공식 `zephyr-sdk-x.y.z`가 아니라 `SRSDK`라는 이름으로, `sr110_cm55`처럼 ESP32-S3와 무관한 Cortex-M 계열 보드용 예제 SDK가 받아지는 사례). 이 상태로는 이후 `west boards`에 esp32s3 계열 보드가 아예 안 나옵니다.
   - Output 패널(채널: "IDE for Zephyr")에서 다운로드되는 SDK 이름이 `zephyr-sdk-`로 시작하는 공식 이름인지 확인하세요
   - 이상한 이름이 보이면 **자동 흐름을 더 진행하지 말고 방법 B(수동 CLI)로 처음부터 다시 진행하는 걸 권장합니다** — 이 문제를 실제로 겪었을 때는 GUI 자동 흐름보다 CLI가 훨씬 예측 가능하고 확실했습니다

7. **Python 인터프리터 연결**
   - west/pip 관련 스크립트가 쓸 Python을 VS Code에 알려줘야 합니다
   - Command Palette → **`Python: Select Interpreter`** → 확장이 생성한 가상환경(`.venv`, 보통 workspace 폴더 안) 선택. 목록에 없으면 "Enter interpreter path..." → "Find..."로 `.venv\Scripts\python.exe` 직접 지정
   - Zephyr IDE 확장 자체에도 별도 Python 경로 설정이 있을 수 있습니다 — `Ctrl+,`(Settings) → Workspace 탭 → `zephyr-ide` 검색 → Python 경로 항목이 있으면 동일하게 지정

8. 여기까지 정상적으로 끝나면 **Step 3(새 프로젝트 생성)**으로 진행합니다

> 실제로 해보면 **4번(Host Tools)**과 **6번(잘못된 SDK)** 두 곳에서 가장 자주 막힙니다. 계속 막히면 무리하게 GUI로 재시도하기보다, 아래 **방법 B**로 west workspace를 CLI로 먼저 확실히 만들어둔 뒤 `Zephyr IDE: Setup Workspace from Current Directory` 명령으로 그 workspace를 확장에 인식시키는 방법이 더 안정적이었습니다.

### 방법 B — 수동 설치 (터미널 CLI, 문제 발생 시 더 안정적 · 권장)

> 💡 여러 차례의 실전 트러블슈팅 끝에 확정된 순서입니다. Windows + 한글 계정명 조합에서도 문제없이 끝까지 갔던 방법이라, **처음부터 이 방법을 권장합니다.**

**핵심 원칙**: Python과 Zephyr SDK 툴체인 설치 위치를 **한글 없는 경로로 직접 지정**합니다 (계정명이 한글이어도 무관).

#### B-1. Python을 한글 없는 경로에 설치 (Windows, 계정명이 한글인 경우 필수)

`winget`으로 설치하면 기본 위치(사용자 폴더 밑)에 깔려서 한글 경로 문제가 재발할 수 있습니다. [python.org](https://www.python.org/downloads/)에서 **Python 3.12**(3.13 이상은 `hidapi` 등 일부 패키지의 사전빌드 wheel이 아직 없어 설치가 실패하는 경우가 확인됨) 설치 파일을 받아 직접 설치합니다.

1. 설치 파일 실행 → **"Customize installation"** 선택 (Install Now 클릭 금지 — 경로 지정 불가)
2. Optional Features → 기본값 그대로 Next
3. Advanced Options → **"Customize install location"**에 한글 없는 경로 입력 (예: `D:\winapp\python\Python312`)
4. Install

확인:
```powershell
D:\winapp\python\Python312\python.exe --version
```

(계정명이 영문이라면 이 단계는 생략하고 `winget install Python.Python.3.12`로 편하게 설치해도 됩니다)

#### B-2. workspace 폴더 생성 + 전용 가상환경(venv)

```powershell
mkdir D:\work\zephyrproject
cd D:\work\zephyrproject
D:\winapp\python\Python312\python.exe -m venv .venv
.venv\Scripts\Activate.ps1        # macOS/Linux: source .venv/bin/activate
```

(프롬프트 앞에 `(.venv)`가 붙으면 활성화된 것 — 이후 모든 명령은 이 상태에서 실행)

#### B-3. west 설치

```powershell
python -m pip install west
```

#### B-4. west workspace 초기화 + Zephyr 소스/HAL 모듈 다운로드

```powershell
python -m west init -m https://github.com/zephyrproject-rtos/zephyr --mr main .
python -m west update              # 수백MB~1GB, 네트워크에 따라 오래 걸림
```

#### B-5. Zephyr 스크립트 의존 패키지 설치

```powershell
# zephyr/scripts 폴더 "안에서" 실행해야 함
# (requirements.txt가 같은 폴더의 다른 requirements-*.txt를 상대경로로 참조하기 때문)
cd zephyr\scripts
python -m pip install -r requirements.txt
cd ..\..
```

#### B-6. Zephyr SDK 설치 (한글 없는 경로로)

```powershell
python -m west sdk install --install-base D:\work\zephyr_toolchains
```

> ⚠️ 이전에 `--install-base` 없이 `west sdk install`을 한 번이라도 실행한 적이 있다면, west가 그 기본 위치(`C:\Users\<계정명>\zephyr-sdk-x.y.z`, 한글 계정이면 그 경로 그대로)에 이미 설치된 걸 발견하고 재사용해버려서 `--install-base`가 무시된 것처럼 보일 수 있습니다. 이 경우 기존 설치를 지우고(`Remove-Item -Recurse -Force`) CMake 패키지 레지스트리 등록도 지운 뒤(`Remove-Item -Path "HKCU:\Software\Kitware\CMake\Packages\Zephyr-sdk" -Recurse -Force`) 다시 시도하세요. 자세한 건 `ZEPHYR_SETUP_TROUBLESHOOTING.md` 참고.

#### B-7. 플래싱 도구(esptool 등) 설치

```powershell
python -m west packages pip --install
```

#### 설치 확인

```powershell
python -m west --version
python -m west boards | findstr esp32s3    # macOS/Linux: grep esp32s3
```

`esp32s3_devkitc` 등 ESP32-S3 관련 보드 타겟이 출력되면 정상입니다.

> 💡 `west zephyr-export`는 이 순서에 없습니다 — west workspace 안에서 `west build`로 빌드하는 일반적인 경우, west가 `ZEPHYR_BASE`를 자동으로 CMake에 넘겨주기 때문에 **불필요합니다** (Zephyr 공식 GitHub 토론에서도 확인된 내용). Workspace 바깥에 별도로 만든 "freestanding 앱"을 CMake만으로 직접 빌드하려는 특수한 경우에만 필요합니다.

> ⚠️ `west init`/`west update` 도중 `PermissionError: Access is denied`가 나거나, `requirements.txt` 설치 중 `hidapi` 빌드가 실패하거나, `west sdk install`에서 `7z`를 못 찾는다는 에러가 나면 — 전부 실제로 확인된 이슈들입니다. **`ZEPHYR_SETUP_TROUBLESHOOTING.md`에 원인과 해결 순서를 정리해뒀으니 그쪽을 참고하세요.**

### VS Code와 west workspace 연결하기 (방법 B로 CLI 설치한 경우)

방법 B로 CLI를 통해 workspace를 직접 만들었다면, 이후 VS Code의 Zephyr IDE 확장이 이 workspace를 인식하도록 연결해야 합니다.

1. VS Code에서 방금 만든 workspace 폴더(`zephyrproject`)를 엽니다
2. Command Palette → **`Zephyr IDE: Setup Workspace from Current Directory`** 실행
3. GUI와 CLI가 같은 Python/SDK를 보도록, Settings(`Ctrl+,`)에서 아래 두 값을 CLI에서 쓴 경로와 동일하게 지정:
   ```json
   {
     "zephyr-ide.venvFolder": "D:\\work\\zephyrproject\\.venv",
     "zephyr-ide.toolchainDirectory": "D:\\work\\zephyr_toolchains"
   }
   ```
   이렇게 맞춰두지 않으면, GUI가 CLI와 다른 venv/SDK를 참조하면서 "PowerShell에서는 되는데 GUI에서는 안 된다" 같은 혼란이 생길 수 있습니다.

---

## Step 3. 새 프로젝트 생성

> 💡 **직접 파일을 만들기보다, 이미 완성되어 있는 공식 샘플을 그대로 쓰는 걸 권장합니다.** `prj.conf`를 빠뜨리는 등의 실수가 흔해서, west workspace 안에 이미 받아져 있는 `zephyr/samples/hello_world`를 그대로 활용하는 게 훨씬 안전합니다.

### 권장 — 기존 샘플 활용 (CLI)

```powershell
cd zephyr\samples\hello_world
```

이 폴더 안에 `CMakeLists.txt`, `prj.conf`, `src/main.c`가 이미 전부 갖춰져 있습니다. 바로 Step 4의 빌드 명령으로 넘어가면 됩니다.

### Zephyr IDE 확장 사용 시 (GUI)

1. Zephyr IDE 패널 → **Create Project** 클릭
2. **Select Sample Project** 검색창이 나오면, 비어있는 목록만 보고 없다고 판단하지 말고 **`hello`라고 직접 입력**해서 필터링 — `hello_world`는 Zephyr 핵심 샘플이라 항상 workspace에 포함되어 있습니다
3. 그래도 안 뜨면 **`Zephyr IDE: Add Project`** 명령으로 아래 경로를 직접 지정:
   ```
   <workspace 폴더>\zephyr\samples\hello_world
   ```

### 직접 새 프로젝트를 만들고 싶다면 (응용, 처음엔 비권장)

익숙해진 뒤에 직접 구조를 만들고 싶다면 아래 구조를 **빠짐없이** 갖춰야 합니다 — 특히 `prj.conf`는 내용이 비어있어도 되지만 파일 자체가 없으면 빌드가 안 됩니다.

```
my_app/
├── CMakeLists.txt
├── prj.conf              ← 커널 설정 (Kconfig) — 이 커리큘럼의 CONFIG_EVENTS, CONFIG_PM 등이 여기 들어감. 파일 자체가 없으면 "No prj.conf file(s) was found" 에러
├── boards/
│   └── esp32s3_devkitc.overlay   ← 보드별 하드웨어 설정 (07번 실습 등에서 사용, Hello World 단계엔 불필요)
└── src/
    └── main.c
```

`CMakeLists.txt` 최소 예시:
```cmake
cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(my_app)
target_sources(app PRIVATE src/main.c)
```

> ⚠️ 사용 중인 보드가 16MB Flash / 8MB PSRAM(N16R8)처럼 특정 메모리 구성이라면, `west build` 명령에 `-S flash-16M -S psram-8M` 같은 snippet 플래그를 추가하거나 overlay 파일에 반영해야 합니다. 보드 데이터시트를 먼저 확인하세요.

---

## Step 4. Hello World 작성 & 실행

기존 샘플(`zephyr/samples/hello_world`)을 그대로 쓰신다면 코드 작성은 필요 없습니다 — 아래는 참고용 내용입니다.

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

### 빌드 · 플래시 · 모니터

```bash
west build -b esp32s3_devkitc/esp32s3/procpu
west flash
west espressif monitor
```

> ⚠️ **보드 이름은 반드시 `esp32s3_devkitc/esp32s3/procpu`까지 전부 써야 합니다.** `esp32s3_devkitc`만 쓰면 아래 에러가 납니다 — ESP32-S3는 Zephyr에서 코어마다 별도 이미지를 빌드하는 **AMP 구조**(`18_MULTICORE_REALITY_LAB.md` 참고)라 어느 코어를 타겟할지(`procpu` 또는 `appcpu`)까지 명시해야 합니다. Hello World처럼 시리얼 콘솔이 필요한 일반적인 경우는 항상 **`procpu`**를 씁니다.
> ```
> Board qualifiers `esp32s3` for board `esp32s3_devkitc` not found.
> Valid board targets for esp32s3_devkitc are:
>   esp32s3_devkitc/esp32s3/procpu
>   esp32s3_devkitc/esp32s3/appcpu
> ```

Zephyr IDE 확장을 사용 중이라면 GUI의 **Build** / **Flash** / **Monitor** 버튼으로 동일하게 실행할 수 있습니다.

### 실행 & 확인

- 시리얼 모니터에 `Hello, ESP32-S3! (Zephyr)`가 1초 간격으로 출력되는지 확인
- 안 뜨면 보드의 RESET 버튼을 눌러보고, 그래도 안 되면 USB 케이블/포트를 확인

### 참고 — `build` 폴더 관련 에러가 나면

이전에 실패한 빌드 시도로 `build` 폴더가 불완전하게 남아있으면 `ninja: error: loading 'build.ninja': The system cannot find the file specified.` 같은 에러가 날 수 있습니다. 이럴 땐 처음부터 다시 설정(pristine build)하면 됩니다.

```bash
west build -p always -b esp32s3_devkitc/esp32s3/procpu
```

또는 `build` 폴더를 직접 지우고(`Remove-Item -Recurse -Force build`) 재시도해도 동일합니다.

## 환경구축 체크리스트

- [ ] (한글 Windows 계정명이라면) Python을 한글 없는 경로에 설치
- [ ] VS Code에서 빈 폴더(한글/공백 없는 경로) Open Folder
- [ ] Zephyr IDE 확장 설치 및 VS Code 재시작
- [ ] Host Tools 체크 화면에서 모든 항목 Installed 확인 (Not Available 있으면 winget으로 설치 후 VS Code 재시작)
- [ ] west workspace 구축 완료 — SDK가 한글 없는 경로에 설치됐는지 확인 (`--install-base` 또는 `zephyr-ide.toolchainDirectory`)
- [ ] `west boards | findstr esp32s3`(Windows) 또는 `grep esp32s3`(macOS/Linux)로 보드 타겟 이름 확인
- [ ] Python 인터프리터가 workspace의 `.venv`로 지정되어 있는지 확인 (GUI/CLI 동일한 venv 사용)
- [ ] `zephyr/samples/hello_world` 등 기존 샘플로 프로젝트 준비 (Step 3)
- [ ] `west build -b esp32s3_devkitc/esp32s3/procpu` → `west flash` → 모니터 확인 (Step 4, **보드 qualifier `/esp32s3/procpu` 필수**)

> `west sdk install`과 최초 `west update`는 다운로드 용량이 커서 시간이 걸릴 수 있습니다. 네트워크 상태를 미리 확인하세요.

## 트러블슈팅 (빌드/실행 단계)

> 환경 구축(Step 1~2) 중 발생하는 이슈는 **`ZEPHYR_SETUP_TROUBLESHOOTING.md`**를 참고하세요. 아래는 workspace가 정상 구축된 이후, 빌드/플래시/실행 단계에서 발생하는 이슈입니다.

| 증상 | 원인 / 해결 |
|---|---|
| `west boards`에 esp32s3 관련 보드가 없음 | `west update`가 완료되지 않았거나 Espressif HAL 모듈이 누락 — `west update` 재실행 |
| `Board qualifiers 'esp32s3' for board 'esp32s3_devkitc' not found` | 보드 이름에 코어 지정이 빠짐 — `esp32s3_devkitc/esp32s3/procpu`까지 전부 명시 |
| `No prj.conf file(s) was found` | 프로젝트를 직접 만들다 `prj.conf` 파일을 빠뜨림 — 빈 파일이라도 존재해야 함, 또는 기존 샘플(`hello_world`) 활용 권장 |
| `ninja: error: loading 'build.ninja': ... cannot find the file` | 이전 실패한 빌드로 `build` 폴더가 불완전 — `west build -p always -b <보드>`로 pristine 재빌드 |
| `esptool>=5.0.2 not found in PATH` | 플래싱 도구 미설치 — `west packages pip --install` 실행 |
| 빌드 시 Espressif HAL 관련 blob 에러 | Espressif HAL은 RF 관련 바이너리 blob이 별도로 필요 — `west blobs fetch hal_espressif` 실행 |
| west build 실패 (보드를 찾을 수 없음) | 정확한 보드 타겟 이름을 `west boards \| grep esp32s3`로 재확인 (Zephyr 버전에 따라 표기 형식이 다를 수 있음) |
| 새 터미널에서 `west: command not found` | venv가 활성화되지 않음 — `.venv\Scripts\Activate.ps1`(또는 `source .venv/bin/activate`) 재실행 |
| `cmake.exe`가 종료코드 `3221226505`(`0xC0000409`)로 크래시 | **한글 경로 문제** — Python/SDK 설치 경로에 한글 계정명이 섞여 있음. `ZEPHYR_SETUP_TROUBLESHOOTING.md`의 한글 경로 이슈 참고 |
| 시리얼 출력이 안 보임 | `west espressif monitor` 대신 `screen`/`minicom`으로 baud rate 115200 확인, RESET 버튼으로 재시작 |

---

## Zephyr 스케줄링의 핵심 — 우선순위 숫자와 협조적/선점형 구분

**Zephyr는 숫자가 작을수록 우선순위가 높습니다.** 그리고 스레드를 **협조적(cooperative, 우선순위가 음수)**과 **선점형(preemptible, 우선순위가 0 이상)** 두 종류로 명확히 구분합니다. 커리큘럼 초반부(02, 04번)에서 이 개념을 집중적으로 다룹니다.

## 공통 사항 (01번부터 적용)

- 대부분 `printk()`만으로 확인 가능하고, 하드웨어 배선이 필요한 실습(07번)은 별도 명시
- 코드의 출력 문자열은 영어로 작성되어 있습니다
- 이후 실습들의 빌드 명령은 위 Step 4와 동일하게 `west build -b esp32s3_devkitc/esp32s3/procpu` → `west flash` → `west espressif monitor` 순서입니다

## 목차

| 번호 | 파일 | 주제 |
|---|---|---|
| 01 | `01_THREAD_CREATION_LAB.md` | Thread 생성 기초 (K_THREAD_DEFINE / k_thread_create) |
| 02 | `02_THREAD_PRIORITY_LAB.md` | 우선순위 체계와 협조적/선점형 스레드 |
| 03 | `03_THREAD_LIFECYCLE_LAB.md` | Thread 동적 생성/종료 |
| 04 | `04_COOPERATIVE_YIELD_LAB.md` | 협조적 스레드와 k_yield — 반드시 양보해야 하는 이유 |
| 05 | `05_PRIORITY_INVERSION_LAB.md` | 우선순위 역전 재현 |
| 06 | `06_IDLE_THREAD_LAB.md` | Idle Thread와 CPU 유휴 시간 |
| 07 | `07_ISR_SEMAPHORE_LAB.md` | 인터럽트(ISR) + k_sem |
| 08 | `08_COUNTING_SEMAPHORE_LAB.md` | Counting Semaphore |
| 09 | `09_MUTEX_LAB.md` | k_mutex vs k_sem, Priority Inheritance |
| 10 | `10_MSGQ_BASICS_LAB.md` | Message Queue 기본 (k_msgq) |
| 11 | `11_K_POLL_LAB.md` | k_poll — 여러 커널 객체 동시 대기 |
| 12 | `12_POLL_SIGNAL_LAB.md` | Poll Signal — 경량 이벤트 |
| 13 | `13_K_EVENT_LAB.md` | k_event — 다중 조건 대기 |
| 14 | `14_K_TIMER_LAB.md` | k_timer (One-shot / Periodic) |
| 15 | `15_STACK_MONITORING_LAB.md` | 스택 사용량 모니터링 |
| 16 | `16_DEADLOCK_LAB.md` | Deadlock 재현과 회피 |
| 17 | `17_CRITICAL_SECTION_LAB.md` | irq_lock / k_sched_lock / k_spinlock |
| 18 | `18_MULTICORE_REALITY_LAB.md` | ESP32-S3에서의 멀티코어 — AMP 아키텍처 |
| 19 | `19_POWER_MANAGEMENT_LAB.md` | Zephyr Power Management (prj.conf) |
| 20 | `20_RUNTIME_STATS_LAB.md` | Thread Runtime Stats — CPU 사용률 API |
| 21 | `21_PRODUCER_CONSUMER_LAB.md` | Producer-Consumer 종합 패턴 |
| 22 | `22_ZEPHYR_SUMMARY_LAB.md` | 전체 커리큘럼 정리 |
| 23 | `23_CUSTOM_DEVICE_DRIVER_LAB.md` | 커스텀 디바이스 드라이버 만들기 (AHT20 예제) |

## 학습 흐름

- **01~06**: Thread 자체의 특성 — 생성, 생명주기, 협조적/선점형 스케줄링
- **07~13**: 동기화/통신 수단 — Semaphore, Mutex, Message Queue, k_poll, k_event
- **14~17**: 타이밍 제어와 자원 보호 — Timer, 스택 모니터링, Deadlock, Critical Section
- **18~20**: ESP32-S3 특화 기능 — 멀티코어(AMP), 전력 관리, 런타임 통계
- **21~23**: 종합 응용 — Producer-Consumer, 커리큘럼 정리, 커스텀 드라이버
