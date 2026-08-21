# 20. Thread Runtime Stats — 정식 CPU 사용률 API

## 이 실습에서 배우는 것

06번 실습에서는 우선순위 낮은 스레드로 CPU 유휴 시간을 수작업으로 흉내 냈습니다. Zephyr는 이를 위한 **공식 API**를 제공합니다 — 특정 칩 전용 API가 아니라 **모든 보드에서 동일하게 쓸 수 있는 이식성 있는 표준 API**입니다.

## 사전 설정

`prj.conf`에 아래 옵션을 추가합니다.

```
CONFIG_THREAD_RUNTIME_STATS=y
CONFIG_SCHED_THREAD_USAGE=y
```

## 핵심 개념

| 함수 | 설명 |
|---|---|
| `k_thread_runtime_stats_get(스레드ID, &stats)` | 특정 스레드의 실행 사이클 수를 가져옴 |
| `k_thread_runtime_stats_all_get(&stats)` | 시스템(코어) 전체의 실행 사이클 수를 가져옴 |
| `stats.execution_cycles` | 실행에 사용된 총 CPU 사이클 수 |

사용률(%) 계산 공식: `(스레드의 execution_cycles × 100) / 전체 execution_cycles`

## 코드

```c
#include <zephyr/kernel.h>

#define STACK_SIZE 1024

void busy_entry(void *p1, void *p2, void *p3) {
    while (1) {
        for (volatile int i = 0; i < 500000; i++) { }   // CPU-bound work
        k_sleep(K_MSEC(50));
    }
}

void idle_ish_entry(void *p1, void *p2, void *p3) {
    while (1) {
        k_sleep(K_MSEC(500));   // mostly sleeping
    }
}

K_THREAD_DEFINE(busy_id, STACK_SIZE, busy_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(idlish_id, STACK_SIZE, idle_ish_entry, NULL, NULL, NULL, 5, 0, 0);

void monitor_entry(void *p1, void *p2, void *p3) {
    while (1) {
        k_sleep(K_SECONDS(2));

        struct k_thread_runtime_stats busy_stats, idlish_stats, cpu_stats;
        k_thread_runtime_stats_get(busy_id, &busy_stats);
        k_thread_runtime_stats_get(idlish_id, &idlish_stats);
        k_thread_runtime_stats_all_get(&cpu_stats);

        uint32_t busy_pct = (uint32_t)((busy_stats.execution_cycles * 100ULL) / cpu_stats.execution_cycles);
        uint32_t idlish_pct = (uint32_t)((idlish_stats.execution_cycles * 100ULL) / cpu_stats.execution_cycles);

        printk("CPU usage - BusyThread: %u%%, IdleIshThread: %u%%\n", busy_pct, idlish_pct);
    }
}

K_THREAD_DEFINE(monitor_id, STACK_SIZE, monitor_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
```

## 실행 & 확인

- `BusyThread`의 사용률이 `IdleIshThread`보다 눈에 띄게 높게 나오는지 확인 (정확한 수치보다는 상대적 크기 차이가 중요합니다)

## 관찰 포인트

- 이 API는 **누적값**을 기준으로 계산됩니다 — 지금 코드는 부팅 이후 누적된 총 사이클을 매번 다시 계산하므로, 시간이 지날수록 두 스레드의 비율 차이가 점점 완만하게 수렴하는 걸 볼 수 있습니다. "최근 구간"만의 사용률을 보고 싶다면 이전 측정값을 저장해뒀다가 차이(delta)를 계산해야 합니다
- `CONFIG_SCHED_THREAD_USAGE_ANALYSIS`를 추가로 켜면, `current_cycles`/`peak_cycles`/`average_cycles`처럼 더 세밀한 통계도 얻을 수 있습니다 (스레드가 스케줄인~스케줄아웃되는 구간 단위의 실행 시간 분석)
- Zephyr는 이 통계를 활용하는 `Thread Analyzer`라는 디버깅 서비스도 별도로 제공합니다 — 이 실습처럼 직접 API를 호출하는 대신, 표준화된 형태로 전체 스레드 목록의 스택/CPU 사용률을 한 번에 출력해주는 편의 기능입니다

## 다음

21번 파일(`21_PRODUCER_CONSUMER_LAB.md`)에서 지금까지 배운 요소들을 조합해 Producer-Consumer 패턴을 구현합니다.
