# 12. Poll Signal — 경량 이벤트

## 이 실습에서 배우는 것

값 하나를 가볍게 전달하고 싶을 때 쓰는 Zephyr 기능이 **Poll Signal**입니다. 별도의 Queue/Semaphore 객체 없이, `k_poll`과 짝을 이루어 "값이 있는 가벼운 1회성 이벤트"를 전달합니다.

## 사전 설정

Poll Signal은 `k_poll` 기능의 일부라 마찬가지로 기본적으로 꺼져 있습니다. `prj.conf`에 아래 옵션이 필요합니다.

```
CONFIG_POLL=y
```

이 옵션 하나가 `k_poll()`과 `k_poll_signal_raise()` API를 함께 활성화합니다 — 11번 실습에서 겪은 것과 똑같이, 빠뜨리면 링크 단계에서 `undefined reference to 'z_impl_k_poll_signal_raise'` 같은 에러가 납니다.

## 핵심 개념

| 함수/매크로 | 설명 |
|---|---|
| `K_POLL_SIGNAL_INITIALIZER(obj)` | Poll Signal을 컴파일 시점에 정적으로 초기화하는 매크로 |
| `k_poll_signal_raise(&signal, 결과값)` | 신호 발생 + 정수 결과값 전달 |
| `k_poll_signal_check(&signal, &signaled, &result)` | 신호가 발생했는지, 그 결과값은 무엇인지 확인 |
| `k_poll_signal_reset(&signal)` | 다음 사용을 위해 신호를 초기 상태로 리셋 |

## 코드

```c
#include <zephyr/kernel.h>

struct k_poll_signal my_signal = K_POLL_SIGNAL_INITIALIZER(my_signal);

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

- `K_POLL_SIGNAL_INITIALIZER(obj)`는 **컴파일 시점 초기화 매크로**입니다 — 별도로 `k_poll_signal_init()`을 런타임에 호출할 필요가 없습니다. `producer_entry`/`consumer_entry`는 `K_THREAD_DEFINE`으로 `main()` 실행 전부터 이미 동작하기 시작하는데, 만약 `my_signal`을 런타임에 초기화하는 방식이었다면 "초기화되기 전에 스레드가 먼저 접근하는" 경쟁 상태가 생길 수 있습니다 — 컴파일 시점 초기화로 이 문제를 원천 차단합니다
- Poll Signal 하나만 놓고 보면 Semaphore와 큰 차이가 없어 보일 수 있지만, 진짜 가치는 **11번 실습의 `k_poll`과 결합했을 때** 드러납니다 — Message Queue, Semaphore, Poll Signal을 전부 같은 `k_poll` 배열에 섞어 넣고 "이 중 아무거나 오면 처리"하는 통합된 대기 지점을 만들 수 있습니다
- `k_poll_signal_raise`는 ISR에서도 호출 가능합니다 — 07번 실습의 `k_sem_give`처럼, 인터럽트에서 스레드로 값을 함께 전달하고 싶을 때 쓸 수 있습니다
- 재사용하려면 `k_poll_signal_reset`을 반드시 호출해야 합니다 — 리셋하지 않으면 다음 `k_poll()` 호출이 "이미 신호가 와 있다"고 착각해 곧바로 반환되어 버립니다

## 트러블슈팅

| 증상 | 원인 / 해결 |
|---|---|
| `undefined reference to 'z_impl_k_poll_signal_raise'` 또는 `z_impl_k_poll` | `prj.conf`에 `CONFIG_POLL=y`가 빠짐 — 위 "사전 설정" 참고 |
| `error: type defaults to 'int' in declaration of 'K_POLL_SIGNAL_DEFINE'` 등 컴파일 에러 | `K_POLL_SIGNAL_DEFINE`은 존재하지 않는 매크로입니다 — `struct k_poll_signal my_signal = K_POLL_SIGNAL_INITIALIZER(my_signal);` 형태로 작성해야 합니다 (위 코드 참고) |

## 다음

13번 파일(`13_K_EVENT_LAB.md`)에서 여러 조건을 비트로 관리하는 `k_event`를 다룹니다.
