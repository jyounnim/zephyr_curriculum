// Source: 11_K_POLL_LAB.md

#include <zephyr/kernel.h>

K_MSGQ_DEFINE(temp_msgq, sizeof(int), 3, 4);
K_MSGQ_DEFINE(humidity_msgq, sizeof(int), 3, 4);

void temp_sensor_entry(void *p1, void *p2, void *p3) {
    int temp_x10 = 200;
    while (1) {
        temp_x10 += 3;
        k_msgq_put(&temp_msgq, &temp_x10, K_FOREVER);
        k_sleep(K_MSEC(1500));
    }
}

void humidity_sensor_entry(void *p1, void *p2, void *p3) {
    int humidity_x10 = 400;
    while (1) {
        humidity_x10 += 10;
        k_msgq_put(&humidity_msgq, &humidity_x10, K_FOREVER);
        k_sleep(K_MSEC(900));
    }
}

void dispatcher_entry(void *p1, void *p2, void *p3) {
    struct k_poll_event events[2] = {
        K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
                                         K_POLL_MODE_NOTIFY_ONLY, &temp_msgq, 0),
        K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_MSGQ_DATA_AVAILABLE,
                                         K_POLL_MODE_NOTIFY_ONLY, &humidity_msgq, 0),
    };

    while (1) {
        k_poll(events, 2, K_FOREVER);

        if (events[0].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
            int value;
            k_msgq_get(&temp_msgq, &value, K_NO_WAIT);
            printk("DispatcherThread: [TEMP] %d.%d C\n", value / 10, value % 10);
            events[0].state = K_POLL_STATE_NOT_READY;   // reset for the next k_poll call
        }
        if (events[1].state == K_POLL_STATE_MSGQ_DATA_AVAILABLE) {
            int value;
            k_msgq_get(&humidity_msgq, &value, K_NO_WAIT);
            printk("DispatcherThread: [HUMIDITY] %d.%d %%\n", value / 10, value % 10);
            events[1].state = K_POLL_STATE_NOT_READY;
        }
    }
}

K_THREAD_DEFINE(temp_id, 1024, temp_sensor_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(humidity_id, 1024, humidity_sensor_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(dispatcher_id, 1024, dispatcher_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
