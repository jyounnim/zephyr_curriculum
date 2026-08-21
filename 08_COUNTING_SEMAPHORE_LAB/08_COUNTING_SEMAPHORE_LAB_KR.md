# 08. Counting Semaphore — 자원 풀 관리

## 이 실습에서 배우는 것

Zephyr의 `k_sem`은 초기값과 최댓값을 다르게 주면 그대로 Counting Semaphore가 됩니다. 별도의 다른 자료형이 필요 없습니다 — Binary Semaphore(count 1)와 Counting Semaphore는 `K_SEM_DEFINE`에 넘기는 숫자만 다를 뿐, 완전히 같은 API를 씁니다.

## 핵심 개념

```c
K_SEM_DEFINE(sem, 초기값, 최댓값);
```

- `초기값`: 시작 시점에 사용 가능한 자원 개수
- `최댓값`: 세마포어가 가질 수 있는 최댓값(=동시 허용 개수)

## 코드

```c
#include <zephyr/kernel.h>

#define STACK_SIZE 1024
#define POOL_SIZE 2
#define WORKER_COUNT 4

K_SEM_DEFINE(resource_pool, POOL_SIZE, POOL_SIZE);   // 2 slots, both available at start

void worker_entry(void *p1, void *p2, void *p3) {
    int id = (int)(intptr_t)p1;
    while (1) {
        printk("Worker%d: waiting for a free slot\n", id);
        k_sem_take(&resource_pool, K_FOREVER);
        printk("Worker%d: acquired a slot, using resource...\n", id);
        k_sleep(K_MSEC(2000));   // simulate using the resource
        printk("Worker%d: releasing the slot\n", id);
        k_sem_give(&resource_pool);
        k_sleep(K_MSEC(500));
    }
}

K_THREAD_STACK_ARRAY_DEFINE(worker_stacks, WORKER_COUNT, STACK_SIZE);
static struct k_thread worker_threads[WORKER_COUNT];

int main(void) {
    for (int i = 0; i < WORKER_COUNT; i++) {
        k_thread_create(
            &worker_threads[i], worker_stacks[i], STACK_SIZE,
            worker_entry, (void *)(intptr_t)(i + 1), NULL, NULL,
            5, 0, K_NO_WAIT
        );
    }
    return 0;
}
```

## 실행 & 확인

- Worker 4개가 동시에 자원을 요청하지만, 한 번에 **2개까지만** "acquired a slot"이 되는지 확인
- 나머지 2개는 "waiting for a free slot"에 머물다가, 자리가 나는 즉시 자원을 획득하는지 확인

## 관찰 포인트

- `POOL_SIZE`를 1로 바꾸면 사실상 상호배제(mutual exclusion)와 동일하게 동작합니다 — 다만 09번에서 배울 `k_mutex`와 달리 소유자 개념과 Priority Inheritance는 없습니다
- 이번 실습에서는 워커 스레드를 `k_thread_create`로 반복문 안에서 동적으로 만들었습니다 — `K_THREAD_STACK_ARRAY_DEFINE`으로 스택 배열을 한 번에 선언해두면, 이렇게 개수가 정해진 여러 스레드를 깔끔하게 반복 생성할 수 있습니다

## 다음

09번 파일(`09_MUTEX_LAB.md`)에서 Mutex와 Priority Inheritance를 다룹니다.
