#include "drivers/light_sensor.h"

#include <stdint.h>

#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#include "config/app_config.h"
#include "util/time_utils.h"

static const char *TAG = "light";

static adc_oneshot_unit_handle_t s_adc;
static adc_channel_t             s_channel;

static bool     s_is_night;
static bool     s_pending_night;        /* кандидат на новий режим */
static uint32_t s_last_poll_ms;
static uint32_t s_change_since_ms;      /* коли датчик почав показувати інше */
static int      s_adc_value;
static int      s_level;

void light_sensor_init(void)
{
    adc_unit_t unit = ADC_UNIT_1;
    ESP_ERROR_CHECK(adc_oneshot_io_to_channel(LIGHT_SENSOR_GPIO, &unit, &s_channel));

    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id  = unit,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc));

    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = ADC_ATTEN_DB_12,            /* повний діапазон ~0..3.1 В */
        .bitwidth = ADC_BITWIDTH_DEFAULT,       /* 12 біт: 0..4095 */
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc, s_channel, &chan_cfg));

    s_last_poll_ms  = millis_u32();
    s_level         = LIGHT_RAW_DAY;    /* стартуємо як "день" */
    s_is_night      = false;
    s_pending_night = false;

    ESP_LOGI(TAG, "датчик світла: GPIO%d -> ADC%d_CH%d",
             LIGHT_SENSOR_GPIO, (int)unit + 1, (int)s_channel);
}

static int read_adc_average(void)
{
    int sum = 0;

    for (int i = 0; i < LIGHT_SAMPLES; i++) {
        int raw = 0;
        if (adc_oneshot_read(s_adc, s_channel, &raw) != ESP_OK) {
            return s_adc_value;     /* збій вимірювання - лишаємо попереднє */
        }
        sum += raw;
    }
    return sum / LIGHT_SAMPLES;
}

light_event_t light_sensor_poll(void)
{
    if (elapsed_ms(s_last_poll_ms) < LIGHT_POLL_MS) {
        return LIGHT_EVENT_NONE;
    }
    s_last_poll_ms = millis_u32();

    s_adc_value = read_adc_average();
#if LIGHT_RAW_INVERTED
    const int level = 4095 - s_adc_value;
#else
    const int level = s_adc_value;
#endif
    s_level = level;

    /* Гістерезис: між порогами режим не змінюється */
    bool candidate = s_is_night;
    if (level < LIGHT_RAW_NIGHT) {
        candidate = true;
    } else if (level > LIGHT_RAW_DAY) {
        candidate = false;
    }

    if (candidate == s_is_night) {
        s_pending_night = s_is_night;   /* повернулося назад */
        return LIGHT_EVENT_NONE;
    }

    /* Нова умова має протриматися LIGHT_STABLE_MS - захист від тіні/фар */
    if (candidate != s_pending_night) {
        s_pending_night   = candidate;
        s_change_since_ms = millis_u32();
        return LIGHT_EVENT_NONE;
    }
    if (elapsed_ms(s_change_since_ms) < LIGHT_STABLE_MS) {
        return LIGHT_EVENT_NONE;
    }

    s_is_night = candidate;
    if (s_is_night) {
        ESP_LOGW(TAG, "темно (рівень=%d): нерегульоване перехрестя", level);
        return LIGHT_EVENT_NIGHT;
    }
    ESP_LOGW(TAG, "світло (рівень=%d): штатне регулювання", level);
    return LIGHT_EVENT_DAY;
}

bool light_sensor_is_night(void)
{
    return s_is_night;
}

int light_sensor_adc(void)
{
    return s_adc_value;
}

int light_sensor_level(void)
{
    return s_level;
}
