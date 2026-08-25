# 18. ESP32-S3에서의 멀티코어 — AMP 아키텍처와 IPC

## 이 실습에서 배우는 것

ESP32-S3용 Zephyr 포트는 **AMP(Asymmetric Multi-Processing)**라는 멀티코어 모델을 씁니다. 이번 실습은 이 구조가 정확히 무엇인지 이해하는 데서 그치지 않고, **공식 IPM(Inter-Processor Mailbox) 샘플을 실제로 빌드·실행하고, 그 안의 IPC API까지 직접 뜯어봅니다.**

## AMP 구조 이해하기

| 항목 | 설명 |
|---|---|
| 커널 이미지 | **코어마다 별도의** OS 이미지 (PROCPU용, APPCPU용 각각 따로 빌드) |
| Thread 배치 | 각 코어가 자기 이미지 안의 스레드만 알고, 다른 코어의 스레드 존재 자체를 모름 |
| 코어 간 통신 | **IPC 메커니즘으로 별도 통신 채널을 만들어야 함** (이번 실습에서 다루는 IPM이 그중 하나) |
| 빌드 방식 | **west sysbuild로 두 개의 이미지를 동시에 빌드**, 각각 코어에 맞춰 플래시 |
| 부트로더 | AMP 빌드는 **MCUboot가 필수**입니다 — 기본값인 Simple Boot로는 AMP를 실행할 수 없습니다 |
| 시리얼 출력 | **PROCPU만** `printk()` 사용 가능. APPCPU는 ESP32 ROM 함수(`ets_printf()`)를 대신 써야 함 |

## 왜 이렇게 설계됐는가

Zephyr는 원래 다양한 아키텍처(단일 코어 MCU부터 멀티코어까지)를 널리 지원하도록 설계된 RTOS입니다. ESP32 계열처럼 "두 코어가 서로 다른 역할(통신 전담 코어 vs 애플리케이션 코어)"을 맡는 게 흔한 칩에서는, 코어마다 독립된 펌웨어를 올리고 필요한 정보만 명시적으로 주고받는 AMP 모델이 자연스럽습니다. 실제로 이 구조는 한쪽 코어에서 Wi-Fi/BLE 스택 같은 걸 별도 이미지로 돌리고, 다른 쪽 코어에서 애플리케이션 로직을 돌리는 식으로 활용됩니다.

## ESP32-S3의 코어 간 통신 — IPM (Inter-Processor Mailbox)

ESP32 계열은 PRO_CPU와 APP_CPU 사이에 하드웨어로 내장된 **인터코어 메시징 채널이 4개** 있습니다. 이 중 **0번과 1번은 Wi-Fi/BT 스택 전용으로 예약**되어 있고, **2번과 3번이 애플리케이션에서 자유롭게 쓸 수 있는 채널**입니다. Zephyr는 이 하드웨어를 `ipm_esp32`라는 IPM(Inter-Processor Mailbox) 드라이버로 감싸서 표준 IPM API로 접근하게 해줍니다.

> ⚠️ 채널 하나가 한 번에 전달할 수 있는 데이터는 **최대 64바이트**입니다 — 더 큰 데이터를 주고받으려면 애플리케이션 레벨에서 여러 조각(chunk)으로 나눠 보내야 합니다.

### 핵심 IPM API

| 함수/타입 | 설명 |
|---|---|
| `ipm_send(dev, wait, id, data, size)` | 상대 코어로 메시지 전송. `id`는 메시지 종류를 구분하는 값, `size`는 최대 64바이트 |
| `ipm_register_callback(dev, callback, user_data)` | 메시지 수신 시 호출될 콜백 등록 |
| `ipm_callback_t` | `void (*)(const struct device *ipmdev, void *user_data, uint32_t id, volatile void *data)` — **이 콜백은 인터럽트 컨텍스트에서 실행됩니다** |
| `ipm_set_enabled(dev, enable)` | 수신 활성화/비활성화 |

콜백이 인터럽트 컨텍스트에서 실행된다는 점을 주목하세요 — **07번 실습(버튼 인터럽트)에서 배운 "콜백/ISR 안에서는 신호만 주고 처리는 별도로"라는 원칙이 여기서도 그대로 적용됩니다.**

## 실습 — 공식 샘플 빌드 & 실행

Zephyr는 이 IPM 통신을 보여주는 공식 샘플(`samples/drivers/ipm/ipm_esp32`)을 제공합니다. `--sysbuild` 옵션으로 PROCPU/APPCPU 이미지를 한 번에 빌드합니다.

```bash
west build -b esp32s3_devkitc/esp32s3/procpu --sysbuild samples/drivers/ipm/ipm_esp32
west flash
west espressif monitor
```

### 실행 결과

```
PRO_CPU is sending a request, waiting remote response...
PRO_CPU received a message from APP_CPU : APP_CPU uptime ticks 502
PRO_CPU is sending a request, waiting remote response...
PRO_CPU received a message from APP_CPU : APP_CPU uptime ticks 502
```

PRO_CPU(PROCPU)가 채널로 요청을 보내면, APP_CPU(APPCPU)가 자신의 uptime 값을 응답으로 돌려주고, PRO_CPU가 그걸 받아서 출력하는 왕복(round-trip) 구조입니다.

## 실습 — 샘플 코드 직접 뜯어보기

샘플 소스는 workspace 안의 `zephyr/samples/drivers/ipm/ipm_esp32/`에 있습니다. 아래 구조로 되어 있습니다.

```
ipm_esp32/
├── src/            ← PROCPU(메인 애플리케이션)용 소스
├── remote/         ← APPCPU(원격 코어)용 별도 프로젝트 (자체 CMakeLists.txt, prj.conf 포함)
└── sysbuild.cmake  ← 두 이미지를 함께 빌드하도록 지정
```

두 개의 **완전히 독립된 애플리케이션**이 한 폴더 안에 나란히 있는 구조라는 걸 확인하세요 — `remote/`는 `src/`를 `add_subdirectory`로 불러오는 게 아니라, 자체 빌드 시스템을 가진 별개의 프로젝트입니다. 이게 AMP의 "코어마다 독립된 이미지"라는 원칙이 프로젝트 구조로 드러난 모습입니다.

### 응용 과제

1. `src/main.c`(PROCPU 쪽)에서 보내는 요청 메시지의 `id` 값을 바꿔보고, `remote/`(APPCPU 쪽)에서 그 `id`에 따라 다른 응답을 하도록 분기를 추가해보세요
2. 현재는 "요청 1번 → 응답 1번"이지만, PROCPU가 카운터 값을 실어 보내고 APPCPU가 그 값을 2배로 만들어 돌려주는 식으로 데이터를 실제로 활용하는 로직을 추가해보세요 (64바이트 제한 안에서)
3. `ipm_register_callback`으로 등록한 콜백 안에 `k_sem_give()`를 넣고, 실제 처리(메시지 파싱, 로그 출력 등)는 별도 스레드에서 `k_sem_take()`로 깨어나 수행하도록 바꿔보세요 — 07번에서 배운 패턴을 IPM 콜백에 그대로 적용하는 연습입니다

## 관찰 포인트

- 지금까지의 01~17번 실습은 전부 **PROCPU 하나**에서만 동작했습니다 — 여러 스레드를 만들었지만, 그건 "한 코어 안에서의 멀티스레딩"이었지 "두 코어를 동시에 쓴 것"은 아니었습니다
- AMP 구조에서는 코어 간 통신 자체가 명시적인 설계 대상입니다 — 어떤 데이터를, 언제, 어떤 프로토콜로 주고받을지를 처음부터 정의해야 합니다. 같은 메모리 공간을 자유롭게 공유하는 방식과는 근본적으로 다른 접근입니다
- IPM은 "메시지 하나당 최대 64바이트"라는 제약이 있는 비교적 단순한 메커니즘입니다 — 더 큰 데이터나 더 복잡한 프로토콜이 필요하다면 Zephyr는 **OpenAMP + 공유 메모리 + rpmsg** 기반의 더 무거운 IPC 스택도 지원합니다 (`ipc_service` API). IPM은 "가볍고 빠른 신호/소량 데이터 교환", OpenAMP는 "대용량·구조화된 메시지 교환"에 적합하다고 구분하면 됩니다
- 실무에서 ESP32-S3 + Zephyr로 두 코어를 모두 활용해야 하는 프로젝트라면, 이 AMP 구조와 IPC 설계를 프로젝트 초기 단계부터 고려해야 합니다 — 나중에 코드 한 줄만 바꿔서 될 일이 아니라, 프로젝트 구조 자체를 처음부터 두 개의 독립된 이미지로 설계해야 합니다

## 트러블슈팅

| 증상 | 원인 / 해결 |
|---|---|
| 빌드는 되는데 APPCPU 쪽 로그가 안 보임 | 정상입니다 — APPCPU는 Zephyr의 `printk()`를 못 씁니다. `ets_printf()`(ESP32 ROM 함수)를 쓰거나, `remote/` 프로젝트 소스에서 실제로 어떤 출력 방식을 쓰는지 확인하세요 |
| `--sysbuild` 없이 빌드하면 실패하거나 한쪽 이미지만 만들어짐 | AMP는 두 이미지를 동시에 빌드해야 합니다 — `--sysbuild` 옵션을 빠뜨리지 않았는지 확인 |
| 플래시 후 아무 반응 없음 | AMP 빌드는 MCUboot가 필수입니다 — Simple Boot(기본값) 설정이 남아있으면 부팅 자체가 안 될 수 있습니다. sysbuild 로그에서 MCUboot 이미지가 같이 빌드됐는지 확인하세요 |
| `ipm_send`가 실패함(`-EBUSY` 등) | 이전 메시지에 대한 응답을 기다리는 중일 수 있습니다 — 콜백에서 응답을 처리하고 다음 요청을 보내는 순서(요청→응답→다음 요청)를 지키고 있는지 확인 |

## 다음

19번 파일(`19_POWER_MANAGEMENT_LAB.md`)에서 Zephyr의 전력 관리 기능을 다룹니다.
