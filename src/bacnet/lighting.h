/**
 * @file
 * @brief API for BACnetLightingCommand and BACnetColorCommand
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date June 2022
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_LIGHTING_H
#define BACNET_LIGHTING_H

#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

/* BACnetLightingCommand ::= SEQUENCE {
    operation [0] BACnetLightingOperation,
    target-level [1] REAL (0.0..100.0) OPTIONAL,
    ramp-rate [2] REAL (0.1..100.0) OPTIONAL,
    step-increment [3] REAL (0.1..100.0) OPTIONAL,
    fade-time [4] Unsigned (100.. 86400000) OPTIONAL,
    priority [5] Unsigned (1..16) OPTIONAL
}
-- Note that the combination of level, ramp-rate, step-increment, and fade-time
fields is
-- dependent on the specific lighting operation. See Table 12-67.
*/
typedef struct BACnetLightingCommand {
    BACNET_LIGHTING_OPERATION operation;
    /* fields are optional */
    bool use_target_level : 1;
    bool use_ramp_rate : 1;
    bool use_step_increment : 1;
    bool use_fade_time : 1;
    bool use_priority : 1;
    float target_level;
    float ramp_rate;
    float step_increment;
    uint32_t fade_time;
    uint8_t priority;
} BACNET_LIGHTING_COMMAND;

/**
 * BACnetxyColor::= SEQUENCE {
 *      x-coordinate REAL, --(0.0 to 1.0)
 *      y-coordinate REAL --(0.0 to 1.0)
 * }
 */
typedef struct BACnetXYColor {
    float x_coordinate;
    float y_coordinate;
} BACNET_XY_COLOR;

/**
 *  BACnetColorCommand ::= SEQUENCE {
 *      operation      [0] BACnetColorOperation,
 *      target-color   [1] BACnetxyColor OPTIONAL,
 *      target-color-temperature [2] Unsigned OPTIONAL,
 *      fade-time      [3] Unsigned (100.. 86400000) OPTIONAL,
 *      ramp-rate      [4] Unsigned (1..30000) OPTIONAL,
 *      step-increment [5] Unsigned (1..30000) OPTIONAL
 *  }
 */
typedef struct BACnetColorCommand {
    BACNET_COLOR_OPERATION operation;
    union {
        BACNET_XY_COLOR color;
        uint16_t color_temperature;
    } target;
    union {
        uint32_t fade_time;
        uint16_t ramp_rate;
        uint16_t step_increment;
    } transit;
} BACNET_COLOR_COMMAND;

/* range restrictions */
#define BACNET_COLOR_FADE_TIME_MIN 100ul
#define BACNET_COLOR_FADE_TIME_MAX 86400000ul
#define BACNET_COLOR_RAMP_RATE_MIN 1ul
#define BACNET_COLOR_RAMP_RATE_MAX 30000ul
#define BACNET_COLOR_STEP_INCREMENT_MIN 1ul
#define BACNET_COLOR_STEP_INCREMENT_MAX 30000ul

#define BACNET_COLOR_TEMPERATURE_MIN 1000ul
#define BACNET_COLOR_TEMPERATURE_MAX 30000ul

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
int lighting_command_encode(uint8_t *apdu, const BACNET_LIGHTING_COMMAND *data);
BACNET_STACK_EXPORT
int lighting_command_encode_context(
    uint8_t *apdu, uint8_t tag_number, const BACNET_LIGHTING_COMMAND *value);
BACNET_STACK_EXPORT
int lighting_command_decode(
    const uint8_t *apdu, unsigned apdu_max_len, BACNET_LIGHTING_COMMAND *data);
BACNET_STACK_EXPORT
bool lighting_command_copy(
    BACNET_LIGHTING_COMMAND *dst, const BACNET_LIGHTING_COMMAND *src);
BACNET_STACK_EXPORT
bool lighting_command_same(
    const BACNET_LIGHTING_COMMAND *dst, const BACNET_LIGHTING_COMMAND *src);

BACNET_STACK_EXPORT
bool lighting_command_from_ascii(
    BACNET_LIGHTING_COMMAND *value, const char *argv);
BACNET_STACK_EXPORT
int lighting_command_to_ascii(
    const BACNET_LIGHTING_COMMAND *value, char *buf, size_t buf_size);

BACNET_STACK_EXPORT
int xy_color_encode(uint8_t *apdu, const BACNET_XY_COLOR *value);
BACNET_STACK_EXPORT
int xy_color_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_XY_COLOR *value);
BACNET_STACK_EXPORT
int xy_color_decode(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_XY_COLOR *value);
BACNET_STACK_EXPORT
int xy_color_context_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_XY_COLOR *value);
BACNET_STACK_EXPORT
int xy_color_copy(BACNET_XY_COLOR *dst, const BACNET_XY_COLOR *src);
BACNET_STACK_EXPORT
bool xy_color_same(
    const BACNET_XY_COLOR *value1, const BACNET_XY_COLOR *value2);
BACNET_STACK_EXPORT
void xy_color_set(BACNET_XY_COLOR *dst, float x, float y);
BACNET_STACK_EXPORT
int xy_color_to_ascii(const BACNET_XY_COLOR *value, char *buf, size_t buf_size);
BACNET_STACK_EXPORT
bool xy_color_from_ascii(BACNET_XY_COLOR *value, const char *arg);

BACNET_STACK_EXPORT
int color_command_encode(uint8_t *apdu, const BACNET_COLOR_COMMAND *address);
BACNET_STACK_EXPORT
int color_command_context_encode(
    uint8_t *apdu, uint8_t tag_number, const BACNET_COLOR_COMMAND *address);
BACNET_STACK_EXPORT
int color_command_decode(
    const uint8_t *apdu,
    uint16_t apdu_len,
    BACNET_ERROR_CODE *error_code,
    BACNET_COLOR_COMMAND *address);
BACNET_STACK_EXPORT
bool color_command_copy(
    BACNET_COLOR_COMMAND *dst, const BACNET_COLOR_COMMAND *src);
BACNET_STACK_EXPORT
bool color_command_same(
    const BACNET_COLOR_COMMAND *dst, const BACNET_COLOR_COMMAND *src);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
