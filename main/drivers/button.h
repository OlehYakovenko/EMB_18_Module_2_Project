/*
 * Кнопка пішохода: переривання по спаду + неблокуючий debounce.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Вхід з внутрішньою підтяжкою, переривання по натиску */
void button_init(void);

/* Викликати кожен такт. true - рівно один раз на кожен підтверджений натиск */
bool button_poll(void);

uint32_t button_press_count(void);
