# 07. 인터럽트(ISR) + k_sem

## 이 실습에서 배우는 것

ISR에서는 신호만 주고, 실제 처리는 Thread에서 하는 패턴을 다룹니다. Zephyr는 핀 번호를 코드에 직접 쓰지 않고 **Devicetree**로 하드웨어를 선언합니다. 그리고 결정적으로, **`k_sem_give()`는 ISR 안에서 별도 전용 함수 없이 그냥 그대로 호출**할 수 있습니다 — 스레드 컨텍스트와 ISR 컨텍스트를 하나의 통일된 API로 처리하는 Zephyr의 특징입니다.

## 준비물

- 택트 스위치(버튼) 1개 — GPIO5, GND (`GPIO_LAB.md`와 동일한 배선)
- Devicetree overlay 파일 추가 필요 (아래 참고)

## Devicetree Overlay

프로젝트의 `boards/esp32s3_devkitc.overlay` 파일에 아래 내용을 추가합니다 (파일이 없다면 새로 생성).

```dts
/ {
    aliases {
        sw0 = &button0;
    };

    buttons {
        compatible = "gpio-keys";
        button0: button_0 {
            gpios = <&gpio0 5 (GPIO_PULL_UP | GPIO_ACTIVE_LOW)>;
            label = "User button";
        };
    };
};
```

이 overlay는 "GPIO5에 풀업 저항을 걸고, 눌리면 LOW가 되는 버튼이 있다"는 걸 Zephyr에게 알려주고, `sw0`이라는 별칭(alias)으로 코드에서 쉽게 참조할 수 있게 해줍니다.

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

    printk("Ready. Press the button connected to GPIO5.\n");
    return 0;
}
```

## 실행 & 확인

- 버튼을 누를 때마다 `ButtonHandlerThread: interrupt signal received...`가 출력되는지 확인

## 관찰 포인트

- Zephyr는 `k_sem_give()` **하나로 스레드 컨텍스트와 ISR 컨텍스트 모두 처리**됩니다 — "깨어난 스레드가 즉시 실행되어야 하는가"는 커널이 알아서 판단합니다. API가 통일되어 있다는 게 이번 실습의 핵심 포인트입니다
- 반면 09번 실습에서 다룰 `k_mutex`는 **ISR에서 절대 사용할 수 없습니다** (Lock/Unlock 모두 금지) — Mutex는 "소유자"라는 개념이 있는데 ISR에는 그 개념이 자연스럽게 적용되지 않기 때문입니다. "이벤트 신호는 Semaphore, 자원 보호는 Mutex"라는 원칙을 기억하세요
- Devicetree 방식은 처음엔 번거로워 보이지만, 핀 번호가 코드에 하드코딩되지 않아 **보드를 바꿔도 overlay 파일만 교체하면 애플리케이션 코드는 그대로 재사용**할 수 있다는 장점이 있습니다

## 다음

08번 파일(`08_COUNTING_SEMAPHORE_LAB.md`)에서 자원 풀을 관리하는 Counting Semaphore를 다룹니다.
