#include "drivers/button.h"

#include "driver/gpio.h"
#include "esp_attr.h"
#include "esp_log.h"

#include "config/app_config.h"
#include "util/time_utils.h"

static const char *TAG = "button";

/*
 * Неблокуючий debounce: ISR лише ставить прапорець, а розбір натиску
 * розкладено на такти. Жодного vTaskDelay - інакше утримана кнопка
 * заморозила б світлофор.
 */
typedef enum {
    BTN_IDLE = 0,
    BTN_DEBOUNCE,
    BTN_WAIT_RELEASE,
    BTN_GUARD,
} btn_phase_t;

static volatile bool s_irq_pending;     /* від GPIO ISR */
static btn_phase_t   s_phase;
static uint32_t      s_phase_ms;
static uint32_t      s_press_count;

/* GPIO: лише прапорець; debounce - у задачі */
static void IRAM_ATTR button_isr(void *arg)
{
    (void)arg;
    s_irq_pending = true;
}

void button_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << BTN_PED_GPIO,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,     /* кнопка тягне пін на GND */
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_NEGEDGE,      /* фронт натиску */
    };
    ESP_ERROR_CHECK(gpio_config(&io));
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
    ESP_ERROR_CHECK(gpio_isr_handler_add(BTN_PED_GPIO, button_isr, NULL));
}

bool button_poll(void)
{
    const bool pressed = (gpio_get_level(BTN_PED_GPIO) == 0);
    bool confirmed = false;

    switch (s_phase) {
    case BTN_IDLE:
        if (s_irq_pending) {
            s_irq_pending = false;
            s_phase_ms    = millis_u32();
            s_phase       = BTN_DEBOUNCE;
        }
        break;

    case BTN_DEBOUNCE:
        if (elapsed_ms(s_phase_ms) < DEBOUNCE_MS) {
            break;
        }
        if (!pressed) {
            s_phase = BTN_IDLE;     /* брязкіт або вже відпустили */
            break;
        }
        s_press_count++;
        ESP_LOGI(TAG, "натиск #%lu", (unsigned long)s_press_count);
        confirmed = true;
        s_phase = BTN_WAIT_RELEASE;
        break;

    case BTN_WAIT_RELEASE:
        if (!pressed) {
            s_phase_ms = millis_u32();
            s_phase    = BTN_GUARD;
        }
        break;

    case BTN_GUARD:
        if (elapsed_ms(s_phase_ms) >= RELEASE_GUARD_MS) {
            s_irq_pending = false;  /* відкинути зайві IRQ за час жесту */
            s_phase       = BTN_IDLE;
        }
        break;
    }

    return confirmed;
}

uint32_t button_press_count(void)
{
    return s_press_count;
}
