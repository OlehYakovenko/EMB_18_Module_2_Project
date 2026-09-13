/*
 * Три лампи світлофора (активний рівень - 1).
 */
#pragma once

#include <stdbool.h>

/* Налаштовує піни на вихід, усі лампи вимкнені */
void traffic_leds_init(void);

void traffic_leds_set(bool red, bool yellow, bool green);

/* Перемикання однієї лампи - для миготливих фаз */
void traffic_leds_set_green(bool on);
void traffic_leds_set_yellow(bool on);
