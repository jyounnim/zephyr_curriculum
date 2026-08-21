// Source: 10_MSGQ_BASICS_LAB.md

#include <zephyr/kernel.h>

struct sensor_data {
    int id;
    int value_x10;   // fixed-point: real value * 10 (e.g. 205 means 20.5)
};

K_MSGQ_DEFINE(data_msgq, sizeof(struct sensor_data), 5, 4);   // 5 slots

void sensor_entry(void *p1, void *p2, void *p3) {
    int counter = 0;
    while (1) {
        struct sensor_data data;
        data.id = counter++;
        data.value_x10 = 200 + (counter % 10) * 5;

        if (k_msgq_put(&data_msgq, &data, K_MSEC(100)) == 0) {
            printk("SensorThread: sent id=%d value=%d.%d\n",
                   data.id, data.value_x10 / 10, data.value_x10 % 10);
        } else {
            printk("SensorThread: queue full, send failed\n");
        }
        k_sleep(K_MSEC(500));
    }
}

void process_entry(void *p1, void *p2, void *p3) {
    struct sensor_data received;
    while (1) {
        k_msgq_get(&data_msgq, &received, K_FOREVER);
        printk("ProcessThread: received id=%d value=%d.%d\n",
               received.id, received.value_x10 / 10, received.value_x10 % 10);
    }
}

K_THREAD_DEFINE(sensor_id, 1024, sensor_entry, NULL, NULL, NULL, 5, 0, 0);
K_THREAD_DEFINE(process_id, 1024, process_entry, NULL, NULL, NULL, 5, 0, 0);

int main(void) {
    return 0;
}
