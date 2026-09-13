#include "drivers/tick_timer.h"

#include <stdbool.h>

#include "driver/gptimer.h"
#include "esp_attr.h"

#include "config/app_config.h"

static gptimer_handle_t s_timer;
static TaskHandle_t     s_task;     /* кого будить ISR */

/*
 * GPTimer: лише будить задачу, жодної логіки.
 * Сповіщення замість прапорця: задача спить у ulTaskNotifyTake(), тож
 * IDLE отримує процесорний час і watchdog задоволений.
 */
static bool IRAM_ATTR on_tick_alarm(gptimer_handle_t timer,
                                    const gptimer_alarm_event_data_t *edata,
                                    void *user_ctx)
{
    (void)timer;
    (void)edata;
    (void)user_ctx;

    BaseType_t woken = pdFALSE;
    vTaskNotifyGiveFromISR(s_task, &woken);
    return woken == pdTRUE;     /* перемкнути контекст одразу після ISR */
}

void tick_timer_init(TaskHandle_t task)
{
    s_task = task;

    gptimer_config_t config = {
        .clk_src       = GPTIMER_CLK_SRC_DEFAULT,
        .direction     = GPTIMER_COUNT_UP,
        .resolution_hz = TIMER_RESOLUTION_HZ,
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&config, &s_timer));

    gptimer_event_callbacks_t cbs = {.on_alarm = on_tick_alarm};
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(s_timer, &cbs, NULL));

    gptimer_alarm_config_t alarm = {
        .reload_count = 0,
        .alarm_count  = TICK_PERIOD_US,
        .flags.auto_reload_on_alarm = true,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(s_timer, &alarm));
    ESP_ERROR_CHECK(gptimer_enable(s_timer));
    ESP_ERROR_CHECK(gptimer_start(s_timer));
}

void tick_timer_wait(void)
{
    /*
     * Не vTaskDelay(pdMS_TO_TICKS(1)): при CONFIG_FREERTOS_HZ=100 це 0 тіків,
     * задача не засинає і "душить" IDLE0 -> task watchdog.
     */
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
}
