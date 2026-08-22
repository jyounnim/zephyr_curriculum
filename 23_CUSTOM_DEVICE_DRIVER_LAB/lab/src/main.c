#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

int main(void) {
    const struct device *aht20 = DEVICE_DT_GET_ANY(zds_aht20);

    if (aht20 == NULL || !device_is_ready(aht20)) {
        printk("AHT20 device not ready\n");
        return 0;
    }

    while (1) {
        struct sensor_value temp, humidity;

        sensor_sample_fetch(aht20);
        sensor_channel_get(aht20, SENSOR_CHAN_AMBIENT_TEMP, &temp);
        sensor_channel_get(aht20, SENSOR_CHAN_HUMIDITY, &humidity);

        printk("Temperature: %d.%03d C, Humidity: %d.%03d %%\n",
               temp.val1, temp.val2 / 1000,
               humidity.val1, humidity.val2 / 1000);

        k_sleep(K_SECONDS(2));
    }
    return 0;
}
