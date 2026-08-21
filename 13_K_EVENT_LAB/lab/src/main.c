// Source: 13_K_EVENT_LAB.md

#include <zephyr/kernel.h>

#define WIFI_READY_BIT     BIT(0)
#define SENSOR_READY_BIT   BIT(1)
#define STORAGE_READY_BIT  BIT(2)
#define ALL_READY_BITS     (WIFI_READY_BIT | SENSOR_READY_BIT | STORAGE_READY_BIT)

K_EVENT_DEFINE(system_events);

void wifi_init_entry(void *p1, void *p2, void *p3) {
    printk("WifiInitThread: initializing...\n");
    k_sleep(K_MSEC(1500));
    printk("WifiInitThread: ready\n");
    k_event_post(&system_events, WIFI_READY_BIT);
}

void sensor_init_entry(void *p1, void *p2, void *p3) {
    printk("SensorInitThread: initializing...\n");
    k_sleep(K_MSEC(800));
    printk("SensorInitThread: ready\n");
    k_event_post(&system_events, SENSOR_READY_BIT);
}

void storage_init_entry(void *p1, void *p2, void *p3) {
    printk("StorageInitThread: initializing...\n");
    k_sleep(K_MSEC(2200));
    printk("StorageInitThread: ready\n");
    k_event_post(&system_events, STORAGE_READY_BIT);
}

void main_app_entry(void *p1, void *p2, void *p3) {
    printk("MainThread: waiting for all subsystems...\n");
    k_event_wait_all(&system_events, ALL_READY_BITS, false, K_FOREVER);
    printk("MainThread: all subsystems ready! starting main application...\n");
    while (1) {
        k_sleep(K_MSEC(5000));
        printk("MainThread: running normally\n");
    }
}

K_THREAD_DEFINE(main_app_id, 1024, main_app_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(wifi_id, 1024, wifi_init_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(sensor_id, 1024, sensor_init_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(storage_id, 1024, storage_init_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
