# 15. 스택 사용량 모니터링

## 이 실습에서 배우는 것

각 스레드의 스택 여유분을 실행 중에 확인할 수 있는 Zephyr API는 `k_thread_stack_space_get`입니다. 단위는 **바이트**입니다.

## 사전 설정

`prj.conf`에 아래 옵션이 필요합니다.

```
CONFIG_THREAD_STACK_INFO=y
```

## 핵심 개념

```c
int k_thread_stack_space_get(const struct k_thread *thread, size_t *unused_ptr);
```

- 성공 시 `unused_ptr`에 **여유 바이트 수**가 채워짐 (작을수록 위험)

## 코드

```c
#include <zephyr/kernel.h>
#include <string.h>

#define STACK_SIZE 2048

void light_entry(void *p1, void *p2, void *p3) {
    while (1) {
        int small_var = 0;
        small_var++;
        k_sleep(K_MSEC(1000));
    }
}

void recursive_work(int depth) {
    char buffer[256];   // consumes stack on every call
    memset(buffer, 0, sizeof(buffer));
    if (depth > 0) {
        recursive_work(depth - 1);
    }
}

void heavy_entry(void *p1, void *p2, void *p3) {
    while (1) {
        recursive_work(4);
        k_sleep(K_MSEC(1000));
    }
}

K_THREAD_DEFINE(light_id, STACK_SIZE, light_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(heavy_id, STACK_SIZE, heavy_entry, NULL, NULL, NULL, 5, 0, 0);

void monitor_entry(void *p1, void *p2, void *p3) {
    size_t light_unused, heavy_unused;
    while (1) {
        k_sleep(K_MSEC(2000));
        k_thread_stack_space_get(light_id, &light_unused);
        k_thread_stack_space_get(heavy_id, &heavy_unused);
        printk("Stack headroom (bytes) - LightThread: %u, HeavyThread: %u\n",
               (unsigned int)light_unused, (unsigned int)heavy_unused);
    }
}

K_THREAD_DEFINE(monitor_id, 1024, monitor_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
```

## 실행 & 확인

- `HeavyThread`(재귀 호출 + 256바이트 버퍼)의 여유분이 `LightThread`보다 눈에 띄게 작게 나오는지 확인

## 관찰 포인트

- `HeavyThread`의 `STACK_SIZE`를 `2048`에서 `512`로 줄여보세요 — 여유분이 0에 가까워지거나, Zephyr의 내장 스택 보호 기능(하드웨어 MPU 가드 또는 canary, 보드/설정에 따라 다름)에 의해 `Stack overflow` 계열의 fatal error가 발생할 수 있습니다 (확인 후 다시 2048로 복구하세요)
- Zephyr는 스택 크기를 **바이트 단위**로 지정한다는 점을 항상 기억하세요 — 다른 임베디드 플랫폼 경험이 있다면 word 단위와 헷갈리기 쉬운 부분입니다
- `CONFIG_THREAD_RUNTIME_STACK_SAFETY`(고급 옵션)를 쓰면, 스택 여유분이 특정 임계값 아래로 떨어지는 "그 순간"을 감지해 콜백을 실행하는 능동적인 모니터링도 가능합니다 — 지금 실습처럼 주기적으로 폴링하는 방식보다 더 즉각적입니다

## 다음

16번 파일(`16_DEADLOCK_LAB.md`)에서 여러 Mutex를 쓸 때 발생할 수 있는 Deadlock을 다룹니다.
