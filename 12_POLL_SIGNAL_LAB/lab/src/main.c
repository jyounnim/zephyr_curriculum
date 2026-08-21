// Source: 12_POLL_SIGNAL_LAB.md

#include <zephyr/kernel.h>

K_POLL_SIGNAL_DEFINE(my_signal);

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
