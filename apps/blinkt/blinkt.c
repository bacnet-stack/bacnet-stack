/**
 * @file
 * @brief API for Blinkt! daughter board for Raspberry Pi
 *
 * SPDX-License-Identifier: MIT
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include "blinkt.h"

#define BLINKT_DEFAULT_BRIGHTNESS 7
#define BLINKT_NUM_LEDS 8

/* GPIO pin numbers */
#define BLINKT_MOSI 23
#define BLINKT_SCLK 24

/* RGBb data for each LED */
static uint32_t Blinkt_LED[BLINKT_NUM_LEDS];
/* handle to the pigpiod */
static int Blinkt_Pi;

#ifdef BUILD_PIPELINE
#define PI_OUTPUT 1
#define gpio_write(a, b, c) printf("gpio_write(%d, %d, %d)\n", a, b, c)
#define set_mode(a, b, c) printf("set_mode(%d, %d, %d)\n", a, b, c)
#define pigpio_start(a, b) printf("pigpio_start(%p, %p)\n", a, b)
#define pigpio_stop(a) printf("pigpio_stop(%d)\n", a)
#else
#include <pigpiod_if2.h>
#endif

/**
 * @brief Get the number of LEDS
 * @return Number of LEDs
 */
uint8_t blinkt_led_count(void)
{
    return BLINKT_NUM_LEDS;
}

/**
 * @brief Set all the LEDS to default brightness (7/31)
 */
void blinkt_clear(void)
{
    int x;
    for (x = 0; x < BLINKT_NUM_LEDS; x++) {
        Blinkt_LED[x] = BLINKT_DEFAULT_BRIGHTNESS;
    }
}

/**
 * @brief Set one LED to specific RGB color
 * @param led index 0..#BLINKT_NUM_LEDS
 * @param r color red from 0..255
 * @param g color green from 0..255
 * @param b color blue from 0..255
 */
void blinkt_set_pixel(uint8_t led, uint8_t r, uint8_t g, uint8_t b)
{
    if (led >= BLINKT_NUM_LEDS) {
        return;
    }

    Blinkt_LED[led] = blinkt_rgbb(r, g, b, Blinkt_LED[led] & 0x1F);
}

/**
 * @brief Get the current RGB color of one LED
 * @param led index 0..#BLINKT_NUM_LEDS
 * @param r pointer to store the red component (0..255), may be NULL
 * @param g pointer to store the green component (0..255), may be NULL
 * @param b pointer to store the blue component (0..255), may be NULL
 */
void blinkt_get_pixel(uint8_t led, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (led >= BLINKT_NUM_LEDS) {
        return;
    }
    if (r) {
        *r = (Blinkt_LED[led] >> 24) & 0xFF;
    }
    if (g) {
        *g = (Blinkt_LED[led] >> 16) & 0xFF;
    }
    if (b) {
        *b = (Blinkt_LED[led] >> 8) & 0xFF;
    }
}

/**
 * @brief Set one LED to specific intensity
 * @param led index 0..#BLINKT_NUM_LEDS
 * @param brightness intensity from 0..31, 0=OFF, 1=dimmest, 31=brightest
 */
void blinkt_set_pixel_brightness(uint8_t led, uint8_t brightness)
{
    if (led >= BLINKT_NUM_LEDS) {
        return;
    }

    Blinkt_LED[led] = (Blinkt_LED[led] & 0xFFFFFF00) | (brightness & 0x1F);
}

/**
 * @brief Get the brightness of one LED
 * @param led index 0..#BLINKT_NUM_LEDS
 * @return brightness intensity from 0..31, 0=OFF, 1=dimmest, 31=brightest
 */
uint8_t blinkt_get_pixel_brightness(uint8_t led)
{
    if (led >= BLINKT_NUM_LEDS) {
        return 0;
    }

    return Blinkt_LED[led] & 0x1F;
}

/**
 * @brief Set one LED to RGB color and brightness
 * @param led index 0..#BLINKT_NUM_LEDS
 * @param color encoded as 32-bit RGBb (red | green | blue | brightness)
 */
void blinkt_set_pixel_uint32(uint8_t led, uint32_t color)
{
    if (led >= BLINKT_NUM_LEDS) {
        return;
    }

    Blinkt_LED[led] = color;
}

/**
 * @brief Encode RGB color and brightness into 32-bit RGBb
 * @param r color red from 0..255
 * @param g color green from 0..255
 * @param b color blue from 0..255
 * @param brightness intensity from 0..31, 0=OFF, 1=dimmest, 31=brightest
 * @return color encoded as 32-bit RGBb (red | green | blue | brightness)
 */
uint32_t blinkt_rgbb(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    uint32_t result = 0;
    result = (brightness & 0x1F);
    result |= ((uint32_t)r << 24);
    result |= ((uint32_t)g << 16);
    result |= ((uint16_t)b << 8);
    return result;
}

/**
 * @brief Encode RGB color at default brightness into 32-bit RGBb
 * @param r color red from 0..255
 * @param g color green from 0..255
 * @param b color blue from 0..255
 * @return color encoded as 32-bit RGBb (red | green | blue | brightness)
 */
uint32_t blinkt_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    return blinkt_rgbb(r, g, b, BLINKT_DEFAULT_BRIGHTNESS);
}

/**
 * @brief Write a byte via shift register to LEDs
 * @param byte to be written
 */
inline static void write_byte(uint8_t byte)
{
    int n;
    for (n = 0; n < 8; n++) {
        gpio_write(Blinkt_Pi, BLINKT_MOSI, (byte & (1 << (7 - n))) > 0);
        gpio_write(Blinkt_Pi, BLINKT_SCLK, 1);
        gpio_write(Blinkt_Pi, BLINKT_SCLK, 0);
    }
}

/**
 * @brief Shift LED values to the actual LEDs via shift registers
 */
void blinkt_show(void)
{
    int x;

    write_byte(0);
    write_byte(0);
    write_byte(0);
    write_byte(0);
    for (x = 0; x < BLINKT_NUM_LEDS; x++) {
        write_byte(0xE0 | (Blinkt_LED[x] & 0x1F));
        write_byte((Blinkt_LED[x] >> 8) & 0xFF);
        write_byte((Blinkt_LED[x] >> 16) & 0xFF);
        write_byte((Blinkt_LED[x] >> 24) & 0xFF);
    }
    write_byte(0xff);
}

/**
 * @brief Disable the GPIO hardware to the Blinkt! board
 */
void blinkt_stop(void)
{
    /* Disconnect from local Pi. */
    pigpio_stop(Blinkt_Pi);
    printf("PiGPIO stopped\n");
}

/**
 * @brief Initialize the GPIO hardware for the Blinkt! board
 */
int blinkt_init(void)
{
    char *optHost = NULL;
    char *optPort = NULL;

    /* Connect to local Pi. */
    Blinkt_Pi = pigpio_start(optHost, optPort);
    if (Blinkt_Pi < 0) {
        perror("PiGPIO failed to start!");
        return -1;
    }
    set_mode(Blinkt_Pi, BLINKT_MOSI, PI_OUTPUT);
    gpio_write(Blinkt_Pi, BLINKT_MOSI, 0);
    set_mode(Blinkt_Pi, BLINKT_SCLK, PI_OUTPUT);
    gpio_write(Blinkt_Pi, BLINKT_SCLK, 0);
    blinkt_clear();
    printf("PiGPIO started\n");

    return 0;
}

static int test_column = 0;
static int test_z, test_y;
/**
 * @brief Test the Blinkt! board with a simple changing pattern
 */
void blinkt_test_task(void)
{
    for (test_z = 0; test_z < BLINKT_NUM_LEDS; test_z++) {
        switch (test_column) {
            case 0:
                blinkt_set_pixel_uint32(test_z, blinkt_rgb(test_y, 0, 0));
                break;
            case 1:
                blinkt_set_pixel_uint32(test_z, blinkt_rgb(0, test_y, 0));
                break;
            case 2:
                blinkt_set_pixel_uint32(test_z, blinkt_rgb(0, 0, test_y));
                break;
            default:
                break;
        }
    }
    blinkt_show();
    test_y += 1;
    if (test_y > 254) {
        test_column++;
    }
    test_column %= 3;
    test_y %= 255;
}
