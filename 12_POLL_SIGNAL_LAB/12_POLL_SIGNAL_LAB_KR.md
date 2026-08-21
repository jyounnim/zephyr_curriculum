# 12. Poll Signal — 경량 이벤트

## 이 실습에서 배우는 것

값 하나를 가볍게 전달하고 싶을 때 쓰는 Zephyr 기능이 **Poll Signal**입니다. 별도의 Queue/Semaphore 객체 없이, `k_poll`과 짝을 이루어 "값이 있는 가벼운 1회성 이벤트"를 전달합니다.

## 핵심 개념

| 함수/매크로 | 설명 |
|---|---|
| `K_POLL_SIGNAL_DEFINE(이름)` | Poll Signal을 컴파일 시점에 정적으로 정의 |
| `k_poll_signal_raise(&signal, 결과값)` | 신호 발생 + 정수 결과값 전달 |
| `k_poll_signal_check(&signal, &signaled, &result)` | 신호가 발생했는지, 그 결과값은 무엇인지 확인 |
| `k_poll_signal_reset(&signal)` | 다음 사용을 위해 신호를 초기 상태로 리셋 |

## 코드

```c
#include <zephyr/kernel.h>

K_POLL_SIGNAL_DEFINE(my_signal);

void producer_entry(void *p1, void *p2, void *p3) {
    int counter = 0;
    while (1) {
        counter++;
        k_sleep(K_MSEC(1000));
        printk("ProducerThread: raising signal with value=%d\n", counter);
        k_poll_signal_raise(&my_signal, counter);
    }
}

void consumer_entry(void *p1, void *p2, void *p3) {
    struct k_poll_event event = K_POLL_EVENT_INITIALIZER(
        K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &my_signal);

    while (1) {
        k_poll(&event, 1, K_FOREVER);

        unsigned int signaled;
        int result;
        k_poll_signal_check(&my_signal, &signaled, &result);
        printk("ConsumerThread: received value=%d\n", result);

        k_poll_signal_reset(&my_signal);
        event.state = K_POLL_STATE_NOT_READY;
    }
}

K_THREAD_DEFINE(consumer_id, 1024, consumer_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(producer_id, 1024, producer_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
```

## 실행 & 확인

- `ProducerThread`가 1초마다 보낸 `counter` 값을 `ConsumerThread`가 그대로 받는지 확인

## 관찰 포인트

- Poll Signal 하나만 놓고 보면 Semaphore와 큰 차이가 없어 보일 수 있지만, 진짜 가치는 **11번 실습의 `k_poll`과 결합했을 때** 드러납니다 — Message Queue, Semaphore, Poll Signal을 전부 같은 `k_poll` 배열에 섞어 넣고 "이 중 아무거나 오면 처리"하는 통합된 대기 지점을 만들 수 있습니다
- `k_poll_signal_raise`는 ISR에서도 호출 가능합니다 — 07번 실습의 `k_sem_give`처럼, 인터럽트에서 스레드로 값을 함께 전달하고 싶을 때 쓸 수 있습니다
- 재사용하려면 `k_poll_signal_reset`을 반드시 호출해야 합니다 — 리셋하지 않으면 다음 `k_poll()` 호출이 "이미 신호가 와 있다"고 착각해 곧바로 반환되어 버립니다

## 다음

13번 파일(`13_K_EVENT_LAB.md`)에서 여러 조건을 비트로 관리하는 `k_event`를 다룹니다.
