/*
 * EMB_18 Module 2 Project - Світлофор за ПДР України (ESP32-S3, ESP-IDF v6.1)
 *
 * Обладнання:
 *   GPIO15 -> зелений LED  (+ ~220 Ом -> GND)
 *   GPIO16 -> жовтий  LED  (+ ~220 Ом -> GND)
 *   GPIO17 -> червоний LED (+ ~220 Ом -> GND)
 *   GPIO18 -> кнопка пішохода -> GND (внутрішня підтяжка, переривання по спаду)
 *   GPIO4  -> дільник з фоторезистором (ADC1_CH3)
 *
 * Структура:
 *   config/app_config.h    піни, тривалості фаз, пороги
 *   util/time_utils.h      millis_u32(), elapsed_ms()
 *   drivers/tick_timer     апаратний такт 5 мс (GPTimer -> task notify)
 *   drivers/traffic_leds   три лампи
 *   drivers/button         кнопка: IRQ + неблокуючий debounce
 *   drivers/light_sensor   ADC, день/ніч з гістерезисом
 *   app/traffic_light      кінцевий автомат ПДР
 *   main.c                 ініціалізація і суперлуп, що з'єднує модулі
 *
 * Драйвери нічого не знають про світлофор: кнопка лише повідомляє про натиск,
 * датчик - про зміну день/ніч. Реакцію на події визначає цей файл.
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app/traffic_light.h"
#include "config/app_config.h"
#include "drivers/button.h"
#include "drivers/light_sensor.h"
#include "drivers/tick_timer.h"
#include "drivers/traffic_leds.h"
#include "util/time_utils.h"

static const char *TAG = "main";

static void log_status(void)
{
    static uint32_t last_ms;

    if (elapsed_ms(last_ms) < STATUS_LOG_MS) {
        return;
    }
    last_ms = millis_u32();

    ESP_LOGI(TAG, "стан=%s t=%lu мс | adc=%d світло=%d (%s) | запит пішохода=%s | натисків=%lu",
             traffic_light_state_name(),
             (unsigned long)traffic_light_ms_in_state(),
             light_sensor_adc(),
             light_sensor_level(),
             light_sensor_is_night() ? "ніч" : "день",
             traffic_light_ped_pending() ? "так" : "ні",
             (unsigned long)button_press_count());
}

void app_main(void)
{
    traffic_leds_init();
    button_init();
    light_sensor_init();

    ESP_LOGI(TAG, "=== Світлофор ПДР: G=GPIO%d Y=GPIO%d R=GPIO%d BTN=GPIO%d LDR=GPIO%d ===",
             (int)LED_GREEN_GPIO, (int)LED_YELLOW_GPIO, (int)LED_RED_GPIO,
             (int)BTN_PED_GPIO, LIGHT_SENSOR_GPIO);

    traffic_light_start();
    tick_timer_init(xTaskGetCurrentTaskHandle());

    while (1) {
        tick_timer_wait();      /* спимо до такту 5 мс */

        if (button_poll()) {
            traffic_light_request_pedestrian();
        }

        traffic_light_update();

        switch (light_sensor_poll()) {
        case LIGHT_EVENT_NIGHT:
            traffic_light_set_night(true);
            break;
        case LIGHT_EVENT_DAY:
            traffic_light_set_night(false);
            break;
        default:
            break;
        }

        log_status();
    }
}
