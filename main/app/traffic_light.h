/*
 * Кінцевий автомат світлофора за ПДР України (п.8.7).
 *
 *   ЗЕЛЕНИЙ -> ЗЕЛЕНИЙ МИГОТЛИВИЙ -> ЖОВТИЙ -> ЧЕРВОНИЙ -> ЧЕРВОНИЙ+ЖОВТИЙ -> ЗЕЛЕНИЙ
 *   Нічний режим: ЖОВТИЙ МИГОТЛИВИЙ - нерегульоване перехрестя.
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Вмикає ЧЕРВОНИЙ - безпечний початковий стан */
void traffic_light_start(void);

/* Викликати кожен такт: миготіння і перемикання фаз за часом */
void traffic_light_update(void);

/* true - ЖОВТИЙ МИГОТЛИВИЙ; false - штатний цикл з ЧЕРВОНОГО */
void traffic_light_set_night(bool night);

/*
 * Запит пішохода: скорочує ЧЕРВОНИЙ (не коротше PED_RED_MIN_MS),
 * далі штатний перехід ЧЕРВОНИЙ+ЖОВТИЙ -> ЗЕЛЕНИЙ. Фази не пропускаються.
 */
void traffic_light_request_pedestrian(void);

const char *traffic_light_state_name(void);
uint32_t    traffic_light_ms_in_state(void);
bool        traffic_light_ped_pending(void);
