/*
 * Датчик освітлення: дільник з фоторезистором на ADC.
 * Визначає день/ніч з гістерезисом і витримкою LIGHT_STABLE_MS.
 */
#pragma once

#include <stdbool.h>

typedef enum {
    LIGHT_EVENT_NONE = 0,
    LIGHT_EVENT_NIGHT,      /* стало стабільно темно */
    LIGHT_EVENT_DAY,        /* стало стабільно світло */
} light_event_t;

/* Налаштовує ADC; стартовий режим - день */
void light_sensor_init(void);

/* Викликати кожен такт; сам витримує період LIGHT_POLL_MS */
light_event_t light_sensor_poll(void);

bool light_sensor_is_night(void);
int  light_sensor_adc(void);        /* останній усереднений код ADC */
int  light_sensor_level(void);      /* рівень світла з урахуванням інверсії */
