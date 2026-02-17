#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(sdvx_pin_logger, LOG_LEVEL_INF);

struct watched_pin {
    const struct device *port;
    gpio_pin_t pin;
    const char *name;
    int last;
};

/*
 * XIAO BLE D2..D8 on nRF52840:
 * D2=P0.28, D3=P0.29, D4=P0.04, D5=P0.05, D6=P1.11, D7=P1.12, D8=P1.13
 */
static const struct device *const gpio0_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));
#if DT_NODE_EXISTS(DT_NODELABEL(gpio1))
static const struct device *const gpio1_dev = DEVICE_DT_GET(DT_NODELABEL(gpio1));
#endif

static struct watched_pin pins[] = {
    { .port = NULL, .pin = 28, .name = "D2", .last = 1 },
    { .port = NULL, .pin = 29, .name = "D3", .last = 1 },
    { .port = NULL, .pin = 4, .name = "D4", .last = 1 },
    { .port = NULL, .pin = 5, .name = "D5", .last = 1 },
#if DT_NODE_EXISTS(DT_NODELABEL(gpio1))
    { .port = NULL, .pin = 11, .name = "D6", .last = 1 },
    { .port = NULL, .pin = 12, .name = "D7", .last = 1 },
    { .port = NULL, .pin = 13, .name = "D8", .last = 1 },
#endif
};

static void sdvx_pin_logger_thread(void *a, void *b, void *c)
{
    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);

    size_t i;

    if (!device_is_ready(gpio0_dev)) {
        LOG_ERR("gpio0 not ready");
        return;
    }

#if DT_NODE_EXISTS(DT_NODELABEL(gpio1))
    if (!device_is_ready(gpio1_dev)) {
        LOG_ERR("gpio1 not ready");
        return;
    }
#endif

    for (i = 0; i < ARRAY_SIZE(pins); i++) {
        if (i < 4) {
            pins[i].port = gpio0_dev;
        } else {
#if DT_NODE_EXISTS(DT_NODELABEL(gpio1))
            pins[i].port = gpio1_dev;
#endif
        }

        gpio_pin_configure(pins[i].port, pins[i].pin, GPIO_INPUT | GPIO_PULL_UP);
        pins[i].last = gpio_pin_get(pins[i].port, pins[i].pin);
    }

    LOG_INF("sdvx pin logger started");

    while (1) {
        for (i = 0; i < ARRAY_SIZE(pins); i++) {
            int v = gpio_pin_get(pins[i].port, pins[i].pin);
            if (v < 0) {
                continue;
            }

            if ((pins[i].last != 0) && (v == 0)) {
                LOG_INF("%s LOW", pins[i].name);
            }

            pins[i].last = v;
        }

        k_msleep(20);
    }
}

K_THREAD_DEFINE(sdvx_pin_logger_tid, 1024, sdvx_pin_logger_thread, NULL, NULL, NULL, 7, 0, 0);
