/*
 * Час у мілісекундах від старту системи (esp_timer).
 */
#pragma once

#include <stdint.h>

#include "esp_timer.h"

static inline uint32_t millis_u32(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

/* Коректно працює і при переповненні uint32_t (~49 діб) */
static inline uint32_t elapsed_ms(uint32_t since)
{
    return millis_u32() - since;
}
