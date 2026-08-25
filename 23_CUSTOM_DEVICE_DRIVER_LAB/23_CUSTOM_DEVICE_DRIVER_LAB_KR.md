# 23. 커스텀 디바이스 드라이버 만들기 — AHT20 예제

## 이 실습에서 배우는 것

`non_os/10_I2C_TEMP_HUMIDITY_LAB.md`에서 Arduino 방식으로 AHT20을 다뤘을 때는, `main.cpp` 안에서 `Adafruit_AHTX0` 라이브러리를 불러와 직접 `aht.getEvent(...)`를 호출했습니다 — **센서 통신 로직이 애플리케이션 코드와 뒤섞여 있는 구조**입니다.

Zephyr는 다릅니다. **Devicetree(하드웨어 선언) + Kconfig(활성화 여부) + 드라이버 코드(하드웨어 제어)를 완전히 분리**하고, 애플리케이션은 `sensor_sample_fetch()`/`sensor_channel_get()`이라는 **센서 종류와 무관한 공통 API**만 호출합니다. 이번 실습에서는 AHT20을 위한 드라이버를 처음부터 직접 만들어, 이 구조가 실제로 어떻게 동작하는지 확인합니다.

## Arduino 방식 vs Zephyr 드라이버 모델

| | Arduino (`non_os/10`) | Zephyr (이번 실습) |
|---|---|---|
| 하드웨어 선언 | 코드에 `Wire.begin()`, 라이브러리 객체 생성 | Devicetree 오버레이(`aht20@38 { compatible = "..."; reg = <0x38>; }`) |
| 활성화/비활성화 | 코드에서 `#include` 여부로 결정 | `prj.conf`의 `CONFIG_AHT20=y` |
| 통신 로직 | 앱 코드(`main.cpp`)에 라이브러리 호출 형태로 섞임 | 별도 드라이버 파일(`aht20.c`)에 완전히 분리 |
| 앱에서 값 읽기 | `aht.getEvent(&humidity, &temp)` (센서 전용 API) | `sensor_sample_fetch()` + `sensor_channel_get()` (모든 센서 공통 API) |
| 센서 교체 시 | 앱 코드를 다른 라이브러리로 다시 작성 | 오버레이의 `compatible`만 바꾸면 앱 코드는 그대로 |

## 준비물

- AHT20 모듈 (`non_os/10`과 동일 배선, I2C 주소 `0x38`)

## 전체 구조

Zephyr는 드라이버를 프로젝트 안에 "out-of-tree"로 넣는 걸 지원합니다 — Zephyr 소스 자체를 건드리지 않고, 내 프로젝트 폴더 안에 드라이버를 두는 방식입니다.

```
my_app/
├── CMakeLists.txt
├── Kconfig                              ← 앱 레벨 Kconfig (드라이버 Kconfig를 불러옴)
├── prj.conf
├── dts/
│   └── bindings/
│       └── sensor/
│           └── zds,aht20.yaml           ← Devicetree 바인딩
├── boards/
│   └── sr100_rdk_sr100_m55.overlay      ← 실제 하드웨어 배치
├── drivers/
│   └── sensor/
│       └── aht20/
│           ├── CMakeLists.txt
│           ├── Kconfig
│           └── aht20.c                  ← 드라이버 구현
└── src/
    └── main.c                           ← 앱 코드 (센서 종류를 모름)
```

---

## Step 1. Devicetree 바인딩 작성

바인딩은 "이 하드웨어는 어떤 속성을 가지는가"를 선언하는 명세입니다. AHT20은 I2C 주소 외에 특별한 속성이 없어서, I2C 장치 공통 바인딩(`i2c-device.yaml`)을 상속받기만 하면 됩니다.

**`dts/bindings/sensor/zds,aht20.yaml`**

```yaml
description: AHT20 I2C temperature and humidity sensor

compatible: "zds,aht20"

include: i2c-device.yaml
```

`zds`는 이 실습에서 임의로 정한 벤더 접두사입니다(실제 프로젝트에서는 회사/개인 이름 등으로 정하면 됩니다). `include: i2c-device.yaml`을 상속받으면 I2C 주소를 나타내는 `reg` 속성이 자동으로 포함됩니다.

## Step 2. Kconfig 작성 — 드라이버 켜고 끄기

**`drivers/sensor/aht20/Kconfig`**

```
config AHT20
    bool "AHT20 temperature and humidity sensor"
    default y
    depends on I2C
    help
      AHT20 I2C 온습도 센서 드라이버를 활성화합니다.
```

**`Kconfig`** (프로젝트 최상위, 앱 레벨)

```
source "Kconfig.zephyr"
rsource "drivers/sensor/aht20/Kconfig"
```

## Step 3. 드라이버 소스를 앱 타겟에 직접 추가

드라이버를 별도의 `zephyr_library()`로 만들지 않습니다. 앱의 `add_subdirectory()`로 불러온 서브디렉토리 안에서 `zephyr_library()`를 호출하면 **"application mode"**로 처리되는데, 이 시점엔 이미 최종 링크 대상 라이브러리 목록이 확정된 뒤라 `zephyr_library()` 호출이 사실상 아무 효과 없는 명령(no-op)이 됩니다 — 소스는 컴파일되지만 결과물이 최종 이미지에 링크되지 않아, `SENSOR_DEVICE_DT_INST_DEFINE`이 만든 디바이스 심볼을 나중에 `main.c`에서 찾지 못하는 `undefined reference` 에러로 이어집니다 (Zephyr GitHub PR #91143에서 이 동작이 의도된 경고임을 확인).

대신 드라이버 소스를 **`app` 타겟에 직접** 추가합니다 — `app`은 항상 확실하게 최종 이미지에 링크되는 특별한 타겟이라 이 문제 자체가 생기지 않습니다.

**`CMakeLists.txt`** (앱 최상위, Step 5 CMakeLists.txt를 아래 내용으로 교체)

```cmake
cmake_minimum_required(VERSION 3.20.0)

list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})   # 이 프로젝트의 dts/bindings도 검색하도록 추가

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(aht20_driver_example)

target_sources(app PRIVATE src/main.c)
target_sources_ifdef(CONFIG_AHT20 app PRIVATE drivers/sensor/aht20/aht20.c)
```

`target_sources_ifdef`는 Zephyr가 제공하는 CMake 확장 함수로, `CONFIG_AHT20`이 `y`일 때만 그 소스를 지정한 타겟(`app`)에 추가합니다 — `_ifdef`가 붙은 이유(안 쓰는 드라이버는 컴파일 대상에서 빠짐)는 이전과 동일합니다.

## Step 4. 드라이버 구현

**`drivers/sensor/aht20/aht20.c`**

```c
#define DT_DRV_COMPAT zds_aht20

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(aht20, CONFIG_SENSOR_LOG_LEVEL);

struct aht20_config {
    struct i2c_dt_spec i2c;
};

struct aht20_data {
    uint32_t humidity_raw;
    uint32_t temp_raw;
};

static int aht20_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    const struct aht20_config *config = dev->config;
    struct aht20_data *data = dev->data;
    uint8_t trigger_cmd[3] = {0xAC, 0x33, 0x00};   // AHT20 측정 트리거 명령 (데이터시트 규격)
    uint8_t buf[6];
    int ret;

    ret = i2c_write_dt(&config->i2c, trigger_cmd, sizeof(trigger_cmd));
    if (ret < 0) {
        LOG_ERR("Failed to trigger measurement: %d", ret);
        return ret;
    }

    k_sleep(K_MSEC(80));   // 변환 완료까지 대기 (데이터시트 권장 80ms)

    ret = i2c_read_dt(&config->i2c, buf, sizeof(buf));
    if (ret < 0) {
        LOG_ERR("Failed to read sensor data: %d", ret);
        return ret;
    }

    // buf[0]은 상태 바이트, buf[1..5]에 20bit 습도 + 20bit 온도가 나뉘어 담김
    data->humidity_raw = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | (buf[3] >> 4);
    data->temp_raw = (((uint32_t)buf[3] & 0x0F) << 16) | ((uint32_t)buf[4] << 8) | buf[5];

    return 0;
}

static int aht20_channel_get(const struct device *dev, enum sensor_channel chan,
                              struct sensor_value *val) {
    struct aht20_data *data = dev->data;

    if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
        // temp(C) = raw / 2^20 * 200 - 50   (밀리도 단위로 계산 후 정수부/소수부 분리)
        int64_t milli_c = ((int64_t)data->temp_raw * 200000 / 1048576) - 50000;
        val->val1 = (int32_t)(milli_c / 1000);
        val->val2 = (int32_t)((milli_c % 1000) * 1000);
    } else if (chan == SENSOR_CHAN_HUMIDITY) {
        // humidity(%) = raw / 2^20 * 100
        int64_t milli_pct = (int64_t)data->humidity_raw * 100000 / 1048576;
        val->val1 = (int32_t)(milli_pct / 1000);
        val->val2 = (int32_t)((milli_pct % 1000) * 1000);
    } else {
        return -ENOTSUP;
    }
    return 0;
}

static const struct sensor_driver_api aht20_api = {
    .sample_fetch = aht20_sample_fetch,
    .channel_get = aht20_channel_get,
};

static int aht20_init(const struct device *dev) {
    const struct aht20_config *config = dev->config;

    if (!device_is_ready(config->i2c.bus)) {
        LOG_ERR("I2C bus not ready");
        return -ENODEV;
    }

    k_sleep(K_MSEC(40));   // 전원 인가 후 안정화 대기 (데이터시트 권장)
    return 0;
}

#define AHT20_INIT(inst)                                                  \
    static struct aht20_data aht20_data_##inst;                          \
    static const struct aht20_config aht20_config_##inst = {             \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                                \
    };                                                                    \
    SENSOR_DEVICE_DT_INST_DEFINE(inst, aht20_init, NULL,                  \
                  &aht20_data_##inst, &aht20_config_##inst,               \
                  POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,                \
                  &aht20_api);

DT_INST_FOREACH_STATUS_OKAY(AHT20_INIT)
```

## Step 5. Devicetree 오버레이 — 실제 하드웨어 배치

**`boards/sr100_rdk_sr100_m55.overlay`** (이 파일명이 board target과 일치해서 자동 인식됨 — I2C 버스 스캐너 실습 참고)

SR110에서 I2C0는 SoC devicetree 기본값이 `status = "disabled"`이고 pinctrl 지정도 없어서, 단순히 `status = "okay"`만 넣는 것으로는 부족합니다 — pinctrl까지 명시해야 합니다(I2C 버스 스캐너 실습에서 확인된 내용, `i2c0_ms_scl`/`i2c0_ms_sda` 핀 그룹 사용). I2C1을 대신 쓴다면 이미 보드 dts에서 활성화되어 있어 오버레이가 필요 없습니다.

```dts
&i2c0 {
    status = "okay";
    pinctrl-names = "default";
    pinctrl-0 = <&i2c0_ms_scl &i2c0_ms_sda>;
    clock-frequency = <100000>;

    aht20_sensor: aht20@38 {
        compatible = "zds,aht20";
        reg = <0x38>;
        status = "okay";
    };
};
```

> `&i2c0`은 보드의 I2C 컨트롤러 노드 레이블입니다 — SR110에서는 `sr100_m55.dtsi`에서 확인됨. 다른 보드/Zephyr 버전에서는 레이블이 다를 수 있으니, `west build -t devicetree` 후 생성되는 병합된 devicetree(`build/zephyr/zephyr.dts`)에서 실제 I2C 노드 이름을 확인하세요.

## Step 6. prj.conf

```
CONFIG_I2C=y
CONFIG_I2C_DW=y
CONFIG_SENSOR=y
CONFIG_AHT20=y
CONFIG_LOG=y
```

> `CONFIG_I2C_DW=y`는 SR110의 I2C 컨트롤러가 Synopsys DesignWare 계열(`snps,designware-i2c`)이라 명시적으로 필요할 수 있습니다 — I2C 버스 스캐너 실습에서 devicetree 노드만으로는 부족했던 것과 같은 패턴입니다.

## Step 7. 앱 코드 — 센서 종류를 몰라도 되는 코드

**`src/main.c`**

```c
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

int main(void) {
    const struct device *aht20 = DEVICE_DT_GET_ANY(zds_aht20);

    if (aht20 == NULL || !device_is_ready(aht20)) {
        printk("AHT20 device not ready\n");
        return 0;
    }

    while (1) {
        struct sensor_value temp, humidity;

        sensor_sample_fetch(aht20);
        sensor_channel_get(aht20, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        sensor_channel_get(aht20, SENSOR_CHAN_HUMIDITY, &humidity);

        printk("Temperature: %d.%03d C, Humidity: %d.%03d %%\n",
               temp.val1, temp.val2 / 1000,
               humidity.val1, humidity.val2 / 1000);

        k_sleep(K_SECONDS(2));
    }
    return 0;
}
```

## 빌드 & 실행

```bash
west build -p always -b sr100_rdk/sr100/m55 <실습 lab 경로>
# 플래시: srsdk_tools 로 이동 후 python openocd_flash.py ... 실행 (SR110 플래시 가이드 참고)
# 콘솔: 시리얼 터미널을 230400bps 8N1로 연결
```

## 실행 & 확인

- 2초마다 온도/습도가 출력되는지 확인 — 값 자체는 `non_os/10`에서 본 것과 동일해야 정상입니다 (같은 센서, 같은 프로토콜을 다르게 구현한 것뿐)

## 관찰 포인트

- **`main.c`은 AHT20이라는 이름조차 몰라도 됩니다** (`DEVICE_DT_GET_ANY(zds_aht20)`로 디바이스를 가져오는 부분만 제외하면). `sensor_sample_fetch`/`sensor_channel_get`은 BME280이든 MPU6050이든 똑같이 호출하는 함수입니다 — **센서를 교체해도 이 두 줄의 호출 패턴 자체는 안 바뀝니다.** 이게 Zephyr의 드라이버 모델이 여러 하드웨어로 확장할 때 강점을 갖는 이유의 구체적인 사례입니다
- `DT_INST_FOREACH_STATUS_OKAY(AHT20_INIT)` 덕분에, 오버레이에 AHT20을 **여러 개**(다른 I2C 버스나 주소로) 선언하면 코드 수정 없이 인스턴스가 자동으로 여러 개 생성됩니다
- 이 실습에서 배운 가장 중요한 교훈은 코드 자체보다 **빌드 시스템의 동작 원리**입니다 — `zephyr_library()`처럼 Zephyr 전용 CMake 함수라도, 언제·어디서 호출하느냐(커널 모드 vs 애플리케이션 모드)에 따라 동작 여부가 갈립니다. 진짜 재사용 가능한 독립 컴포넌트로 만들고 싶다면, `zephyr/module.yml`을 갖춘 정식 Zephyr 모듈로 분리하고 `ZEPHYR_EXTRA_MODULES`로 등록하는 게 공식적으로 권장되는 다음 단계입니다
- `sensor_value`가 `val1`(정수부) + `val2`(마이크로 단위 소수부) 두 필드로 나뉜 이유: Zephyr 커널은 부동소수점 연산을 기본적으로 피하는 경향이 있습니다(일부 MCU는 FPU가 없거나, 있어도 인터럽트 컨텍스트에서 부동소수점 사용이 제한적) — 그래서 센서 값도 정수 두 개로 표현하는 고정소수점 방식을 표준으로 씁니다

## 트러블슈팅

| 증상 | 원인 / 해결 |
|---|---|
| `undefined reference to '__device_dts_ord_N'` 또는 `aht20_init` 등 링크 에러 | 드라이버 소스가 최종 이미지에 링크되지 않은 것 — 별도 `zephyr_library()` + `add_subdirectory()` 방식을 쓰고 있다면 Step 3에서 설명한 "application mode no-op" 문제일 가능성이 높습니다. `target_sources_ifdef(CONFIG_AHT20 app PRIVATE ...)`로 `app` 타겟에 직접 추가하는 방식(Step 3)으로 바꾸세요 |
| Devicetree 바인딩을 못 찾음 (`'zds,aht20' compatible not found`) | `list(APPEND DTS_ROOT ${CMAKE_CURRENT_SOURCE_DIR})`가 `find_package(Zephyr...)` **이전**에 있는지 확인 (순서 중요) |
| `device_is_ready()`가 계속 false | 오버레이의 I2C 버스 레이블(`&i2c0`, `&i2c1` 등)이 실제 보드와 다를 수 있음 — `west build -t devicetree`로 병합된 dts에서 정확한 I2C 노드명 확인 |
| 값이 `non_os/10`(Arduino)과 다르게 나옴 | 트리거 명령(`0xAC 0x33 0x00`)이나 raw 값 비트 계산식을 다시 확인 — 두 구현이 같은 프로토콜이므로 값이 같아야 정상 |
| `CONFIG_AHT20`이 안 먹힘 | 앱 최상위 `Kconfig` 파일에 `rsource "drivers/sensor/aht20/Kconfig"`가 있는지, 파일명이 정확히 `Kconfig`(확장자 없음)인지 확인 |
| Kconfig 순환 의존성 에러 (`...depends on I2C ... select I2C ...`) | `depends on I2C`와 `select I2C`를 동시에 쓴 게 원인 — 이 둘을 같은 심볼에 함께 쓰면 I2C의 역방향 의존 체인(다른 드라이버의 select 등)과 얽혀 순환 의존성 에러가 날 수 있는 Kconfig 안티패턴입니다. `select I2C`를 지우고 `depends on I2C`만 남기세요 |
| `fatal error: zephyr/heap_constants.h: No such file or directory` | Zephyr 4.4.0에서 보고된 이슈로, 별도 `zephyr_library()`를 쓸 때 `zephyr_generated_headers`보다 먼저 컴파일되며 생기던 문제입니다 — Step 3의 `target_sources_ifdef(app ...)` 방식으로 바꾸면 `app` 타겟이 이미 이 의존성을 올바르게 갖고 있어서 이 문제 자체가 발생하지 않습니다 |

## 다음 응용 주제

- 전원 관리 콜백(`pm_device_action_cb`) 추가 — `19_POWER_MANAGEMENT_LAB.md`와 결합해 센서도 절전 모드에 들어가게 만들기
- 트리거/인터럽트 지원 추가 — 폴링 대신 데이터 준비 인터럽트로 동작하는 드라이버로 확장
- 이 드라이버를 별도 west 모듈(zephyr/module.yml)로 분리해서, 여러 프로젝트에서 재사용 가능한 독립 컴포넌트로 만들기
