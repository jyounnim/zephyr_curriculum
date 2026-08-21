# 11. k_poll — 여러 커널 객체 동시 대기

## 이 실습에서 배우는 것

"여러 이벤트 소스 중 아무거나 먼저 오는 걸 처리하고 싶다"는 요구를, Zephyr는 **`k_poll`**이라는 범용적인 API로 해결합니다. `k_poll`은 Message Queue, Semaphore, Poll Signal(12번) 등 **다양한 종류의 커널 객체를 한 번에, 섞어서** 기다릴 수 있습니다.

## 사전 설정

`k_poll`은 기본적으로 꺼져 있는 기능입니다. `prj.conf`에 아래 옵션이 필요합니다.

```
CONFIG_POLL=y
```

이 옵션 없이 빌드하면 코드는 컴파일되지만, 링크 단계에서 `undefined reference to 'z_impl_k_poll'` 에러가 납니다 — `k_poll` API 자체가 커널 빌드에서 빠져있어서 실제 구현체를 찾지 못하는 것입니다.

## 핵심 개념

| 함수/매크로 | 설명 |
|---|---|
| `K_POLL_EVENT_STATIC_INITIALIZER(타입, 모드, 객체, 태그)` | 감시할 이벤트 하나를 정의 (컴파일 시점) |
| `k_poll(events, 개수, 대기시간)` | 이벤트 배열 중 하나라도 발생할 때까지 대기 |
| `event.state` | 대기가 풀린 뒤, 어떤 이벤트가 실제로 발생했는지 확인 |

## 코드

```c
#include <zephyr/kernel.h>

K_MSGQ_DEFINE(temp_msgq, sizeof(int), 3, 4);
K_MSGQ_DEFINE(humidity_msgq, sizeof(int), 3, 4);

void temp_sensor_entry(void *p1, void *p2, void *p3) {
    int temp_x10 = 200;
    while (1) {
        temp_x10 += 3;
        k_msgq_put(&temp_msgq, &temp_x10, K_FOREVER);
        k_sleep(K_MSEC(1500));
    }
}

void humidity_sensor_entry(void *p1, void *p2, void *p3) {
    int humidity_x10 = 400;
    while (1) {
        humidity_x10 += 10;
        k_msgq_put(&humidity_msgq, &humidity_x10, K_FOREVER);
        k_sleep(K_MSEC(900));
    }
}

void dispatcher_entry(void *p1, void *p2, void *p3) {
    struct k_poll_event events[2] = {
        K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
                                         K_POLL_MODE_NOTIFY_ONLY, &temp_msgq, 0),
        K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
                                         K_POLL_MODE_NOTIFY_ONLY, &humidity_msgq, 0),
    };

    while (1) {
        k_poll(events, 2, K_FOREVER);

        if (events[0].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
            int value;
            k_msgq_get(&temp_msgq, &value, K_NO_WAIT);
            printk("DispatcherThread: [TEMP] %d.%d C\n", value / 10, value % 10);
            events[0].state = K_POLL_STATE_NOT_READY;   // reset for the next k_poll call
        }
        if (events[1].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
            int value;
            k_msgq_get(&humidity_msgq, &value, K_NO_WAIT);
            printk("DispatcherThread: [HUMIDITY] %d.%d %%\n", value / 10, value % 10);
            events[1].state = K_POLL_STATE_NOT_READY;
        }
    }
}

K_THREAD_DEFINE(temp_id, 1024, temp_sensor_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(humidity_id, 1024, humidity_sensor_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(dispatcher_id, 1024, dispatcher_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
```

## 실행 & 확인

- `DispatcherThread` 하나가 온도(1.5초 주기)와 습도(0.9초 주기) 두 메시지 큐를 동시에 감시하며, 도착하는 순서대로 `[TEMP]`/`[HUMIDITY]`를 출력하는지 확인

## 관찰 포인트

- 매 루프마다 처리한 이벤트의 `state`를 `K_POLL_STATE_NOT_READY`로 **직접 리셋**해줘야 합니다 — `events` 배열을 재사용하기 때문에, 리셋하지 않으면 다음 `k_poll()` 호출 때 이미 처리한 이벤트를 또 "발생했다"고 착각할 수 있습니다
- `k_poll`은 `K_POLL_TYPE_SEM_AVAILABLE`(세마포어), `K_POLL_TYPE_SIGNAL`(12번에서 다룰 Poll Signal) 등 **서로 다른 종류의 이벤트를 같은 배열에 섞어서** 기다릴 수 있다는 점에서 매우 범용적인 API입니다
- `k_poll`은 사실 POSIX의 `poll()`/`select()`와 사상이 비슷합니다 — "여러 파일 디스크립터(또는 여기서는 커널 객체) 중 준비된 게 있는지 한 번에 확인"하는 패턴을, Zephyr는 커널 레벨의 IPC 객체에도 동일하게 적용한 것입니다

## 트러블슈팅

| 증상 | 원인 / 해결 |
|---|---|
| `undefined reference to 'z_impl_k_poll'` | `prj.conf`에 `CONFIG_POLL=y`가 빠짐 — 위 "사전 설정" 참고 |

## 다음

12번 파일(`12_POLL_SIGNAL_LAB.md`)에서 `k_poll`과 짝을 이루는 경량 이벤트, Poll Signal을 다룹니다.
