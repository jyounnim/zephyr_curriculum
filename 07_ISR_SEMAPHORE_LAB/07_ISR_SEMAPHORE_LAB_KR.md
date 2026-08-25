# 07. 인터럽트(ISR) + k_sem

## 이 실습에서 배우는 것

ISR에서는 신호만 주고, 실제 처리는 Thread에서 하는 패턴을 다룹니다. Zephyr는 핀 번호를 코드에 직접 쓰지 않고 **Devicetree**로 하드웨어를 선언합니다. 그리고 결정적으로, **`k_sem_give()`는 ISR 안에서 별도 전용 함수 없이 그냥 그대로 호출**할 수 있습니다 — 스레드 컨텍스트와 ISR 컨텍스트를 하나의 통일된 API로 처리하는 Zephyr의 특징입니다.

## 준비물

- 없음 — **SR110 RDK 보드에 이미 있는 물리 버튼(SW8)을 그대로 사용**합니다. 별도 배선이나 devicetree overlay가 필요 없습니다.

> **보드 확인 완료 (실제 `sr100_rdk_m55.dts` 검토, 2026-08)**: 보드 레벨 devicetree에 `sw0` alias가 이미 `user_button` 노드로 정의되어 있습니다.
>
> ```dts
> aliases {
>     ...
>     sw0 = &user_button;
>     ...
> };
>
> buttons: keys {
>     compatible = "gpio-keys";
>
>     user_button: user_button {
>         label = "SW8";
>         gpios = <&gpio_exp0 11 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
>         zephyr,code = <INPUT_KEY_0>;
>     };
> };
> ```
>
> 즉 실제 물리 버튼은 보드 실크스크린 기준 **SW8**이고, `gpio_exp0`(PCA6416A I2C GPIO 익스팬더, I2C1에 연결됨 — I2C 버스 스캐너 실습 참고)의 11번 핀에 물려 있습니다. `gpio_exp0`는 자신의 INT 핀(`&gpioa 3`)을 SoC GPIO에 연결해두고 있어서, 익스팬더 뒤에 있는 버튼이라도 Zephyr의 표준 GPIO 인터럽트 API(`gpio_pin_interrupt_configure_dt`, `gpio_add_callback` 등)를 그대로 쓸 수 있습니다 — I2C 너머에 있다는 사실이 애플리케이션 코드에서는 드러나지 않습니다.
>
> **결론: 새 GPIO를 정의하거나 외부 버튼을 배선할 필요 없이, 기존 `sw0` alias를 그대로 쓰면 됩니다.** 아래 코드는 `DT_ALIAS(sw0)`만 참조하므로 오버레이 파일 자체가 필요 없습니다.

## 핵심 개념

| 함수 | 설명 |
|---|---|
| `GPIO_DT_SPEC_GET(노드, gpios)` | Devicetree에 선언된 GPIO 정보를 코드에서 쓸 수 있는 구조체로 변환 |
| `gpio_pin_interrupt_configure_dt(...)` | 해당 핀에 인터럽트(엣지 감지 등) 설정 |
| `gpio_init_callback` / `gpio_add_callback` | 콜백 함수를 인터럽트에 연결 |
| `k_sem_give(&sem)` | **ISR 안에서 그대로 호출 가능** — 별도의 `FromISR` 버전이 필요 없음 |

## 코드

```c
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define BUTTON_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET(BUTTON_NODE, gpios);
static struct gpio_callback button_cb_data;

K_SEM_DEFINE(button_sem, 0, 1);

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    k_sem_give(&button_sem);   // safe to call directly from ISR context
}

void button_handler_entry(void *p1, void *p2, void *p3) {
    while (1) {
        k_sem_take(&button_sem, K_FOREVER);
        printk("ButtonHandlerThread: interrupt signal received, handling button press\n");
    }
}

K_THREAD_DEFINE(handler_id, 1024, button_handler_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    if (!gpio_is_ready_dt(&button)) {
        printk("Error: button device not ready\n");
        return 0;
    }

    gpio_pin_configure_dt(&button, GPIO_INPUT);
    gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);

    gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
    gpio_add_callback(button.port, &button_cb_data);

    printk("Ready. Press SW8.\n");
    return 0;
}
```

`prj.conf`:

```
CONFIG_GPIO=y
CONFIG_PRINTK=y
```

> `user_button`이 `gpio_exp0`(PCA6416A, `nxp,pcal6416a` compatible) 뒤에 있으므로, 이 드라이버가 Kconfig에서 자동으로 활성화되는지 확인하세요 — 보드 dts에 이미 `gpio_exp0` 노드가 `status = "okay"`로 들어있어서 대부분 자동으로 잡히지만, 만약 링크 에러가 나면 `CONFIG_GPIO_PCAL6416A` 관련 Kconfig 심볼을 명시적으로 켜야 할 수 있습니다.

## 실행 & 확인

콘솔은 230400bps 8N1.

- SW8 버튼을 누를 때마다 `ButtonHandlerThread: interrupt signal received...`가 출력되는지 확인

## 관찰 포인트

- Zephyr는 `k_sem_give()` **하나로 스레드 컨텍스트와 ISR 컨텍스트 모두 처리**됩니다 — "깨어난 스레드가 즉시 실행되어야 하는가"는 커널이 알아서 판단합니다. API가 통일되어 있다는 게 이번 실습의 핵심 포인트입니다
- 반면 09번 실습에서 다룰 `k_mutex`는 **ISR에서 절대 사용할 수 없습니다** (Lock/Unlock 모두 금지) — Mutex는 "소유자"라는 개념이 있는데 ISR에는 그 개념이 자연스럽게 적용되지 않기 때문입니다. "이벤트 신호는 Semaphore, 자원 보호는 Mutex"라는 원칙을 기억하세요
- Devicetree 방식은 처음엔 번거로워 보이지만, 핀 번호가 코드에 하드코딩되지 않아 **보드를 바꿔도 alias/overlay만 맞으면 애플리케이션 코드는 그대로 재사용**할 수 있다는 장점이 있습니다. 이번 실습이 그 직접적인 증거입니다 — ESP32-S3에서 SR110으로 바뀌면서 애플리케이션 코드(`main.c`)는 **한 줄도 안 바뀌었고**, 물리적으로는 SoC 직결 GPIO 버튼에서 I2C GPIO 익스팬더 뒤의 버튼으로 완전히 달라졌는데도 `DT_ALIAS(sw0)` 덕분에 코드가 그 차이를 몰라도 됩니다

## 다음

08번 파일(`08_COUNTING_SEMAPHORE_LAB.md`)에서 자원 풀을 관리하는 Counting Semaphore를 다룹니다.
