/*
 * Апаратний такт системи на GPTimer (період TICK_PERIOD_US).
 */
#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/* Запускає такт; кожен такт будить задачу task */
void tick_timer_init(TaskHandle_t task);

/* Блокує задачу, передану в tick_timer_init(), до наступного такту */
void tick_timer_wait(void);
