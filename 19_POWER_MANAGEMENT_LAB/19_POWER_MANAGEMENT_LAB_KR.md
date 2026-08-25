# 19. Zephyr Power Management (prj.conf)

## 이 실습에서 배우는 것

Zephyr는 **소스에서 직접 빌드하는 생태계**입니다 — `prj.conf`에 옵션 하나만 켜면, 코드를 전혀 바꾸지 않고도 시스템이 자동으로 절전 모드에 들어갑니다.

## 사전 설정

`prj.conf`에 아래 옵션을 추가합니다.

```
CONFIG_PM=y
```

## 핵심 개념

- `CONFIG_PM=y`를 켜면, Zephyr의 Idle Thread가 "지금부터 다음 스레드가 깨어날 때까지 얼마나 남았는지"를 계산해서, **그 시간 동안 안전하게 들어갈 수 있는 가장 깊은 절전 상태를 자동으로 선택**해 진입합니다
- 애플리케이션 코드는 평소처럼 `k_sleep()`을 쓰면 됩니다 — **절전 모드 진입/해제를 위한 별도 API 호출이 필요 없습니다.** 이게 이번 실습의 핵심입니다

## 코드

```c
#include <zephyr/kernel.h>

void periodic_entry(void *p1, void *p2, void *p3) {
    while (1) {
        printk("PeriodicThread: awake, doing some work...\n");
        k_sleep(K_SECONDS(3));   // 그냥 평소처럼 sleep - PM 관련 코드 없음
    }
}

K_THREAD_DEFINE(periodic_id, 1024, periodic_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
```

## 실행 & 확인

- `CONFIG_PM=y` 없이 한 번 실행하고, 있을 때 한 번 더 실행해서 겉보기 로그 출력은 동일한지 확인 — **애플리케이션 동작 자체는 똑같습니다.** 차이는 겉으로 보이지 않고, 그 3초의 대기 구간 동안 실제로 저전력 상태에 들어가 있는지 여부입니다
- 정밀한 소비 전류 측정 장비(전류 프로브 등)가 있다면, `CONFIG_PM=y` 유무에 따라 대기 구간의 소비 전류가 달라지는지 비교해볼 수 있습니다 (이 문서만으로는 확인이 어려우니 참고만 하세요)

## 관찰 포인트

- Zephyr는 "언제, 얼마나 절전 모드에 들어갈지"를 개발자가 직접 결정하는 게 아니라 **커널이 알아서 판단**합니다 — 다음 깨어날 시점(다음 `k_sleep` 만료, 다음 타이머 만료 등)을 스케줄러가 이미 알고 있기 때문에 가능한 설계입니다
- 다만 실제로 어떤 절전 상태까지 지원되는지는 **SoC의 Power Management 드라이버 구현 수준에 달려 있습니다.** SR110 제품 브리프 기준으로는 active(<100mW) / low-power(<10mW) / ULP AON(<100µW) / power-down 4단계 전력 모드가 하드웨어 레벨에서 존재합니다 — 다만 이 4단계가 Zephyr의 `CONFIG_PM` 상태 머신을 통해 이 SDK 빌드에서 그대로 제어되는지, 아니면 일부는 벤더 전용 전력 API로만 접근 가능한지는 별개 문제입니다. TODO/VERIFY: `sr100_rdk/sr100/m55`에서 `CONFIG_PM=y`가 실제로 어느 상태까지 진입시키는지 확인 필요
- 더 세밀한 제어가 필요하면 `pm_state_force()`로 특정 절전 상태를 코드에서 강제할 수도 있고, `pm_device_action_run()`으로 개별 주변장치(센서, 통신 모듈 등)만 따로 끄고 켤 수도 있습니다 — 다만 이번 실습의 핵심은 "아무것도 안 해도 기본적으로 절전이 동작한다"는 자동화 자체입니다

## 다음

20번 파일(`20_RUNTIME_STATS_LAB.md`)에서 Zephyr가 공식 제공하는 스레드별 CPU 사용률 API를 다룹니다.
