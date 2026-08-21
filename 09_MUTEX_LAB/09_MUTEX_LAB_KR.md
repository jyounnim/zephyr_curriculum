# 09. k_mutex vs k_sem — Priority Inheritance

## 이 실습에서 배우는 것

05번 실습의 우선순위 역전 문제를, `k_sem` 대신 `k_mutex`로 바꿔서 완화해봅니다. Zephyr의 `k_mutex`는 **기본적으로 Priority Inheritance가 내장**되어 있습니다 — 별다른 설정 없이 자료형을 세마포어에서 뮤텍스로 바꾸는 것만으로 상속 로직이 자동으로 적용됩니다.

## 핵심 개념

| 항목 | `k_sem` | `k_mutex` |
|---|---|---|
| 소유자 개념 | 없음 | 있음 |
| Priority Inheritance | 없음 | **기본으로 항상 적용** |
| ISR에서 사용 | 가능 (`k_sem_give`는 ISR에서 호출 가능) | **금지** (Lock/Unlock 모두 스레드 컨텍스트에서만) |
| 재귀적 잠금(같은 스레드가 또 lock) | 지원 안 함 | 지원 — 잠근 횟수만큼 풀어줘야 함 |
| 주 용도 | 이벤트/신호, ISR↔스레드 동기화 | 공유 자원의 상호배제 |

## 코드

05번 실습 코드에서 `k_sem` → `k_mutex`로 바꾼 버전입니다.

```c
#include <zephyr/kernel.h>

#define STACK_SIZE 1024

K_MUTEX_DEFINE(lock_mutex);   // priority inheritance is built in

void thread_l_entry(void *p1, void *p2, void *p3) {   // priority 8 (low) - holds the resource
    while (1) {
        k_mutex_lock(&lock_mutex, K_FOREVER);
        printk("ThreadL: acquired the resource\n");
        for (int i = 0; i < 20; i++) {
            k_busy_wait(50000);
        }
        printk("ThreadL: releasing the resource\n");
        k_mutex_unlock(&lock_mutex);
        k_sleep(K_MSEC(500));
    }
}

void thread_m_entry(void *p1, void *p2, void *p3) {    // priority 5 (medium) - does NOT need the resource
    while (1) {
        k_sleep(K_MSEC(300));
        printk("ThreadM: doing unrelated work (tries to preempt ThreadL)\n");
        for (int i = 0; i < 5; i++) {
            k_busy_wait(100000);
        }
    }
}

void thread_h_entry(void *p1, void *p2, void *p3) {    // priority 2 (high) - urgently needs the resource
    while (1) {
        k_sleep(K_MSEC(1000));
        int64_t start = k_uptime_get();
        printk("ThreadH: requesting the resource...\n");
        k_mutex_lock(&lock_mutex, K_FOREVER);
        int64_t waited_ms = k_uptime_get() - start;
        printk("ThreadH: acquired after waiting %lld ms\n", waited_ms);
        k_mutex_unlock(&lock_mutex);
    }
}

K_THREAD_DEFINE(l_id, STACK_SIZE, thread_l_entry, NULL, NULL, NULL, 8, 0, 0);
K_THREAD_DEFINE(m_id, STACK_SIZE, thread_m_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(h_id, STACK_SIZE, thread_h_entry, NULL, NULL, NULL, 2, 0, 0);

int main(void) {
    return 0;
}
```

## 실행 & 확인

- `ThreadH: acquired after waiting XXX ms` 값을 05번 실습(`k_sem` 버전) 결과와 비교해보세요 — 이번엔 대기 시간이 더 짧고 일정해야 합니다
- 내부 동작: `ThreadH`가 뮤텍스를 기다리기 시작하면, Zephyr는 현재 뮤텍스를 쥔 `ThreadL`의 우선순위를 일시적으로 `ThreadH`와 같은 수준(2)까지 끌어올립니다. 이 상태에서는 우선순위 5인 `ThreadM`이 더 이상 `ThreadL`을 선점할 수 없습니다

## 관찰 포인트

- `CONFIG_PRIORITY_CEILING`이라는 Kconfig 옵션으로, 상속받을 수 있는 우선순위의 상한선을 제한할 수도 있습니다 (기본값은 무제한) — `prj.conf`에서 조절 가능합니다
- 뮤텍스를 여러 개 겹쳐서 잠글 경우, **잠갔던 순서의 역순으로 풀어야** Priority Inheritance가 의도대로 동작한다는 점에 유의하세요 (공식 문서에서 명시하는 주의사항입니다)
- 07번 실습에서 ISR은 `k_sem_give`를 직접 호출할 수 있었지만, `k_mutex`는 Lock도 Unlock도 **ISR에서 절대 호출할 수 없습니다** — "이 뮤텍스를 누가 쥐고 있는가"라는 소유자 개념이 인터럽트 컨텍스트에는 자연스럽게 적용되지 않기 때문입니다

## 다음

10번 파일(`10_MSGQ_BASICS_LAB.md`)에서 스레드 간 실제 데이터를 전달하는 Message Queue를 다룹니다.
