# 06. Idle Thread와 CPU 유휴 시간

## 이 실습에서 배우는 것

Zephyr는 실행할 스레드가 하나도 없을 때, 시스템이 자동으로 만든 **Idle Thread**를 실행합니다. 이번 실습은 아주 낮은 우선순위(선점형)의 사용자 스레드로 그 효과를 관찰합니다.

## 코드

```c
#include <zephyr/kernel.h>

#define STACK_SIZE 1024

volatile uint32_t idle_counter = 0;

void idle_counter_entry(void *p1, void *p2, void *p3) {   // priority 10 - very low (close to system idle)
    while (1) {
        idle_counter++;
    }
}

void reporter_entry(void *p1, void *p2, void *p3) {        // priority 5
    while (1) {
        k_sleep(K_MSEC(1000));
        printk("IdleCounterThread incremented %u times in the last second\n", idle_counter);
        idle_counter = 0;
    }
}

void busy_burst_entry(void *p1, void *p2, void *p3) {       // priority 2 - occasionally hogs the CPU
    while (1) {
        k_sleep(K_MSEC(3000));
        printk("BusyBurstThread: starting a 1s CPU burst\n");
        int64_t start = k_uptime_get();
        while (k_uptime_get() - start < 1000) { }
        printk("BusyBurstThread: burst done\n");
    }
}

K_THREAD_DEFINE(idle_counter_id, STACK_SIZE, idle_counter_entry, NULL, NULL, NULL, 10, 0, 0);
K_THREAD_DEFINE(reporter_id, STACK_SIZE, reporter_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(busy_id, STACK_SIZE, busy_burst_entry, NULL, NULL, NULL, 2, 0, 0);

int main(void) {
    return 0;
}
```

## 실행 & 확인

- 평상시(`BusyBurstThread`가 쉬는 3초 구간)에는 `idle_counter` 보고 값이 매우 크게 나오는지 확인
- `BusyBurstThread`가 1초짜리 CPU burst를 도는 동안 값이 확 줄어드는지 확인 — 우선순위 2(더 급함)인 `BusyBurstThread`가 코어를 차지하는 동안, 우선순위 10(가장 느긋함)인 `idle_counter_entry`는 거의 실행되지 못합니다

## 관찰 포인트

- `idle_counter_entry`는 **선점형**(우선순위가 0 이상인 양수)이기 때문에, 04번 실습의 협조적 스레드와 달리 **스스로 yield하지 않아도** 더 높은 우선순위 스레드가 자동으로 선점해줍니다 — 그래서 이 코드엔 `k_yield()`가 없어도 안전합니다
- Zephyr의 진짜 Idle Thread는 이 사용자 스레드보다도 우선순위가 낮습니다 — 시스템이 내부적으로 예약해둔 가장 낮은 우선순위 슬롯을 씁니다
- 20번 실습(`20_RUNTIME_STATS_LAB.md`)에서 이런 수작업 카운팅 대신, Zephyr가 공식 제공하는 CPU 사용률 측정 API를 다룹니다

## 다음

07번 파일(`07_ISR_SEMAPHORE_LAB.md`)에서 인터럽트(ISR)가 Semaphore로 스레드를 깨우는 방법을 다룹니다.
