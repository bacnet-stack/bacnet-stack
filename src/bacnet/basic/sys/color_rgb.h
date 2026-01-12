/**
 * @file
 * @brief API Color sRGB to CIE xy and brightness
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2022
 * @copyright SPDX-License-Identifier: CC-PDDC
 */
#ifndef BACNET_SYS_COLOR_RGB_H
#define BACNET_SYS_COLOR_RGB_H
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
double color_rgb_clamp(double d, double min, double max);

BACNET_STACK_EXPORT
void color_rgb_to_xy(
    uint8_t r,
    uint8_t g,
    uint8_t b,
    float *x_coordinate,
    float *y_coordinate,
    uint8_t *brightness);
BACNET_STACK_EXPORT
void color_rgb_from_xy(
    uint8_t *red,
    uint8_t *green,
    uint8_t *blue,
    float x_coordinate,
    float y_coordinate,
    uint8_t brightness);

BACNET_STACK_EXPORT
void color_rgb_to_xy_gamma(
    uint8_t r,
    uint8_t g,
    uint8_t b,
    float *x_coordinate,
    float *y_coordinate,
    uint8_t *brightness);
BACNET_STACK_EXPORT
void color_rgb_from_xy_gamma(
    uint8_t *red,
    uint8_t *green,
    uint8_t *blue,
    float x_coordinate,
    float y_coordinate,
    uint8_t brightness);

BACNET_STACK_EXPORT
const char *color_rgb_to_ascii(uint8_t red, uint8_t green, uint8_t blue);
BACNET_STACK_EXPORT
unsigned color_rgb_from_ascii(
    uint8_t *red, uint8_t *green, uint8_t *blue, const char *name);
BACNET_STACK_EXPORT
const char *color_rgb_from_index(
    unsigned target_index, uint8_t *red, uint8_t *green, uint8_t *blue);
BACNET_STACK_EXPORT
unsigned color_rgb_count(void);

BACNET_STACK_EXPORT
void color_rgb_from_temperature(
    uint16_t temperature_kelvin, uint8_t *r, uint8_t *g, uint8_t *b);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
