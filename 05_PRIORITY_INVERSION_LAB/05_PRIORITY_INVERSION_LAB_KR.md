# 05. 우선순위 역전(Priority Inversion) 재현

## 이 실습에서 배우는 것

우선순위 역전이라는 고전적인 RTOS 문제를 Zephyr에서 재현합니다. 여기서는 세 스레드 모두 **선점형(우선순위 0 이상)**으로 만들어서, 04번에서 다룬 협조적 스레드의 특수성과 이 문제(우선순위 역전)를 분리해서 볼 수 있게 했습니다.

> Zephyr는 숫자가 작을수록 우선순위가 높습니다 — `ThreadH`(2)가 가장 급하고, `ThreadL`(8)이 가장 느긋합니다.

## 코드

```c
#include <zephyr/kernel.h>

#define STACK_SIZE 1024

K_SEM_DEFINE(lock_sem, 1, 1);   // binary semaphore used as a simple lock (not priority-aware)

void thread_l_entry(void *p1, void *p2, void *p3) {   // priority 8 (low) - holds the resource
    while (1) {
        k_sem_take(&lock_sem, K_FOREVER);
        printk("ThreadL: acquired the resource\n");
        for (int i = 0; i < 20; i++) {
            k_busy_wait(50000);   // simulate real work, 50ms chunks
        }
        printk("ThreadL: releasing the resource\n");
        k_sem_give(&lock_sem);
        k_sleep(K_MSEC(500));
    }
}

void thread_m_entry(void *p1, void *p2, void *p3) {    // priority 5 (medium) - does NOT need the resource
    while (1) {
        k_sleep(K_MSEC(300));
        printk("ThreadM: doing unrelated work (preempts ThreadL)\n");
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
        k_sem_take(&lock_sem, K_FOREVER);
        int64_t waited_ms = k_uptime_get() - start;
        printk("ThreadH: acquired after waiting %lld ms\n", waited_ms);
        k_sem_give(&lock_sem);
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

- `ThreadH: acquired after waiting XXX ms` 값을 확인하세요 — `ThreadM`이 계속 끼어들며 `ThreadL`이 자원을 오래 붙잡고 있어서, 가장 급한 `ThreadH`가 예상보다 오래 기다립니다
- `ThreadM`을 만드는 `K_THREAD_DEFINE(m_id, ...)` 줄을 잠시 지우고 다시 빌드해보세요 — `ThreadM`의 방해가 없으면 `ThreadH`의 대기 시간이 훨씬 짧고 일정해집니다

## 관찰 포인트

- `k_sem`은 "소유자" 개념이 없는 단순 신호 장치라, 스케줄러가 `ThreadL`을 배려해서 승격시켜줄 방법이 없습니다
- 이 문제를 해결하려면 09번 실습처럼 `k_mutex`로 바꿔야 합니다 — Zephyr의 `k_mutex`는 **기본적으로 Priority Inheritance가 내장**되어 있어서, 별다른 설정 없이 `k_sem`을 `k_mutex`로만 바꿔도 문제가 완화됩니다

## 다음

06번 파일(`06_IDLE_THREAD_LAB.md`)에서 Idle Thread와 CPU 유휴 시간을 다룹니다.
