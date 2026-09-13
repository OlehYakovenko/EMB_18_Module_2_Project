#include "drivers/traffic_leds.h"

#include <stddef.h>

#include "driver/gpio.h"

#include "config/app_config.h"

void traffic_leds_init(void)
{
    const gpio_num_t pins[] = {LED_RED_GPIO, LED_YELLOW_GPIO, LED_GREEN_GPIO};

    for (size_t i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        gpio_reset_pin(pins[i]);
        gpio_set_direction(pins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(pins[i], 0);
    }
}

void traffic_leds_set(bool red, bool yellow, bool green)
{
    gpio_set_level(LED_RED_GPIO, red ? 1 : 0);
    gpio_set_level(LED_YELLOW_GPIO, yellow ? 1 : 0);
    gpio_set_level(LED_GREEN_GPIO, green ? 1 : 0);
}

void traffic_leds_set_green(bool on)
{
    gpio_set_level(LED_GREEN_GPIO, on ? 1 : 0);
}

void traffic_leds_set_yellow(bool on)
{
    gpio_set_level(LED_YELLOW_GPIO, on ? 1 : 0);
}
