# 17. Critical Section — irq_lock / k_sched_lock / k_spinlock

## 이 실습에서 배우는 것

14번 실습에서 배운 "Zephyr의 타이머 콜백은 ISR 컨텍스트에서 실행된다"는 사실을 활용해, **스레드와 ISR이 동시에 같은 변수를 건드리는 진짜 Race Condition**을 재현합니다. 경쟁 상대가 "다른 코어"가 아니라 "ISR"이라는 점이 특징이지만, 짧은 공유 데이터를 보호해야 한다는 결론은 동일합니다.

## 코드 (문제 상황 — 보호 없음)

```c
#include <zephyr/kernel.h>

volatile int32_t shared_counter = 0;
volatile int32_t isr_increment_count = 0;
#define ITERATIONS 300000

void timer_expiry(struct k_timer *timer_id) {
    shared_counter++;        // unprotected - runs in ISR context (see lab 14)
    isr_increment_count++;   // only the ISR touches this one, so it's safe on its own
}

K_TIMER_DEFINE(counter_timer, timer_expiry, NULL);

void counting_thread_entry(void *p1, void *p2, void *p3) {
    for (int i = 0; i < ITERATIONS; i++) {
        shared_counter++;    // unprotected - races with the ISR above
    }
    k_timer_stop(&counter_timer);
    k_sleep(K_MSEC(10));     // let any in-flight ISR settle

    int32_t expected = ITERATIONS + isr_increment_count;
    printk("Expected: %d, Actual: %d\n", expected, shared_counter);
}

K_THREAD_DEFINE(counting_id, 1024, counting_thread_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    k_timer_start(&counter_timer, K_USEC(50), K_USEC(50));
    return 0;
}
```

### 실행 & 확인

- `Expected`와 `Actual` 값을 비교해보세요 — 대부분 **Actual이 더 작게** 나옵니다. 스레드가 `shared_counter`를 읽고 +1해서 다시 쓰는 도중에 타이머 ISR이 끼어들어 같은 변수를 건드리면, 한쪽의 결과가 사라집니다

## 코드 (해결 — irq_lock으로 보호)

```c
void timer_expiry(struct k_timer *timer_id) {
    unsigned int key = irq_lock();
    shared_counter++;
    irq_unlock(key);
    isr_increment_count++;
}

void counting_thread_entry(void *p1, void *p2, void *p3) {
    for (int i = 0; i < ITERATIONS; i++) {
        unsigned int key = irq_lock();
        shared_counter++;
        irq_unlock(key);
    }
    k_timer_stop(&counter_timer);
    k_sleep(K_MSEC(10));

    int32_t expected = ITERATIONS + isr_increment_count;
    printk("Expected: %d, Actual: %d\n", expected, shared_counter);
}
```

### 실행 & 확인

- 이번엔 `Expected`와 `Actual`이 정확히 일치하는지 확인 — `irq_lock()`이 그 구간 동안 인터럽트 자체를 잠깐 막아서, ISR이 끼어들 수 없게 만들었기 때문입니다

## 관찰 포인트

- **`k_sched_lock()`은 이 문제를 해결하지 못합니다** — `k_sched_lock`은 "다른 스레드에게 선점당하지 않기"만 보장할 뿐, **인터럽트(ISR)는 여전히 발생**합니다. 지금처럼 상대가 ISR이라면 반드시 `irq_lock()`(또는 그 이상)을 써야 합니다. 이 차이를 직접 코드로 바꿔서 (`irq_lock`→`k_sched_lock`) 실험해보면, 여전히 `Actual`이 어긋나는 걸 확인할 수 있습니다
- `irq_lock()`은 매우 강력한(무거운) 도구입니다 — 호출 구간 동안 **모든 인터럽트**가 막히므로, 짧게 유지해야 시스템 반응성에 영향을 덜 줍니다
- 이식성을 고려한 최신 Zephyr 코드에서는 `irq_lock` 대신 **`struct k_spinlock` + `k_spin_lock`/`k_spin_unlock`**을 권장합니다 — SMP(진짜 멀티코어 대칭) 환경까지 고려한 API라, 단일 코어에서는 `irq_lock`과 동등하게 동작하고 SMP 환경에서는 자동으로 스핀락까지 추가로 처리해줍니다:
  ```c
  struct k_spinlock lock;

  k_spinlock_key_t key = k_spin_lock(&lock);
  shared_counter++;
  k_spin_unlock(&lock, key);
  ```

## 다음

18번 파일(`18_MULTICORE_REALITY_LAB.md`)에서 SR110의 M55+M4 이종 코어에서 Zephyr의 멀티코어가 실제로 어떻게 동작하는지 다룹니다.
