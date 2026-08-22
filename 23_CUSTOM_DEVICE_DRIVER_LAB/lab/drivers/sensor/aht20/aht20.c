#define DT_DRV_COMPAT zds_aht20

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(aht20, CONFIG_SENSOR_LOG_LEVEL);

struct aht20_config {
    struct i2c_dt_spec i2c;
};

struct aht20_data {
    uint32_t humidity_raw;
    uint32_t temp_raw;
};

static int aht20_sample_fetch(const struct device *dev, enum sensor_channel chan) {
    const struct aht20_config *config = dev->config;
    struct aht20_data *data = dev->data;
    uint8_t trigger_cmd[3] = {0xAC, 0x33, 0x00};   // AHT20 measurement trigger command (per datasheet)
    uint8_t buf[6];
    int ret;

    ret = i2c_write_dt(&config->i2c, trigger_cmd, sizeof(trigger_cmd));
    if (ret < 0) {
        LOG_ERR("Failed to trigger measurement: %d", ret);
        return ret;
    }

    k_sleep(K_MSEC(80));   // wait for the conversion to complete (datasheet recommends 80ms)

    ret = i2c_read_dt(&config->i2c, buf, sizeof(buf));
    if (ret < 0) {
        LOG_ERR("Failed to read sensor data: %d", ret);
        return ret;
    }

    // buf[0] is a status byte; buf[1..5] hold a 20-bit humidity value + a 20-bit temperature value
    data->humidity_raw = ((uint32_t)buf[1] << 12) | ((uint32_t)buf[2] << 4) | (buf[3] >> 4);
    data->temp_raw = (((uint32_t)buf[3] & 0x0F) << 16) | ((uint32_t)buf[4] << 8) | buf[5];

    return 0;
}

static int aht20_channel_get(const struct device *dev, enum sensor_channel chan,
                              struct sensor_value *val) {
    struct aht20_data *data = dev->data;

    if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
        // temp(C) = raw / 2^20 * 200 - 50   (computed in milli-degree units, then split into integer/fractional parts)
        int64_t milli_c = ((int64_t)data->temp_raw * 200000 / 1048576) - 50000;
        val->val1 = (int32_t)(milli_c / 1000);
        val->val2 = (int32_t)((milli_c % 1000) * 1000);
    } else if (chan == SENSOR_CHAN_HUMIDITY) {
        // humidity(%) = raw / 2^20 * 100
        int64_t milli_pct = (int64_t)data->humidity_raw * 100000 / 1048576;
        val->val1 = (int32_t)(milli_pct / 1000);
        val->val2 = (int32_t)((milli_pct % 1000) * 1000);
    } else {
        return -ENOTSUP;
    }
    return 0;
}

static const struct sensor_driver_api aht20_api = {
    .sample_fetch = aht20_sample_fetch,
    .channel_get = aht20_channel_get,
};

static int aht20_init(const struct device *dev) {
    const struct aht20_config *config = dev->config;

    if (!device_is_ready(config->i2c.bus)) {
        LOG_ERR("I2C bus not ready");
        return -ENODEV;
    }

    k_sleep(K_MSEC(40));   // stabilization delay after power-on (per datasheet)
    return 0;
}

#define AHT20_INIT(inst)                                                  \
    static struct aht20_data aht20_data_##inst;                          \
    static const struct aht20_config aht20_config_##inst = {             \
        .i2c = I2C_DT_SPEC_INST_GET(inst),                                \
    };                                                                    \
    SENSOR_DEVICE_DT_INST_DEFINE(inst, aht20_init, NULL,                  \
                  &aht20_data_##inst, &aht20_config_##inst,               \
                  POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,                \
                  &aht20_api);

DT_INST_FOREACH_STATUS_OKAY(AHT20_INIT)
