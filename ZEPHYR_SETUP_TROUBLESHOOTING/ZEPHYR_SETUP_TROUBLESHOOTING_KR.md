# Zephyr 환경 구축 트러블슈팅

`00_ZEPHYR_CURRICULUM_LAB.md`의 Step 1~2(west 환경 & SDK 설치) 과정에서 실제로 확인된 이슈들을 겪은 순서대로 정리했습니다. Windows 기준으로 작성했지만, 원인 설명은 macOS/Linux에도 대부분 적용됩니다.

> 💡 **자주 묻는 질문 — `west zephyr-export`를 꼭 실행해야 하나요?** 아니요. west workspace 안에서 `west build`로 빌드하는 일반적인 경우엔 필요 없습니다. west가 빌드 시점에 `ZEPHYR_BASE`를 자동으로 CMake에 넘겨주기 때문입니다 (Zephyr 공식 GitHub 토론 #61039에서 확인). Workspace 밖에 별도로 만든 "freestanding 앱"을 CMake만으로 직접 빌드할 때만 필요합니다.

---

## 1. Command Palette에 "Zephyr IDE: Setup Zephyr IDE"가 없음

### 증상

Command Palette에 정확히 이 이름의 명령이 없고, 비슷한 이름이 여러 개 뜸: `Setup Standard Workspace`, `Setup West Environment`, `Setup Workspace from Git`, `Re-run West Setup`, `Skip West Setup` 등

### 원인

확장 버전이 올라가면서 명령어 체계가 더 세분화됨.

### 해결

새로 시작하는 경우 **`Zephyr IDE: Setup Standard Workspace`**를 실행하면 됩니다. 각 명령의 역할:

| 명령어 | 역할 |
|---|---|
| **Setup Standard Workspace** | 새 workspace를 처음부터 생성 (west 환경 설정 + SDK 다운로드까지 한 번에) — **처음 시작할 때 이것** |
| Setup West Environment | west 환경만 별도로 설정 (workspace는 이미 있다는 전제) |
| Setup Workspace from Current Directory | CLI로 이미 만들어둔 workspace를 현재 열린 폴더에서 인식 |
| Setup Workspace from Git | 이미 존재하는 프로젝트를 git에서 clone해서 workspace로 사용 |
| Re-run West Setup | 기존 workspace에서 west 설정을 다시 실행 (문제 생겼을 때 재시도용) |
| Skip West Setup | west 환경 구축 단계를 건너뛰기 (이미 수동 설치했을 때) |

---

## 2. `No workspace folder open. Please open a folder first.`

### 증상

Setup 명령 실행 시 알림창에 이 에러가 뜸

### 원인

Zephyr IDE 확장의 모든 명령은 **VS Code에 폴더가 열려있는 상태**를 전제로 동작합니다. 폴더를 안 열고(시작 화면 상태로) 명령을 실행하면 발생합니다.

### 해결

1. 빈 폴더를 하나 만듭니다 (경로에 **한글/공백 없이** — 예: `C:\zephyrproject`)
2. `File → Open Folder`로 그 폴더를 엽니다
3. 폴더가 열린 상태에서 Setup 명령 재실행

---

## 3. Host Tools 체크 화면에서 일부 항목이 "Not Available"

### 증상

Python3, Git, DTC는 Installed인데 `gperf`, `wget`, `cmake`, `ninja` 등 일부가 Not Available로 표시됨

### 원인

Zephyr 빌드에 필요한 여러 호스트 도구가 시스템에 없거나 PATH에 안 잡혀 있음. 확장이 자동으로 설치해주지 않고, 사용자가 직접 설치해야 함.

### 해결 (Windows, `winget`)

```powershell
winget install Kitware.CMake Ninja-build.Ninja oss-winget.dtc oss-winget.gperf wget 7zip.7zip
```

설치 후 **VS Code를 완전히 종료했다가 다시 열어야** PATH 변경이 반영됩니다. 재시작 후 Host Tools 화면에서 재확인하세요.

---

## 4. Setup 과정에서 `sr110_cm55`, `SRSDK` 같은 이름이 나옴 — SR110에서는 이게 정상입니다

### 증상

- Problems 패널에 `c_cpp_properties.json`이 `sr110_cm55_fw` 같은 경로를 참조한다는 경고가 뜸
- 알림에 `SRSDK_DIR set to: ...\srsdk\srsdk-main\srsdk`, `No GCC_TOOLCHAIN_* key found in settings.json` 등이 뜸
- `west boards`를 실행하면 `sr100_rdk`가 뜨는지, 아니면 전혀 다른(예: 다른 벤더용) 보드 목록이 뜨는지 헷갈림

### 확인해야 할 것

이 커리큘럼 초기 버전(ESP32-S3 기준)에서는 이 상황을 "잘못된 보드/SDK가 설정된 경고 신호"로 다뤘습니다 — Xtensa 기반 ESP32-S3 작업 중에 Cortex-M55용 `sr110_cm55`나 `SRSDK`가 튀어나오는 건 확실히 잘못된 상황이었기 때문입니다.

**SR110으로 작업하는 지금은 정반대입니다.** `sr110_cm55`(또는 `sr100_rdk/sr100/m55`), `SRSDK` 관련 이름이 보이는 건 **오히려 정상**이며, 지금 바로 이 커리큘럼이 다루는 보드/SDK 생태계입니다. 대신 확인해야 할 건 다음입니다.

1. `west boards | findstr sr100`을 실행했을 때 `sr100_rdk`가 실제로 나오는지
2. 툴체인이 실제 확인된 `zephyr-sdk-1.0.1`(또는 그에 상응하는 SR110용 빌드)인지 — 전혀 무관한 보드 생태계(예: 다른 벤더의 예시 템플릿)가 잘못 잡힌 것인지는 `west build` 로그의 `Found toolchain:` 줄과 `Board:` 줄을 보고 실제 SR110/Cortex-M55 관련 내용인지로 판단하세요
3. 반대로 **ESP32 관련 이름**(`esp32s3_devkitc`, `esptool` 등)이 이 워크스페이스에서 나온다면, 그건 이 워크스페이스가 `syna_zephyr_sdk`가 아니라 mainline Zephyr 매니페스트로 잘못 구성된 것일 수 있습니다 — 그 경우에만 워크스페이스를 다시 만드세요

### 정리

"낯선 이름이 나오면 무조건 잘못된 것"이 아니라, **지금 작업 대상이 어떤 보드/SDK인지에 따라 정상/비정상 판단 기준 자체가 달라진다**는 게 이 항목의 핵심입니다.

---

## 5. `pip: The term 'pip' is not recognized...`

### 증상

Python은 인식되는데(`python --version` 정상) `pip` 명령만 인식 안 됨

### 원인

Windows에서 Python 본체는 PATH에 있지만, pip 실행파일이 있는 `Scripts` 폴더는 PATH에 없는 경우가 흔함.

### 해결

`pip` 대신 `python -m pip`로 실행:

```powershell
python -m pip install west
```

이후 `west` 명령도 인식이 안 되면 마찬가지로 `python -m west`로 대체합니다. 근본적으로 해결하려면:

```powershell
python -c "import sysconfig; print(sysconfig.get_path('scripts'))"
```

로 나온 경로를 시스템 환경변수 PATH에 직접 추가하세요.

---

## 6. `west init` 도중 `PermissionError: [WinError 5] Access is denied`

### 증상

```
PermissionError: [WinError 5] Access is denied: '...\.west\manifest-tmp\.git\objects\pack\...'
```

### 원인

Windows Defender(또는 다른 백신)가 방금 생성된 git 임시 파일을 실시간 검사하며 순간적으로 잠그는 경우가 대부분입니다. OneDrive 동기화 폴더 안에서 작업할 때도 비슷한 문제가 생길 수 있습니다.

### 해결 (아래 순서대로 시도)

1. **재시도만으로 해결되는 경우도 있음**:
   ```powershell
   Remove-Item -Recurse -Force .west -ErrorAction SilentlyContinue
   # TODO/VERIFY: syna_zephyr_sdk의 정확한 west init 매니페스트 명령 (00_ZEPHYR_CURRICULUM_LAB.md B-4 참고)
   ```
2. **PowerShell을 관리자 권한으로 실행**해서 재시도 (실제로 이 방법으로 해결된 사례 확인됨)
3. **Windows Defender 제외 폴더 추가**: Windows 보안 → 바이러스 및 위협 방지 → 설정 관리 → 제외 추가 → 작업 폴더(예: `C:\02.work`) 추가
4. **작업 폴더가 OneDrive 동기화 경로 밖에 있는지 확인** — 동기화 폴더 안이라면 밖으로(예: `C:\dev\...`) 옮겨서 재시도

---

## 7. `west boards` 실행 시 `ModuleNotFoundError: No module named 'jsonschema'`

### 증상

west 자체는 동작하지만 `west boards`처럼 Zephyr 저장소 안에 정의된 확장 명령을 실행하면 이 에러가 남

### 원인

`zephyr/scripts/requirements.txt`에 명시된 Python 패키지들이 설치되어 있지 않음 (west workspace 구축과 별개의 단계입니다 — `west update`가 소스만 받아오고, 그 소스가 필요로 하는 Python 패키지는 별도로 설치해야 합니다).

### 해결

```powershell
cd zephyr\scripts
python -m pip install -r requirements.txt
cd ..\..
```

> ⚠️ 이 설치가 또 실패한다면 8번(`hidapi` 빌드 실패) 항목을 확인하세요 — 흔히 같이 발생합니다.

---

## 8. `requirements.txt` 설치 중 `hidapi` 빌드 실패로 전체 설치가 롤백됨

### 증상

```
LINK : fatal error LNK1104: cannot open file '...hid.cp314-win_amd64.exp'
error: command '...\link.exe' failed with exit code 1104
ERROR: Failed building wheel for hidapi
```

이후 `jsonschema` 등 나머지 패키지도 하나도 설치되지 않은 상태로 남음 (7번 문제가 재발함)

### 원인

- pip는 `requirements.txt`의 패키지들을 **전부 성공해야 하나라도 반영되는 원자적(all-or-nothing) 방식**으로 설치합니다 — `hidapi` 하나만 실패해도 `jsonschema`를 포함한 나머지 전체 설치가 롤백됩니다
- `hidapi`가 실패하는 근본 원인은 보통 둘 중 하나입니다:
  1. 너무 최신 Python 버전(예: 3.14)이라 `hidapi`의 사전빌드 wheel이 아직 없어 소스에서 직접 컴파일을 시도
  2. 컴파일에 쓰이는 Visual Studio 링커가 **프리뷰/Insiders 버전**이라 `.exp` 파일 생성 등에서 실패

### 해결 — 근본적 해결 (권장)

**Python 3.11 또는 3.12를 이 workspace 전용으로 별도 설치**해서 진행합니다. 이 버전들은 `hidapi`를 포함해 대부분의 패키지에 사전빌드 wheel이 존재해서, 애초에 로컬 컴파일 자체가 필요 없습니다.

```powershell
winget install Python.Python.3.12
cd <workspace 폴더>
py -3.12 -m venv .venv
.venv\Scripts\Activate.ps1
python -m pip install west
python -m pip install -r zephyr\scripts\requirements.txt
```

이후 이 workspace에서는 항상 `.venv`를 활성화한 상태로 작업합니다 (VS Code Python 인터프리터도 이걸로 지정 — `00_ZEPHYR_CURRICULUM_LAB.md`의 Python 인터프리터 연결 단계 참고).

### 해결 — 임시 우회 (비권장, 계속 비슷한 문제가 재발할 수 있음)

`requirements.txt`에서 `hidapi` 줄만 제외하고 설치를 시도할 수 있지만, `hidapi`가 **최상위 파일이 아니라 그게 참조하는 하위 requirements 파일(`requirements-*.txt`) 안에도 따로 포함**되어 있어서 한 번의 필터링으로 안 끝나는 경우가 확인됐습니다. 또한 필터링한 파일을 workspace 루트에 두면, 원본 `requirements.txt`가 같은 폴더의 다른 파일들을 상대경로로 참조하는 구조 때문에 `Could not open requirements file: requirements-base.txt` 같은 새 에러가 날 수 있습니다(반드시 `zephyr\scripts` 폴더 **안에서** 필터링/설치해야 함). 이런 이유로 위의 "근본적 해결"(Python 버전 교체) 쪽을 권장합니다.

SR110은 `srsdk_tools`의 `openocd_flash.py`로 CMSIS-DAP/OpenOCD 기반 플래싱을 쓰기 때문에, `hidapi`(주로 USB-HID 방식 디버그 프로브용) 자체가 당장의 빌드/플래시에는 필요 없는 경우가 대부분입니다.

---

## 9. `west sdk install` 실행 시 `Please install '7z' and run this script again`

### 증상

```
Zephyr SDK setup requires '7z' to be installed and available in the PATH.
FATAL ERROR: command "...\zephyr-sdk-x.y.z\setup.cmd /c" failed
```

### 원인

Zephyr SDK 압축 해제에 7-Zip이 필요한데 설치가 안 되어 있거나, 설치는 됐지만 PATH에 안 잡혀 있음.

### 해결

```powershell
winget install 7zip.7zip
```

⚠️ **7-Zip은 설치만으로 PATH에 자동으로 안 잡히는 경우가 많습니다.** 설치 후 새 터미널에서 `7z --help`를 쳐서 확인하세요. 인식이 안 되면 수동으로 PATH에 추가해야 합니다:

1. `Win + R` → `sysdm.cpl` → 엔터
2. 고급 탭 → 환경 변수 → 시스템 변수의 **Path** 선택 → 편집
3. 새로 만들기 → `C:\Program Files\7-Zip` 추가 → 확인
4. **VS Code/터미널 재시작** 후 `7z --help`로 재확인
5. `python -m west sdk install` 재시도

---

## 10. VS Code가 workspace의 Python(.venv)을 인식하지 못함

### 증상

터미널에서는 `.venv`가 정상 동작하는데, Zephyr IDE 확장의 빌드/설정 과정에서는 다른(시스템 기본) Python을 쓰는 것처럼 동작함

### 해결

1. Command Palette → **`Python: Select Interpreter`** → `.venv\Scripts\python.exe` 선택 (목록에 없으면 "Enter interpreter path..." → "Find..."로 직접 지정)
2. `Ctrl+,`(Settings) → Workspace 탭 → `zephyr-ide` 검색 → Python 관련 경로 설정이 있으면 동일하게 `.venv` 경로로 지정
3. 설정 후 **VS Code 재시작**

---

## 11. `Board qualifiers 'sr100' for board 'sr100_rdk' not found`

### 증상

```
CMake Error ... Board qualifiers `sr100` for board `sr100_rdk` not found.
Valid board targets for sr100_rdk are:
  sr100_rdk/sr100/m55
  ...
```

### 원인

SR110은 Zephyr에서 **M55/M4 이종 AMP 구조**입니다 (`18_MULTICORE_REALITY_LAB.md` 참고) — 코어마다 별도 OS 이미지를 빌드하는 구조라, 어느 코어를 타겟할지까지 보드 이름에 명시해야 합니다.

### 해결

```powershell
west build -b sr100_rdk/sr100/m55
```

일반적인 애플리케이션(시리얼 콘솔 사용)은 항상 **`m55`**를 씁니다. `m4`는 `18_MULTICORE_REALITY_LAB.md`에서 다루는 특수한 듀얼코어 IPC 시나리오에서만 필요합니다.

> TODO/VERIFY: M4용 정확한 board qualifier 표기(`sr100_rdk/sr100/m4`로 추정)는 이번 커리큘럼 작업에서 실제로 빌드해보지 않았습니다. `west boards`의 `Valid board targets` 출력에서 실제 표기를 확인하세요.

---

## 12. `No prj.conf file(s) was found in the ... folder`

### 증상

프로젝트를 직접 만들어서 빌드하면 이 에러가 남

### 원인

`prj.conf` 파일이 프로젝트 폴더에 없음. Zephyr 애플리케이션은 이 파일의 존재가 필수입니다(내용이 비어있어도 무방).

### 해결

```powershell
New-Item -ItemType File -Name prj.conf
```

**더 확실한 방법**: 처음부터 직접 만들지 말고, west workspace에 이미 포함된 공식 샘플(`zephyr/samples/hello_world`)을 그대로 쓰거나, `Zephyr IDE: Add Project`로 그 폴더를 등록하세요. 필요한 파일이 전부 갖춰져 있어 이런 실수 자체가 생기지 않습니다.

---

## 13. `ninja: error: loading 'build.ninja': The system cannot find the file specified.`

### 증상

```
ninja: error: loading 'build.ninja': The system cannot find the file specified.
FATAL ERROR: command exited with status 1: '...\cmake.EXE' --build '...\build'
```

### 원인

`build` 폴더가 이미 존재하는데 그 안에 `build.ninja`가 없거나 손상됨 (이전 빌드 시도가 CMake 설정 단계에서 실패한 채로 폴더만 남은 경우). west는 `build` 폴더가 있으면 설정 단계를 건너뛰고 바로 `ninja`를 실행하려다 이 에러를 냅니다.

### 해결

```powershell
west build -p always -b sr100_rdk/sr100/m55
```

`-p always`(pristine)는 기존 build 폴더를 무시하고 처음부터 다시 설정합니다. 또는 `build` 폴더를 직접 지우고(`Remove-Item -Recurse -Force build`) 재시도해도 동일합니다.

---

## 14. 플래싱 시 `west flash`가 SR110에 맞는 러너를 못 찾거나 예상과 다르게 동작함

### 증상

`west flash`를 실행했는데 원하는 대로 플래시가 안 되거나, 이미지 경로/오프셋 관련 에러가 남

### 원인

SR110은 `west flash`를 통한 표준 플래시 흐름 대신, `srsdk_tools`의 `openocd_flash.py`를 별도로 실행하는 방식이 실제로 확인된 방법입니다 (Zephyr 공식 `sr100_rdk` 보드 문서에서도 `west debugserver` + 별도 터미널에서 `openocd_flash.py` 실행을 안내). `west flash`만으로 충분한 ESP32-S3(esptool 기반)와는 플래시 절차 자체가 다릅니다.

### 해결

```powershell
cd srsdk_tools
python openocd_flash.py `
  --openocd <openocd.exe 경로> `
  --flash-offset 0x0 `
  --file-offset 0x0 `
  --cfg_path Input_Config\sr100_m55.cfg `
  --image <빌드/변환된 이미지 경로>.bin
```

`--image` 경로가 실제 빌드 산출물(`build\zephyr\zephyr.bin`)을 그대로 가리키는지, 아니면 별도 signing/packaging을 거친 이미지를 가리켜야 하는지는 SDK 릴리스마다 다를 수 있습니다 — `00_ZEPHYR_CURRICULUM_LAB.md`의 Flash 섹션 TODO/VERIFY 참고.

---

## 15. `cmake.exe`가 종료코드 `3221226505`(`0xC0000409`, STATUS_STACK_BUFFER_OVERRUN)로 크래시

### 증상

CMake 설정이 상당히 진행되다가(컴파일러 식별까지 성공하는 경우도 있음) 설명 없이 크래시. `FATAL ERROR: command exited with status 3221226505`

### 원인

**Windows 계정명이 한글**이고, Python 또는 Zephyr SDK 툴체인이 기본 위치(`C:\Users\<한글계정명>\...`)에 설치되어 있는 경우입니다. 임베디드 툴체인(특히 Xtensa 크로스컴파일러 등 네이티브 바이너리)이 비ASCII 경로를 완전히 지원하지 못해 발생하는 것으로 보입니다.

### 해결

Python과 SDK 둘 다 한글 없는 경로로 재설치합니다.

**Python** — [python.org](https://www.python.org/downloads/) 설치 파일로 "Customize installation" → 설치 경로를 직접 지정 (예: `D:\winapp\python\Python312`)

**SDK**:
```powershell
python -m west sdk install --install-base D:\zephyr_toolchains
```

두 경로 모두 바꾼 뒤, workspace의 venv를 새 Python으로 재생성합니다.

```powershell
cd <workspace 폴더>
Remove-Item -Recurse -Force .venv
D:\winapp\python\Python312\python.exe -m venv .venv
.venv\Scripts\Activate.ps1
python -m pip install west
cd zephyr\scripts
python -m pip install -r requirements.txt
cd ..\..
```

> Windows 계정명 자체는 바꿀 필요 없습니다 — Python/SDK "설치 위치"만 한글이 없는 별도 경로로 지정하면 충분합니다.

---

## 16. `west sdk install --install-base <경로>`를 줬는데 다른(한글) 경로에 설치됨

### 증상

```
Zephyr SDK version 1.0.1 is already installed at C:\Users\<한글계정>\zephyr-sdk-1.0.1. Using it.
```

`--install-base`로 새 경로를 지정했는데도 로그에 `Using it.`과 함께 기존 경로가 그대로 사용됨

### 원인

이전에 `--install-base` 없이(또는 다른 경로로) `west sdk install`을 한 번이라도 실행한 적이 있으면, west가 "이미 설치되어 있으니 새로 설치하지 않고 그걸 재사용"하는 로직 때문에 새로 지정한 `--install-base`가 무시됩니다.

### 해결

기존 설치와 그 등록 정보를 먼저 지운 뒤 재시도합니다.

```powershell
# 1. 기존 SDK 폴더 삭제 (실제 경로는 에러 로그에서 확인)
Remove-Item -Recurse -Force "C:\Users\<계정명>\zephyr-sdk-1.0.1"

# 2. CMake 패키지 레지스트리에 남은 등록 정보도 삭제
Remove-Item -Path "HKCU:\Software\Kitware\CMake\Packages\Zephyr-sdk" -Recurse -Force -ErrorAction SilentlyContinue

# 3. 원하는 경로로 재설치
python -m west sdk install --install-base D:\zephyr_toolchains
```

---

## 문제 해결 순서 요약 (막혔을 때 체크리스트)

- [ ] VS Code에 폴더가 열려있는가 (2번)
- [ ] 폴더 경로에 한글/공백이 없는가
- [ ] Host Tools가 전부 Installed인가 (3번)
- [ ] 다운로드된 SDK 이름이 `zephyr-sdk-`로 시작하는 공식 이름인가 (4번)
- [ ] `pip`/`west` 명령이 안 먹히면 `python -m pip`/`python -m west`로 시도했는가 (5번)
- [ ] 권한 에러가 나면 관리자 권한 + Defender 제외 폴더를 시도했는가 (6번)
- [ ] Python 3.11/3.12 전용 venv를 쓰고 있는가 (8번 — 대부분의 연쇄 문제를 예방)
- [ ] 7-Zip이 설치되고 PATH에 잡혀있는가 (9번)
- [ ] VS Code의 Python 인터프리터가 workspace의 `.venv`를 가리키는가 (10번)
- [ ] `west build`에 보드 코어까지 명시했는가 (`sr100_rdk/sr100/m55`) (11번)
- [ ] 직접 만든 프로젝트라면 `prj.conf` 파일이 존재하는가 (12번)
- [ ] `build` 폴더 에러가 나면 pristine 재빌드(`-p always`)를 시도했는가 (13번)
- [ ] `west flash` 대신 `srsdk_tools\openocd_flash.py`로 플래시했는가 (14번)
- [ ] **Windows 계정명이 한글이라면, Python과 SDK 설치 경로를 한글 없는 곳으로 지정했는가** (15번 — 가장 근본적인 예방책)
- [ ] `west sdk install --install-base`가 무시되면 기존 설치부터 지웠는가 (16번)
