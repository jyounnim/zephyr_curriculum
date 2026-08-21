# 10. Message Queue 기본 (k_msgq)

## 이 실습에서 배우는 것

Zephyr에서 스레드 간에 실제 데이터를 전달하는 기본 자료구조는 `k_msgq`입니다. 고정 크기 항목을 FIFO(선입선출)로 주고받습니다.

## 핵심 개념

```c
K_MSGQ_DEFINE(msgq, 항목크기, 개수, 정렬바이트);
```

| 함수 | 설명 |
|---|---|
| `k_msgq_put(&msgq, &data, 대기시간)` | 항목 추가 (성공 시 0 반환) |
| `k_msgq_get(&msgq, &buffer, 대기시간)` | 항목 꺼내오기 |

## 코드

```c
#include <zephyr/kernel.h>

struct sensor_data {
    int id;
    int value_x10;   // fixed-point: real value * 10 (e.g. 205 means 20.5)
};

K_MSGQ_DEFINE(data_msgq, sizeof(struct sensor_data), 5, 4);   // 5 slots

void sensor_entry(void *p1, void *p2, void *p3) {
    int counter = 0;
    while (1) {
        struct sensor_data data;
        data.id = counter++;
        data.value_x10 = 200 + (counter % 10) * 5;

        if (k_msgq_put(&data_msgq, &data, K_MSEC(100)) == 0) {
            printk("SensorThread: sent id=%d value=%d.%d\n",
                   data.id, data.value_x10 / 10, data.value_x10 % 10);
        } else {
            printk("SensorThread: queue full, send failed\n");
        }
        k_sleep(K_MSEC(500));
    }
}

void process_entry(void *p1, void *p2, void *p3) {
    struct sensor_data received;
    while (1) {
        k_msgq_get(&data_msgq, &received, K_FOREVER);
        printk("ProcessThread: received id=%d value=%d.%d\n",
               received.id, received.value_x10 / 10, received.value_x10 % 10);
    }
}

K_THREAD_DEFINE(sensor_id, 1024, sensor_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(process_id, 1024, process_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
```

## 실행 & 확인

- `id`와 `value`가 순서대로 정확히 전달되는지 확인

## 관찰 포인트

- `value_x10`처럼 소수점 값을 정수로 스케일링(× 10)해서 다뤘습니다 — Zephyr의 `printk()`는 기본 설정에서 **`%f` 같은 부동소수점 포맷을 지원하지 않습니다** (`CONFIG_CBPRINTF_FP_SUPPORT`를 켜야 가능하며, 코드 크기가 커집니다). 임베디드에서는 이런 정수 기반 고정소수점 트릭이 실무에서도 흔히 쓰입니다
- `k_msgq_put`의 대기시간을 `K_MSEC(100)`으로 줬습니다 — `ProcessThread`를 없앤 채로 실행하면, 5개가 다 차고 나서 "queue full, send failed"가 나오는지 확인해보세요
- Zephyr에는 `k_msgq` 외에도 **`k_queue`**라는 더 유연한 자료구조가 있습니다 — `k_msgq`는 고정 크기 항목만 담을 수 있지만, `k_queue`는 크기가 다른 데이터도(연결 리스트 방식으로) 넣을 수 있습니다. 다만 그만큼 사용법이 더 로우레벨이라, 대부분의 경우 `k_msgq`로 충분합니다

## 다음

11번 파일(`11_K_POLL_LAB.md`)에서 여러 커널 객체를 하나의 스레드가 동시에 기다리는 `k_poll`을 다룹니다.
