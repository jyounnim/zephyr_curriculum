# 18. ESP32-S3에서의 멀티코어 — AMP 아키텍처

## 이 실습에서 배우는 것

ESP32-S3용 Zephyr 포트는 **AMP(Asymmetric Multi-Processing)**라는 멀티코어 모델을 씁니다. 이번 실습은 코드 실습보다는, 이 구조가 정확히 무엇이고 왜 이렇게 설계됐는지 이해하는 데 집중합니다.

## AMP 구조 이해하기

| 항목 | 설명 |
|---|---|
| 커널 이미지 | **코어마다 별도의** OS 이미지 (PROCPU용, APPCPU용 각각 따로 빌드) |
| Thread 배치 | 각 코어가 자기 이미지 안의 스레드만 알고, 다른 코어의 스레드 존재 자체를 모름 |
| 코어 간 통신 | **IPC(OpenAMP 등) 메커니즘으로 별도 통신 채널을 만들어야 함** |
| 빌드 방식 | **west sysbuild로 두 개의 이미지를 동시에 빌드**, 각각 코어에 맞춰 플래시 |
| 시리얼 출력 | **PROCPU만** `printk()` 사용 가능. APPCPU는 ESP32 ROM 함수(`ets_printf()`)를 대신 써야 함 |

## 왜 이렇게 설계됐는가

Zephyr는 원래 다양한 아키텍처(단일 코어 MCU부터 멀티코어까지)를 널리 지원하도록 설계된 RTOS입니다. ESP32 계열처럼 "두 코어가 서로 다른 역할(통신 전담 코어 vs 애플리케이션 코어)"을 맡는 게 흔한 칩에서는, 코어마다 독립된 펌웨어를 올리고 필요한 정보만 명시적으로 주고받는 AMP 모델이 자연스럽습니다. 실제로 이 구조는 한쪽 코어에서 Wi-Fi/BLE 스택 같은 걸 별도 이미지로 돌리고, 다른 쪽 코어에서 애플리케이션 로직을 돌리는 식으로 활용됩니다.

## 실제로 두 코어를 함께 써보고 싶다면

Zephyr는 이런 AMP 구성을 위한 공식 샘플(`samples/drivers/ipm/ipm_esp32`)을 제공합니다. 아래처럼 `--sysbuild` 옵션으로 두 이미지를 한 번에 빌드합니다.

```bash
west build -b esp32s3_devkitc/esp32s3/procpu --sysbuild samples/drivers/ipm/ipm_esp32
west flash
west espressif monitor
```

이 샘플은 PROCPU가 APPCPU에게 메시지를 보내고, APPCPU가 응답을 돌려주는 왕복(round-trip) 통신을 보여줍니다. 로그는 대략 이런 식으로 나옵니다.

```
PRO_CPU is sending a request, waiting remote response...
PRO_CPU received a message from APP_CPU : APP_CPU uptime ticks 502
```

이 실습 시리즈에서는 이 샘플을 직접 실행해보는 것을 "다음 단계"로만 권장하고, 본문 코드로 재현하지는 않습니다 — IPC 설정(devicetree, sysbuild 구성)이 지금까지의 실습보다 훨씬 복잡하고, 프로젝트 구조 자체가 다르기 때문입니다.

## 관찰 포인트

- 지금까지의 01~17번 실습은 전부 **PROCPU 하나**에서만 동작했습니다 — 여러 스레드를 만들었지만, 그건 "한 코어 안에서의 멀티스레딩"이었지 "두 코어를 동시에 쓴 것"은 아니었습니다
- AMP 구조에서는 코어 간 통신 자체가 명시적인 설계 대상입니다 — 어떤 데이터를, 언제, 어떤 프로토콜로 주고받을지를 처음부터 정의해야 합니다. 같은 메모리 공간을 자유롭게 공유하는 방식과는 근본적으로 다른 접근입니다
- 실무에서 ESP32-S3 + Zephyr로 두 코어를 모두 활용해야 하는 프로젝트라면, 이 AMP 구조와 IPC 설계를 프로젝트 초기 단계부터 고려해야 합니다 — 나중에 코드 한 줄만 바꿔서 될 일이 아니라, 프로젝트 구조 자체를 처음부터 두 개의 독립된 이미지로 설계해야 합니다

## 다음

19번 파일(`19_POWER_MANAGEMENT_LAB.md`)에서 Zephyr의 전력 관리 기능을 다룹니다.
