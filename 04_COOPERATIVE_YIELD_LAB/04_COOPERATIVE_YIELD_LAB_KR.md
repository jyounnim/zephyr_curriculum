# 04. 협조적 스레드와 k_yield — 반드시 양보해야 하는 이유

## 이 실습에서 배우는 것

02번 실습에서 "협조적 스레드는 스스로 양보하기 전까지 절대 선점되지 않는다"는 걸 확인했습니다. 이번엔 그게 **같은 우선순위의 협조적 스레드끼리도 마찬가지**라는 걸 확인합니다. Zephyr의 time-slicing(같은 우선순위끼리 스케줄러가 자동으로 교대해주는 기능)은 **선점형 스레드에만 적용**되고, **협조적 스레드는 이 자동 교대 대상에서 아예 제외**됩니다.

## 핵심 개념

- `k_yield()`: 현재 스레드가 자발적으로 CPU를 양보 — 같은(또는 더 높은) 우선순위의 다른 Ready 스레드에게 순서를 넘김
- Zephyr의 Time-slicing(`CONFIG_TIMESLICING`)은 **선점형(preemptible) 스레드에만** 적용됩니다 — 협조적 스레드는 정의상 "스스로 양보하기 전까지 실행 유지"이므로 애초에 강제 교대 대상이 아닙니다

## 코드 (문제 상황 — yield 없음)

```c
#include <zephyr/kernel.h>

#define STACK_SIZE 1024

void coop_a_entry(void *p1, void *p2, void *p3) {
    long count = 0;
    while (1) {
        count++;
        if (count % 2000000 == 0) {
            printk("CoopA: count=%ld\n", count);
        }
        // no yield - if this thread got the CPU first, it keeps it forever
    }
}

void coop_b_entry(void *p1, void *p2, void *p3) {
    long count = 0;
    while (1) {
        count++;
        if (count % 2000000 == 0) {
            printk("CoopB: count=%ld\n", count);
        }
    }
}

K_THREAD_DEFINE(coop_a_id, STACK_SIZE, coop_a_entry, NULL, NULL, NULL, -1, 0, 0);
K_THREAD_DEFINE(coop_b_id, STACK_SIZE, coop_b_entry, NULL, NULL, NULL, -1, 0, 0);

int main(void) {
    return 0;
}
```

### 실행 & 확인

- 둘 다 우선순위 `-1`(협조적, 동일)인데도 **`CoopA`만 계속 출력되고 `CoopB`는 단 한 줄도 출력되지 않는지** 확인 — 먼저 CPU를 잡은 쪽이 영원히 놓아주지 않기 때문입니다. (크래시나 재부팅은 아니고, `CoopB`가 그냥 조용히 굶는 상태입니다)

## 코드 (해결 — 주기적으로 k_yield 호출)

```c
void coop_a_entry(void *p1, void *p2, void *p3) {
    long count = 0;
    while (1) {
        count++;
        if (count % 2000000 == 0) {
            printk("CoopA: count=%ld\n", count);
            k_yield();   // voluntarily let CoopB (equal priority) have a turn
        }
    }
}

void coop_b_entry(void *p1, void *p2, void *p3) {
    long count = 0;
    while (1) {
        count++;
        if (count % 2000000 == 0) {
            printk("CoopB: count=%ld\n", count);
            k_yield();
        }
    }
}
```

### 실행 & 확인

- 이번엔 `CoopA`와 `CoopB`가 번갈아 출력되는지 확인 — `k_yield()` 호출이 "이제 다른 스레드도 실행될 차례를 줘"라는 명시적 신호이기 때문입니다

## 관찰 포인트

- `k_yield()` 대신 `k_sleep(K_MSEC(0))`이나 `k_sleep(K_MSEC(10))`으로 바꿔도 비슷한 효과를 낼 수 있습니다 — 다만 `k_sleep`은 "최소 이 시간만큼은 확실히 대기"를 보장하는 반면, `k_yield()`는 "지금 당장 순서만 양보, 다른 스레드가 없으면 바로 이어서 실행"이라는 차이가 있습니다
- Zephyr의 협조적 스레드 굶주림은 **기본적으로 조용히 멈출 뿐, 자동으로 감지·복구해주는 안전장치가 없습니다**. 그만큼 협조적 스레드를 쓸 때는 개발자가 스스로 yield 지점을 잘 설계해야 할 책임이 큽니다
- 실무에서는 "정말 방해받으면 안 되는 아주 짧은 작업"에만 협조적 우선순위를 쓰고, 나머지는 선점형(0 이상)을 기본값으로 쓰는 것이 안전합니다 — Nordic 등 Zephyr 실무 가이드에서도 협조적 스레드는 제한적으로만 쓸 것을 권장합니다

## 다음

05번 파일(`05_PRIORITY_INVERSION_LAB.md`)에서 우선순위 역전 문제를 Zephyr에서 재현합니다.
