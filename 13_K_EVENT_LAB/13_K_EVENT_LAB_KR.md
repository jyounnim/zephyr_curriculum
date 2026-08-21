# 13. k_event — 다중 조건 대기

## 이 실습에서 배우는 것

여러 조건을 동시에 기다려야 할 때 쓰는 Zephyr 기능이 `k_event`입니다. 32비트 비트마스크로 여러 조건을 관리하고, "아무거나 하나"(OR) 또는 "전부 다"(AND) 충족될 때까지 대기할 수 있습니다.

## 사전 설정

`prj.conf`에 아래 옵션이 필요합니다.

```
CONFIG_EVENTS=y
```

## 핵심 개념

| 함수 | 설명 |
|---|---|
| `K_EVENT_DEFINE(이름)` | Event 객체를 정적으로 정의 |
| `k_event_post(&event, bits)` | 비트를 **추가**(OR) — 기존 비트는 유지한 채 새 비트를 켬 |
| `k_event_wait(&event, bits, reset, 대기시간)` | 지정한 비트들 중 **아무거나 하나**라도 켜지면 반환 |
| `k_event_wait_all(&event, bits, reset, 대기시간)` | 지정한 비트들이 **전부** 켜져야 반환 |

## 코드

```c
#include <zephyr/kernel.h>

#define WIFI_READY_BIT     BIT(0)
#define SENSOR_READY_BIT   BIT(1)
#define STORAGE_READY_BIT  BIT(2)
#define ALL_READY_BITS     (WIFI_READY_BIT | SENSOR_READY_BIT | STORAGE_READY_BIT)

K_EVENT_DEFINE(system_events);

void wifi_init_entry(void *p1, void *p2, void *p3) {
    printk("WifiInitThread: initializing...\n");
    k_sleep(K_MSEC(1500));
    printk("WifiInitThread: ready\n");
    k_event_post(&system_events, WIFI_READY_BIT);
}

void sensor_init_entry(void *p1, void *p2, void *p3) {
    printk("SensorInitThread: initializing...\n");
    k_sleep(K_MSEC(800));
    printk("SensorInitThread: ready\n");
    k_event_post(&system_events, SENSOR_READY_BIT);
}

void storage_init_entry(void *p1, void *p2, void *p3) {
    printk("StorageInitThread: initializing...\n");
    k_sleep(K_MSEC(2200));
    printk("StorageInitThread: ready\n");
    k_event_post(&system_events, STORAGE_READY_BIT);
}

void main_app_entry(void *p1, void *p2, void *p3) {
    printk("MainThread: waiting for all subsystems...\n");
    k_event_wait_all(&system_events, ALL_READY_BITS, false, K_FOREVER);
    printk("MainThread: all subsystems ready! starting main application...\n");
    while (1) {
        k_sleep(K_MSEC(5000));
        printk("MainThread: running normally\n");
    }
}

K_THREAD_DEFINE(main_app_id, 1024, main_app_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(wifi_id, 1024, wifi_init_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(sensor_id, 1024, sensor_init_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(storage_id, 1024, storage_init_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
```

## 실행 & 확인

- 세 초기화 스레드가 각자 다른 시간(1.5초/0.8초/2.2초)에 끝나는데도, `MainThread`는 **가장 늦게 끝나는 StorageInitThread(2.2초) 기준**으로 "all subsystems ready"를 출력하는지 확인

## 관찰 포인트

- `k_event_wait_all`을 `k_event_wait`(아무거나 하나)로 바꿔보세요 — `SensorInitThread`(가장 빠른 0.8초)가 끝나자마자 `MainThread`가 바로 진행되는지 확인할 수 있습니다
- `k_event_wait`/`k_event_wait_all`의 3번째 인자(`reset`)를 `true`로 주면, 대기가 풀리는 순간 이벤트 객체의 비트가 초기화됩니다 — 다만 공식 문서는 **여러 스레드가 같은 Event 객체를 동시에 기다릴 때 reset을 쓰면 경쟁 상태(race condition)가 생길 수 있다**고 경고합니다. 이런 경우를 위해 원자적으로 안전하게 초기화까지 처리하는 `k_event_wait_safe`/`k_event_wait_all_safe` 버전도 있습니다
- `k_event_post`(추가/병합)와 `k_event_set`(덮어쓰기)의 차이를 기억하세요 — 지금처럼 "여러 곳에서 각자 자기 비트만 켜는" 상황에는 반드시 `k_event_post`를 써야 합니다. `k_event_set`을 쓰면 다른 스레드가 이미 켜둔 비트까지 지워버릴 수 있습니다

## 다음

14번 파일(`14_K_TIMER_LAB.md`)에서 주기적/일회성 작업을 처리하는 `k_timer`를 다룹니다.
