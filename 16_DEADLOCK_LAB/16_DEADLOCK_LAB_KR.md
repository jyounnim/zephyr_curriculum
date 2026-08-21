# 16. Deadlock 재현과 회피

## 이 실습에서 배우는 것

Zephyr의 `k_mutex`로 고전적인 Deadlock 문제를 재현합니다. Mutex 두 개를 서로 다른 순서로 잠그면 순환 대기(Deadlock)에 빠진다는 원리 자체는 RTOS 종류와 무관한 보편적인 문제입니다.

> ⚠️ 첫 번째 코드는 의도적으로 멈추게 만드는 코드입니다.

## 코드 (문제 상황 재현)

```c
#include <zephyr/kernel.h>

K_MUTEX_DEFINE(mutex_x);
K_MUTEX_DEFINE(mutex_y);

void thread_a_entry(void *p1, void *p2, void *p3) {
    while (1) {
        printk("ThreadA: taking MutexX\n");
        k_mutex_lock(&mutex_x, K_FOREVER);
        printk("ThreadA: got MutexX, now taking MutexY\n");
        k_sleep(K_MSEC(100));   // give ThreadB time to grab MutexY first
        k_mutex_lock(&mutex_y, K_FOREVER);

        printk("ThreadA: got both mutexes, working...\n");
        k_mutex_unlock(&mutex_y);
        k_mutex_unlock(&mutex_x);
        k_sleep(K_MSEC(1000));
    }
}

void thread_b_entry(void *p1, void *p2, void *p3) {
    while (1) {
        printk("ThreadB: taking MutexY\n");
        k_mutex_lock(&mutex_y, K_FOREVER);
        printk("ThreadB: got MutexY, now taking MutexX\n");
        k_sleep(K_MSEC(100));   // give ThreadA time to grab MutexX first
        k_mutex_lock(&mutex_x, K_FOREVER);

        printk("ThreadB: got both mutexes, working...\n");
        k_mutex_unlock(&mutex_x);
        k_mutex_unlock(&mutex_y);
        k_sleep(K_MSEC(1000));
    }
}

K_THREAD_DEFINE(a_id, 1024, thread_a_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(b_id, 1024, thread_b_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
```

### 실행 & 확인

- 로그가 아래 패턴에서 그대로 멈추는지 확인:
  ```
  ThreadA: taking MutexX
  ThreadB: taking MutexY
  ThreadA: got MutexX, now taking MutexY
  ThreadB: got MutexY, now taking MutexX
  (여기서 영원히 정지)
  ```

## 코드 (해결 — 잠금 순서 통일)

```c
void thread_b_entry(void *p1, void *p2, void *p3) {
    while (1) {
        printk("ThreadB: taking MutexX\n");
        k_mutex_lock(&mutex_x, K_FOREVER);   // ThreadA와 동일하게 X를 먼저
        printk("ThreadB: got MutexX, now taking MutexY\n");
        k_sleep(K_MSEC(100));
        k_mutex_lock(&mutex_y, K_FOREVER);

        printk("ThreadB: got both mutexes, working...\n");
        k_mutex_unlock(&mutex_y);
        k_mutex_unlock(&mutex_x);
        k_sleep(K_MSEC(1000));
    }
}
```

### 실행 & 확인

- 두 스레드 모두 "got both mutexes, working..."까지 정상 도달하며 무한 반복되는지 확인

## 관찰 포인트

- 이번 실습에서 쓴 **Lock Ordering(잠금 순서 통일)**은 RTOS 종류와 무관하게 통하는 범용적인 Deadlock 회피 전략입니다
- Zephyr에는 `k_mutex_lock`에 타임아웃을 주는 방법(`K_FOREVER` 대신 `K_MSEC(500)` 등)도 동일하게 있어, 자원을 못 얻으면 실패로 처리하고 재시도하는 방식으로도 회피할 수 있습니다
- 09번에서 배운 Priority Inheritance는 이번 Deadlock 문제와는 무관합니다 — 두 스레드가 서로 다른 자원을 기다리며 영원히 멈춘 상태이므로, 어느 쪽 우선순위를 올려줘도 순환 자체가 풀리지 않습니다

## 다음

17번 파일(`17_CRITICAL_SECTION_LAB.md`)에서 아주 짧은 공유 데이터를 보호하는 방법들을 비교합니다.
