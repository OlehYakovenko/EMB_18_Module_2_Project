#include "app/traffic_light.h"

#include "esp_log.h"

#include "config/app_config.h"
#include "drivers/traffic_leds.h"
#include "util/time_utils.h"

static const char *TAG = "traffic-light";

typedef enum {
    ST_RED = 0,
    ST_RED_YELLOW,
    ST_GREEN,
    ST_GREEN_BLINK,
    ST_YELLOW,
    ST_NIGHT,           /* жовтий миготливий - нерегульоване перехрестя */
    ST_COUNT
} tl_state_t;

static const char *const STATE_NAME[ST_COUNT] = {
    [ST_RED]         = "ЧЕРВОНИЙ",
    [ST_RED_YELLOW]  = "ЧЕРВОНИЙ+ЖОВТИЙ",
    [ST_GREEN]       = "ЗЕЛЕНИЙ",
    [ST_GREEN_BLINK] = "ЗЕЛЕНИЙ МИГОТЛИВИЙ",
    [ST_YELLOW]      = "ЖОВТИЙ",
    [ST_NIGHT]       = "ЖОВТИЙ МИГОТЛИВИЙ (ніч)",
};

static const uint32_t STATE_DURATION_MS[ST_COUNT] = {
    [ST_RED]         = T_RED_MS,
    [ST_RED_YELLOW]  = T_RED_YELLOW_MS,
    [ST_GREEN]       = T_GREEN_MS,
    [ST_GREEN_BLINK] = T_GREEN_BLINK_MS,
    [ST_YELLOW]      = T_YELLOW_MS,
    [ST_NIGHT]       = 0,   /* без таймауту: поки темно */
};

/* Штатна послідовність ПДР */
static const tl_state_t STATE_NEXT[ST_COUNT] = {
    [ST_RED]         = ST_RED_YELLOW,
    [ST_RED_YELLOW]  = ST_GREEN,
    [ST_GREEN]       = ST_GREEN_BLINK,
    [ST_GREEN_BLINK] = ST_YELLOW,
    [ST_YELLOW]      = ST_RED,
    [ST_NIGHT]       = ST_NIGHT,
};

static tl_state_t s_state;
static uint32_t   s_state_entered_ms;
static uint32_t   s_blink_last_ms;
static bool       s_blink_on;
static bool       s_ped_request;    /* запит пішохода очікує обробки */

static void enter_state(tl_state_t next)
{
    s_state            = next;
    s_state_entered_ms = millis_u32();
    s_blink_last_ms    = s_state_entered_ms;
    s_blink_on         = true;      /* миготливі фази починаються зі свічення */

    switch (s_state) {
    case ST_RED:         traffic_leds_set(true,  false, false); break;
    case ST_RED_YELLOW:  traffic_leds_set(true,  true,  false); break;
    case ST_GREEN:       traffic_leds_set(false, false, true);  break;
    case ST_GREEN_BLINK: traffic_leds_set(false, false, true);  break;
    case ST_YELLOW:      traffic_leds_set(false, true,  false); break;
    case ST_NIGHT:       traffic_leds_set(false, true,  false); break;
    default:             traffic_leds_set(false, false, false); break;
    }

    ESP_LOGI(TAG, "-> %s (%lu мс)", STATE_NAME[s_state],
             (unsigned long)STATE_DURATION_MS[s_state]);
}

/* Миготіння всередині фази: перемикає лише "свою" лампу */
static void update_blink(void)
{
    uint32_t period;

    if (s_state == ST_GREEN_BLINK) {
        period = BLINK_GREEN_MS;
    } else if (s_state == ST_NIGHT) {
        period = BLINK_NIGHT_MS;
    } else {
        return;
    }

    if (elapsed_ms(s_blink_last_ms) < period) {
        return;
    }
    s_blink_last_ms = millis_u32();
    s_blink_on = !s_blink_on;

    if (s_state == ST_GREEN_BLINK) {
        traffic_leds_set_green(s_blink_on);
    } else {
        traffic_leds_set_yellow(s_blink_on);
    }
}

void traffic_light_start(void)
{
    ESP_LOGI(TAG, "цикл: З %lu + З-миг %lu + Ж %lu + Ч %lu + Ч+Ж %lu мс",
             (unsigned long)T_GREEN_MS, (unsigned long)T_GREEN_BLINK_MS,
             (unsigned long)T_YELLOW_MS, (unsigned long)T_RED_MS,
             (unsigned long)T_RED_YELLOW_MS);

    s_ped_request = false;
    enter_state(ST_RED);
}

void traffic_light_update(void)
{
    update_blink();

    if (s_state == ST_NIGHT) {
        return;     /* вихід з ночі - лише за датчиком світла */
    }

    const uint32_t in_state = elapsed_ms(s_state_entered_ms);

    /*
     * Запит пішохода: скорочуємо червоне, але не коротше за PED_RED_MIN_MS.
     * Перехід іде штатним шляхом ЧЕРВОНИЙ -> ЧЕРВОНИЙ+ЖОВТИЙ -> ЗЕЛЕНИЙ.
     */
    if (s_ped_request && s_state == ST_RED && in_state >= PED_RED_MIN_MS) {
        s_ped_request = false;
        ESP_LOGI(TAG, "запит пішохода: червоний скорочено на %lu мс",
                 (unsigned long)(T_RED_MS - in_state));
        enter_state(ST_RED_YELLOW);
        return;
    }

    if (in_state >= STATE_DURATION_MS[s_state]) {
        if (s_state == ST_GREEN && s_ped_request) {
            s_ped_request = false;      /* пішохід уже йде - запит виконано */
            ESP_LOGI(TAG, "запит пішохода виконано штатним зеленим");
        }
        enter_state(STATE_NEXT[s_state]);
    }
}

void traffic_light_set_night(bool night)
{
    if (night) {
        s_ped_request = false;
        enter_state(ST_NIGHT);
    } else {
        enter_state(ST_RED);        /* безпечний старт циклу */
    }
}

void traffic_light_request_pedestrian(void)
{
    if (s_state == ST_NIGHT) {
        ESP_LOGI(TAG, "запит пішохода: нічний режим, регулювання вимкнене");
    } else if (s_ped_request) {
        ESP_LOGI(TAG, "запит пішохода: уже прийнято");
    } else {
        s_ped_request = true;
        ESP_LOGI(TAG, "запит пішохода прийнято (стан %s)", STATE_NAME[s_state]);
    }
}

const char *traffic_light_state_name(void)
{
    return STATE_NAME[s_state];
}

uint32_t traffic_light_ms_in_state(void)
{
    return elapsed_ms(s_state_entered_ms);
}

bool traffic_light_ped_pending(void)
{
    return s_ped_request;
}
