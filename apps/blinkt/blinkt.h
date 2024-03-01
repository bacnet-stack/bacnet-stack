/**
 * @file
 * @brief API for Blinkt! daughter board for Raspberry Pi
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef BLINKT_H
#define BLINKT_H

void blinkt_clear(void);
void blinkt_set_pixel_uint32(uint8_t led, uint32_t color);
void blinkt_set_pixel(uint8_t led, uint8_t r, uint8_t g, uint8_t b);
void blinkt_set_pixel_brightness(uint8_t led, uint8_t brightness);
uint32_t blinkt_rgbb(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);
uint32_t blinkt_rgb(uint8_t r, uint8_t g, uint8_t b);
void blinkt_stop(void);
void blinkt_show(void);
uint8_t blinkt_led_count(void);

int blinkt_init(void);
void blinkt_test_task(void);

#endif
