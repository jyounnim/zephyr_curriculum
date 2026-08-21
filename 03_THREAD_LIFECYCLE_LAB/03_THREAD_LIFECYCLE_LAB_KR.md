# 03. Thread 동적 생성/종료

## 이 실습에서 배우는 것

`K_THREAD_DEFINE`은 스레드 개수가 고정적일 때 편하지만, 실행 중에 필요할 때마다 만들고 없애야 하는 상황(작업 큐 워커 등)에는 `k_thread_create`를 씁니다. 다만 Zephyr는 **스레드가 쓸 스택 메모리를 미리 별도로 선언**해야 한다는 특징이 있습니다.

## 핵심 개념

| 함수/매크로 | 설명 |
|---|---|
| `K_THREAD_STACK_DEFINE(이름, 크기)` | 스레드가 쓸 스택 메모리 공간을 정적으로 확보 (동적 생성이라도 스택 자체는 미리 마련) |
| `k_thread_create(&data, stack, 크기, 함수, p1, p2, p3, 우선순위, 옵션, 시작지연)` | 런타임에 스레드 생성, 반환값은 `k_tid_t`(스레드 ID) |
| 스레드 함수가 `return`하는 것 | 그 자체로 스레드 종료 |
| `k_thread_abort(tid)` | 다른 스레드를 외부에서 강제 종료 |

## 코드

```c
#include <zephyr/kernel.h>

#define STACK_SIZE 1024
#define WORKER_PRIORITY 5

K_THREAD_STACK_DEFINE(worker_stack, STACK_SIZE);
static struct k_thread worker_thread_data;
static k_tid_t worker_tid = NULL;

void worker_entry(void *p1, void *p2, void *p3) {
    int job_id = (int)(intptr_t)p1;
    printk("WorkerThread #%d: started\n", job_id);

    for (int i = 0; i < 5; i++) {
        printk("WorkerThread #%d: working... (%d/5)\n", job_id, i + 1);
        k_sleep(K_MSEC(500));
    }

    printk("WorkerThread #%d: done, exiting\n", job_id);
    worker_tid = NULL;
    // returning here ends the thread - no explicit "delete self" call needed
}

void manager_entry(void *p1, void *p2, void *p3) {
    int job_counter = 0;
    while (1) {
        if (worker_tid == NULL) {
            job_counter++;
            printk("ManagerThread: spawning WorkerThread #%d\n", job_counter);
            worker_tid = k_thread_create(
                &worker_thread_data, worker_stack, STACK_SIZE,
                worker_entry, (void *)(intptr_t)job_counter, NULL, NULL,
                WORKER_PRIORITY, 0, K_NO_WAIT
            );
        }
        k_sleep(K_MSEC(1000));
    }
}

K_THREAD_DEFINE(manager_id, 1024, manager_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
```

## 실행 & 확인

- `ManagerThread`가 1초마다 `worker_tid`가 비어있는지 확인하고, 비어있으면 새 `WorkerThread`를 생성하는 패턴이 반복되는지 확인
- `WorkerThread`는 5번 일하고 스스로 함수를 `return`해서 종료 → 다음 주기에 `ManagerThread`가 새 Worker를 또 생성

## 관찰 포인트

- 이번 예제의 `worker_stack`/`worker_thread_data`는 **static(정적)** 변수입니다 — 같은 메모리 공간을 재사용하기 때문에, 이전 Worker가 완전히 종료되기 전에 새 Worker를 만들면 메모리가 겹쳐 위험합니다. 지금 코드는 `worker_tid == NULL` 체크로 이를 방지하고 있습니다. 여러 Worker를 동시에 띄우고 싶다면 스택/구조체를 Worker 개수만큼 각각 따로 선언해야 합니다
- Zephyr는 `K_THREAD_STACK_DEFINE`으로 **스택 공간을 개발자가 직접 준비**하게 합니다 — 이는 Zephyr가 메모리 사용을 더 명시적으로/예측 가능하게 만들려는 설계 철학과 관련이 있습니다 (특히 메모리가 극히 제한된 소형 MCU를 함께 지원하기 위함)
- `k_thread_abort(tid)`로 실행 중인 스레드를 외부에서 강제 종료하는 방법도 있습니다 — 다만 스레드가 Mutex 등 자원을 쥔 상태에서 강제 종료되면 그 자원이 영영 풀리지 않을 수 있으므로, 가능하면 지금처럼 "스스로 끝내는" 방식이 더 안전합니다

## 다음

04번 파일(`04_COOPERATIVE_YIELD_LAB.md`)에서 협조적 스레드가 반드시 양보해야 하는 이유를 더 깊이 다룹니다.
