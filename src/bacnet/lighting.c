/**
 * @file
 * @brief API for BACnetLightingCommand and BACnetColorCommand
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date June 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include "bacnet/bacdcode.h"
#include "bacnet/bacreal.h"
#include "bacnet/lighting.h"

#ifndef islessgreater
#define islessgreater(x, y) ((x) < (y) || (x) > (y))
#endif

/** @file lighting.c  Manipulate BACnet lighting command values */

/**
 * Encodes into bytes from the lighting-command structure
 *
 * @param apdu - buffer to hold the bytes, or null for length
 * @param value - lighting command value to encode
 *
 * @return  number of bytes encoded
 */
int lighting_command_encode(uint8_t *apdu, BACNET_LIGHTING_COMMAND *data)
{
    int apdu_len = 0; /* total length of the apdu, return value */
    int len = 0; /* total length of the apdu, return value */
    uint8_t *apdu_offset = NULL;

    len = encode_context_enumerated(apdu, 0, data->operation);
    apdu_len += len;
    /* optional target-level */
    if (data->use_target_level) {
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = encode_context_real(apdu_offset, 1, data->target_level);
        apdu_len += len;
    }
    /* optional ramp-rate */
    if (data->use_ramp_rate) {
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = encode_context_real(apdu_offset, 2, data->ramp_rate);
        apdu_len += len;
    }
    /* optional step increment */
    if (data->use_step_increment) {
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = encode_context_real(apdu_offset, 3, data->step_increment);
        apdu_len += len;
    }
    /* optional fade time */
    if (data->use_fade_time) {
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = encode_context_unsigned(apdu_offset, 4, data->fade_time);
        apdu_len += len;
    }
    /* optional priority */
    if (data->use_priority) {
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = encode_context_unsigned(apdu_offset, 5, data->priority);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * Encodes into bytes from the lighting-command structure
 * a context tagged chunk (opening and closing tag)
 *
 * @param apdu - buffer to hold the bytes
 * @param tag_number - tag number to encode this chunk
 * @param value - lighting command value to encode
 *
 * @return  number of bytes encoded, or 0 if unable to encode.
 */
int lighting_command_encode_context(
    uint8_t *apdu, uint8_t tag_number, BACNET_LIGHTING_COMMAND *value)
{
    int apdu_len = 0;
    uint8_t *apdu_offset = NULL;

    apdu_len += encode_opening_tag(apdu, tag_number);
    if (apdu) {
        apdu_offset = &apdu[apdu_len];
    }
    apdu_len += lighting_command_encode(apdu_offset, value);
    if (apdu) {
        apdu_offset = &apdu[apdu_len];
    }
    apdu_len += encode_closing_tag(apdu_offset, tag_number);

    return apdu_len;
}

/**
 * Decodes from bytes into the lighting-command structure
 *
 * @param apdu - buffer to hold the bytes
 * @param apdu_max_len - number of bytes in the buffer to decode
 * @param value - lighting command value to place the decoded values
 *
 * @return  number of bytes decoded
 */
int lighting_command_decode(
    uint8_t *apdu, unsigned apdu_max_len, BACNET_LIGHTING_COMMAND *data)
{
    int len = 0;
    int apdu_len = 0;
    uint8_t tag_number = 0;
    uint32_t len_value_type = 0;
    uint32_t enum_value = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_LIGHTING_OPERATION operation = BACNET_LIGHTS_NONE;
    float real_value = 0.0;

    /* check for value pointers */
    if (apdu_max_len) {
        /* Tag 0: operation */
        if (!decode_is_context_tag(&apdu[apdu_len], 0)) {
            return BACNET_STATUS_ERROR;
        }
        len = decode_tag_number_and_value(
            &apdu[apdu_len], &tag_number, &len_value_type);
        apdu_len += len;
        len = decode_enumerated(&apdu[apdu_len], len_value_type, &enum_value);
        if (len > 0) {
            if (unsigned_value <= BACNET_LIGHTS_PROPRIETARY_LAST) {
                if (data) {
                    data->operation = (BACNET_LIGHTING_OPERATION)enum_value;
                }
            } else {
                return BACNET_STATUS_ERROR;
            }
        }
        apdu_len += len;
    }
    switch (operation) {
        case BACNET_LIGHTS_NONE:
            break;
        case BACNET_LIGHTS_FADE_TO:
            if ((apdu_max_len - apdu_len) == 0) {
                return BACNET_STATUS_REJECT;
            }
            /* Tag 1: target-level */
            if (decode_is_context_tag(&apdu[apdu_len], 1)) {
                len = decode_tag_number_and_value(
                    &apdu[apdu_len], &tag_number, &len_value_type);
                apdu_len += len;
                len = decode_real(&apdu[apdu_len], &real_value);
                apdu_len += len;
                if (data) {
                    data->target_level = real_value;
                    data->use_target_level = true;
                }
            } else {
                if (data) {
                    data->use_target_level = false;
                }
            }
            if ((apdu_max_len - apdu_len) != 0) {
                /* Tag 4: fade-time - OPTIONAL */
                if (decode_is_context_tag(&apdu[apdu_len], 4)) {
                    len = decode_tag_number_and_value(
                        &apdu[apdu_len], &tag_number, &len_value_type);
                    apdu_len += len;
                    len = decode_unsigned(
                        &apdu[apdu_len], len_value_type, &unsigned_value);
                    apdu_len += len;
                    if (data) {
                        data->fade_time = (uint32_t)unsigned_value;
                        data->use_fade_time = true;
                    }
                } else {
                    if (data) {
                        data->use_fade_time = false;
                    }
                }
            }
            if ((apdu_max_len - apdu_len) != 0) {
                /* Tag 5: priority - OPTIONAL */
                if (decode_is_context_tag(&apdu[apdu_len], 4)) {
                    len = decode_tag_number_and_value(
                        &apdu[apdu_len], &tag_number, &len_value_type);
                    apdu_len += len;
                    len = decode_unsigned(
                        &apdu[apdu_len], len_value_type, &unsigned_value);
                    apdu_len += len;
                    if (data) {
                        data->priority = (uint8_t)unsigned_value;
                        data->use_priority = true;
                    }
                } else {
                    if (data) {
                        data->use_priority = false;
                    }
                }
            }
            break;
        case BACNET_LIGHTS_RAMP_TO:
            if ((apdu_max_len - apdu_len) == 0) {
                return BACNET_STATUS_REJECT;
            }
            /* Tag 1: target-level */
            if (decode_is_context_tag(&apdu[apdu_len], 1)) {
                len = decode_tag_number_and_value(
                    &apdu[apdu_len], &tag_number, &len_value_type);
                apdu_len += len;
                len = decode_real(&apdu[apdu_len], &real_value);
                apdu_len += len;
                if (data) {
                    data->target_level = real_value;
                    data->use_target_level = true;
                }
            } else {
                if (data) {
                    data->use_target_level = false;
                }
            }
            if ((apdu_max_len - apdu_len) != 0) {
                /* Tag 2: ramp-rate - OPTIONAL */
                if (decode_is_context_tag(&apdu[apdu_len], 2)) {
                    len = decode_tag_number_and_value(
                        &apdu[apdu_len], &tag_number, &len_value_type);
                    apdu_len += len;
                    len = decode_real(&apdu[apdu_len], &real_value);
                    apdu_len += len;
                    if (data) {
                        data->ramp_rate = real_value;
                        data->use_ramp_rate = true;
                    }
                } else {
                    if (data) {
                        data->use_ramp_rate = false;
                    }
                }
            }
            if ((apdu_max_len - apdu_len) != 0) {
                /* Tag 5: priority - OPTIONAL */
                if (decode_is_context_tag(&apdu[apdu_len], 4)) {
                    len = decode_tag_number_and_value(
                        &apdu[apdu_len], &tag_number, &len_value_type);
                    apdu_len += len;
                    len = decode_unsigned(
                        &apdu[apdu_len], len_value_type, &unsigned_value);
                    apdu_len += len;
                    if (data) {
                        data->priority = (uint8_t)unsigned_value;
                        data->use_priority = true;
                    }
                } else {
                    if (data) {
                        data->use_priority = false;
                    }
                }
            }
            break;
        case BACNET_LIGHTS_STEP_UP:
        case BACNET_LIGHTS_STEP_DOWN:
        case BACNET_LIGHTS_STEP_ON:
        case BACNET_LIGHTS_STEP_OFF:
            if ((apdu_max_len - apdu_len) != 0) {
                /* Tag 3: step-increment - OPTIONAL */
                if (decode_is_context_tag(&apdu[apdu_len], 3)) {
                    len = decode_tag_number_and_value(
                        &apdu[apdu_len], &tag_number, &len_value_type);
                    apdu_len += len;
                    len = decode_real(&apdu[apdu_len], &real_value);
                    apdu_len += len;
                    if (data) {
                        data->step_increment = real_value;
                        data->use_step_increment = true;
                    }
                } else {
                    if (data) {
                        data->use_step_increment = false;
                    }
                }
            }
            if ((apdu_max_len - apdu_len) != 0) {
                /* Tag 5: priority - OPTIONAL */
                if (decode_is_context_tag(&apdu[apdu_len], 4)) {
                    len = decode_tag_number_and_value(
                        &apdu[apdu_len], &tag_number, &len_value_type);
                    apdu_len += len;
                    len = decode_unsigned(
                        &apdu[apdu_len], len_value_type, &unsigned_value);
                    apdu_len += len;
                    if (data) {
                        data->priority = (uint8_t)unsigned_value;
                        data->use_priority = true;
                    }
                } else {
                    if (data) {
                        data->use_priority = false;
                    }
                }
            }
            break;
        case BACNET_LIGHTS_WARN:
        case BACNET_LIGHTS_WARN_OFF:
        case BACNET_LIGHTS_WARN_RELINQUISH:
        case BACNET_LIGHTS_STOP:
            if ((apdu_max_len - apdu_len) != 0) {
                /* Tag 5: priority - OPTIONAL */
                if (decode_is_context_tag(&apdu[apdu_len], 4)) {
                    len = decode_tag_number_and_value(
                        &apdu[apdu_len], &tag_number, &len_value_type);
                    apdu_len += len;
                    len = decode_unsigned(
                        &apdu[apdu_len], len_value_type, &unsigned_value);
                    apdu_len += len;
                    if (data) {
                        data->priority = (uint8_t)unsigned_value;
                        data->use_priority = true;
                    }
                } else {
                    if (data) {
                        data->use_priority = false;
                    }
                }
            }
            break;
        default:
            break;
    }

    return apdu_len;
}

/**
 * Copies one lighting-command structure to another
 *
 * @param dst - lighting command value target
 * @param src - lighting command value source
 *
 * @return  true if copy succeeded
 */
bool lighting_command_copy(
    BACNET_LIGHTING_COMMAND *dst, BACNET_LIGHTING_COMMAND *src)
{
    bool status = false;

    if (dst && src) {
        dst->operation = src->operation;
        dst->use_target_level = src->use_target_level;
        dst->use_ramp_rate = src->use_ramp_rate;
        dst->use_step_increment = src->use_step_increment;
        dst->use_fade_time = src->use_fade_time;
        dst->use_priority = src->use_priority;
        dst->target_level = src->target_level;
        dst->ramp_rate = src->ramp_rate;
        dst->step_increment = src->step_increment;
        dst->fade_time = src->fade_time;
        dst->priority = src->priority;
        status = true;
    }

    return status;
}

/**
 * Compare one lighting-command structure to another
 *
 * @param dst - lighting command value target
 * @param src - lighting command value source
 *
 * @return  true if lighting-commands are the same for values in-use
 */
bool lighting_command_same(
    BACNET_LIGHTING_COMMAND *dst, BACNET_LIGHTING_COMMAND *src)
{
    bool status = false;

    if (dst && src) {
        if ((dst->operation == src->operation) &&
            (dst->use_target_level == src->use_target_level) &&
            (dst->use_ramp_rate == src->use_ramp_rate) &&
            (dst->use_step_increment == src->use_step_increment) &&
            (dst->use_fade_time == src->use_fade_time) &&
            (dst->use_priority == src->use_priority)) {
            status = true;
            if ((dst->use_target_level) &&
                islessgreater(dst->target_level, src->target_level)) {
                status = false;
            }
            if ((dst->use_ramp_rate) &&
                islessgreater(dst->ramp_rate, src->ramp_rate)) {
                status = false;
            }
            if ((dst->use_step_increment) &&
                islessgreater(dst->step_increment, src->step_increment)) {
                status = false;
            }
            if ((dst->use_fade_time) && (dst->fade_time != src->fade_time)) {
                status = false;
            }
            if ((dst->use_priority) && (dst->priority != src->priority)) {
                status = false;
            }
        }
    }

    return status;
}

/**
 * @brief Encode a BACnetxyColor complex data type
 *
 * BACnetxyColor::= SEQUENCE {
 *      x-coordinate REAL, --(0.0 to 1.0)
 *      y-coordinate REAL --(0.0 to 1.0)
 * }
 * @param apdu - the APDU buffer, or NULL for length
 * @param value - BACnetxyColor structure
 * @return length of the encoded APDU buffer
 */
int xy_color_encode(uint8_t *apdu, BACNET_XY_COLOR *value)
{
    int len = 0;
    int apdu_len = 0;
    uint8_t *apdu_offset = NULL;

    if (value) {
        /* x-coordinate REAL */
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = encode_bacnet_real(value->x_coordinate, apdu_offset);
        apdu_len += len;
        /* y-coordinate REAL */
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = encode_bacnet_real(value->y_coordinate, apdu_offset);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Encode a context tagged BACnetxyColor complex data type
 * @param apdu - the APDU buffer
 * @param tag_number - the APDU buffer size
 * @param value - BACnetxyColor structure
 * @return length of the APDU buffer, or 0 if not able to encode
 */
int xy_color_context_encode(
    uint8_t *apdu, uint8_t tag_number, BACNET_XY_COLOR *value)
{
    int len = 0;
    int apdu_len = 0;
    uint8_t *apdu_offset = NULL;

    if (value) {
        apdu_offset = apdu;
        len = encode_opening_tag(apdu_offset, tag_number);
        apdu_len += len;
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = xy_color_encode(apdu_offset, value);
        apdu_len += len;
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = encode_closing_tag(apdu_offset, tag_number);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Decode the BACnetxyColor complex data type Value
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_size - the size of the data buffer
 * @param value - decoded BACnetxyColor, if decoded
 *
 * @return the number of apdu bytes consumed
 */
int xy_color_decode(uint8_t *apdu, uint32_t apdu_size, BACNET_XY_COLOR *value)
{
    float real_value;
    int len = 0;
    int apdu_len = 0;

    if (apdu && value && (apdu_size >= 8)) {
        /* each REAL is encoded in 4 octets */
        len = decode_real(&apdu[0], &real_value);
        if (len == 4) {
            if (value) {
                value->x_coordinate = real_value;
            }
            apdu_len += len;
        }
        len = decode_real(&apdu[4], &real_value);
        if (len == 4) {
            if (value) {
                value->y_coordinate = real_value;
            }
            apdu_len += len;
        }
    }

    return apdu_len;
}

/**
 * @brief Decode the BACnetxyColor complex data type Value
 *
 * @param apdu - buffer of data to be decoded
 * @param apdu_size - the size of the data buffer
 * @param value - decoded BACnetxyColor, if decoded
 *
 * @return the number of apdu bytes consumed
 */
int xy_color_context_decode(uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_XY_COLOR *value)
{
    int len = 0;
    int rlen = 0;
    int apdu_len = 0;
    BACNET_XY_COLOR color = { 0.0, 0.0 };

    if (apdu_size > 0) {
        if (decode_is_opening_tag_number(&apdu[apdu_len], tag_number)) {
            apdu_len += 1;
            len =
                xy_color_decode(&apdu[apdu_len], apdu_size - apdu_len, &color);
            if (len > 0) {
                apdu_len += len;
                if (value) {
                    value->x_coordinate = color.x_coordinate;
                    value->y_coordinate = color.y_coordinate;
                }
                if ((apdu_size - apdu_len) > 0) {
                    if (decode_is_closing_tag_number(
                            &apdu[apdu_len], tag_number)) {
                        apdu_len += 1;
                        rlen = apdu_len;
                    }
                }
            }
        }
    }

    return rlen;
}

/**
 * @brief Copy the BACnetxyColor complex data from src to dst
 * @param dst - destination BACNET_XY_COLOR structure
 * @param src - source BACNET_XY_COLOR structure
 * @return true if successfully copied
 */
int xy_color_copy(BACNET_XY_COLOR *dst, BACNET_XY_COLOR *src)
{
    bool status = false;

    if (dst && src) {
        dst->x_coordinate = src->x_coordinate;
        dst->y_coordinate = src->y_coordinate;
        status = true;
    }

    return status;
}

/**
 * @brief Compare the BACnetxyColor complex data
 * @param value1 - BACNET_XY_COLOR structure
 * @param value2 - BACNET_XY_COLOR structure
 * @return true if the same
 */
bool xy_color_same(BACNET_XY_COLOR *value1, BACNET_XY_COLOR *value2)
{
    bool status = false;

    if (value1 && value2) {
        if ((value1->x_coordinate == value2->x_coordinate) &&
            (value1->y_coordinate == value2->y_coordinate)) {
            status = true;
        }
    }

    return status;
}

/**
 * @brief Encode a BACnetColorCommand complex data type
 *
 *  BACnetColorCommand structure ::= SEQUENCE {
 *      operation      [0] BACnetColorOperation,
 *      target-color   [1] BACnetxyColor OPTIONAL,
 *      target-color-temperature [2] Unsigned OPTIONAL,
 *      fade-time      [3] Unsigned (100.. 86400000) OPTIONAL,
 *      ramp-rate      [4] Unsigned (1..30000) OPTIONAL,
 *      step-increment [5] Unsigned (1..30000) OPTIONAL
 *  }
 *
 * @param apdu - the APDU buffer, or NULL for length
 * @param value - BACnetColorCommand structure
 * @return length of the encoded APDU buffer
 */
int color_command_encode(uint8_t *apdu, BACNET_COLOR_COMMAND *value)
{
    int len = 0;
    int apdu_len = 0;
    uint8_t *apdu_offset = NULL;
    BACNET_UNSIGNED_INTEGER unsigned_value;

    if (value) {
        /* operation [0] BACnetColorOperation */
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = encode_context_enumerated(apdu_offset, 0, value->operation);
        apdu_len += len;
        switch (value->operation) {
            case BACNET_COLOR_OPERATION_NONE:
                break;
            case BACNET_COLOR_OPERATION_FADE_TO_COLOR:
                if (apdu) {
                    apdu_offset = &apdu[apdu_len];
                }
                /* target-color [1] BACnetxyColor */
                len = xy_color_context_encode(
                    apdu_offset, 1, &value->target.color);
                apdu_len += len;
                if ((value->transit.fade_time >= BACNET_COLOR_FADE_TIME_MIN) &&
                    (value->transit.fade_time <= BACNET_COLOR_FADE_TIME_MAX)) {
                    /* fade-time [3] Unsigned (100.. 86400000) */
                    unsigned_value = value->transit.fade_time;
                    if (apdu) {
                        apdu_offset = &apdu[apdu_len];
                    }
                    len =
                        encode_context_unsigned(apdu_offset, 3, unsigned_value);
                    apdu_len += len;
                }
                break;
            case BACNET_COLOR_OPERATION_FADE_TO_CCT:
                if (apdu) {
                    apdu_offset = &apdu[apdu_len];
                }
                /* target-color-temperature [2] Unsigned */
                unsigned_value = value->target.color_temperature;
                if (apdu) {
                    apdu_offset = &apdu[apdu_len];
                }
                len = encode_context_unsigned(apdu_offset, 2, unsigned_value);
                apdu_len += len;
                if ((value->transit.fade_time >= BACNET_COLOR_FADE_TIME_MIN) &&
                    (value->transit.fade_time <= BACNET_COLOR_FADE_TIME_MAX)) {
                    /* fade-time [3] Unsigned (100.. 86400000) */
                    unsigned_value = value->transit.fade_time;
                    if (apdu) {
                        apdu_offset = &apdu[apdu_len];
                    }
                    len =
                        encode_context_unsigned(apdu_offset, 3, unsigned_value);
                    apdu_len += len;
                }
                break;
            case BACNET_COLOR_OPERATION_RAMP_TO_CCT:
                if (apdu) {
                    apdu_offset = &apdu[apdu_len];
                }
                /* target-color-temperature [2] Unsigned */
                unsigned_value = value->target.color_temperature;
                if (apdu) {
                    apdu_offset = &apdu[apdu_len];
                }
                len = encode_context_unsigned(apdu_offset, 2, unsigned_value);
                apdu_len += len;
                if ((value->transit.ramp_rate >= BACNET_COLOR_RAMP_RATE_MIN) &&
                    (value->transit.ramp_rate <= BACNET_COLOR_RAMP_RATE_MAX)) {
                    /* ramp-rate      [4] Unsigned (1..30000) */
                    unsigned_value = value->transit.ramp_rate;
                    if (apdu) {
                        apdu_offset = &apdu[apdu_len];
                    }
                    len =
                        encode_context_unsigned(apdu_offset, 4, unsigned_value);
                    apdu_len += len;
                }
                break;
            case BACNET_COLOR_OPERATION_STEP_UP_CCT:
            case BACNET_COLOR_OPERATION_STEP_DOWN_CCT:
                if ((value->transit.step_increment >=
                        BACNET_COLOR_STEP_INCREMENT_MIN) &&
                    (value->transit.step_increment <=
                        BACNET_COLOR_STEP_INCREMENT_MAX)) {
                    if (apdu) {
                        apdu_offset = &apdu[apdu_len];
                    }
                    /* step-increment [5] Unsigned (1..30000) */
                    unsigned_value = value->transit.step_increment;
                    if (apdu) {
                        apdu_offset = &apdu[apdu_len];
                    }
                    len =
                        encode_context_unsigned(apdu_offset, 5, unsigned_value);
                    apdu_len += len;
                }
                break;
            case BACNET_COLOR_OPERATION_STOP:
                break;
            default:
                break;
        }
    }

    return apdu_len;
}

/**
 * @brief Encode a context tagged BACnetColorCommand complex data type
 * @param apdu - the APDU buffer
 * @param tag_number - the APDU buffer size
 * @param address - IP address and port number
 * @return length of the APDU buffer, or 0 if not able to encode
 */
int color_command_context_encode(
    uint8_t *apdu, uint8_t tag_number, BACNET_COLOR_COMMAND *value)
{
    int len = 0;
    int apdu_len = 0;
    uint8_t *apdu_offset = NULL;

    if (value) {
        apdu_offset = apdu;
        len = encode_opening_tag(apdu_offset, tag_number);
        apdu_len += len;
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = color_command_encode(apdu_offset, value);
        apdu_len += len;
        if (apdu) {
            apdu_offset = &apdu[apdu_len];
        }
        len = encode_closing_tag(apdu_offset, tag_number);
        apdu_len += len;
    }

    return apdu_len;
}

/**
 * @brief Decode the BACnetColorCommand complex data
 *
 *  BACnetColorCommand ::= SEQUENCE {
 *      operation      [0] BACnetColorOperation,
 *      target-color   [1] BACnetxyColor OPTIONAL,
 *      target-color-temperature [2] Unsigned OPTIONAL,
 *      fade-time      [3] Unsigned (100.. 86400000) OPTIONAL,
 *      ramp-rate      [4] Unsigned (1..30000) OPTIONAL,
 *      step-increment [5] Unsigned (1..30000) OPTIONAL
 *  }
 *
 * @param apdu - the APDU buffer
 * @param apdu_len - the APDU buffer length
 * @param value - BACnetColorCommand structure values
 * @return length of the APDU buffer decoded, or ERROR, REJECT, or ABORT
 */
int color_command_decode(uint8_t *apdu,
    uint16_t apdu_size,
    BACNET_ERROR_CODE *error_code,
    BACNET_COLOR_COMMAND *value)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_COLOR_OPERATION operation = BACNET_COLOR_OPERATION_NONE;
    BACNET_XY_COLOR color;

    /* default reject code */
    if (error_code) {
        *error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
    }
    /* check for value pointers */
    if ((apdu_size == 0) || (!apdu)) {
        return BACNET_STATUS_REJECT;
    }
    /* operation [0] BACnetColorOperation */
    len = bacnet_unsigned_context_decode(
        apdu, apdu_size - apdu_len, 0, &unsigned_value);
    if (len <= 0) {
        if (len == 0) {
            if (error_code) {
                *error_code = ERROR_CODE_REJECT_INVALID_TAG;
            }
        } else {
            if (error_code) {
                *error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
            }
        }
        return BACNET_STATUS_REJECT;
    }
    apdu_len += len;
    if (unsigned_value >= BACNET_COLOR_OPERATION_MAX) {
        if (error_code) {
            *error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
        }
        return BACNET_STATUS_REJECT;
    }
    operation = (BACNET_COLOR_OPERATION)unsigned_value;
    if (value) {
        value->operation = operation;
    }
    switch (operation) {
        case BACNET_COLOR_OPERATION_NONE:
            break;
        case BACNET_COLOR_OPERATION_FADE_TO_COLOR:
            /* target-color [1] BACnetxyColor */
            if ((apdu_size - apdu_len) == 0) {
                if (error_code) {
                    *error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
                }
                return BACNET_STATUS_REJECT;
            }
            len = xy_color_context_decode(
                &apdu[apdu_len], apdu_size - apdu_len, 1, &color);
            if (len == 0) {
                if (error_code) {
                    *error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
                }
                return BACNET_STATUS_REJECT;
            }
            apdu_len += len;
            if (value) {
                value->target.color.x_coordinate = color.x_coordinate;
                value->target.color.y_coordinate = color.y_coordinate;
            }
            if ((apdu_size - apdu_len) != 0) {
                /* fade-time [3] Unsigned (100.. 86400000) OPTIONAL */
                len = bacnet_unsigned_context_decode(
                    &apdu[apdu_len], apdu_size - apdu_len, 3, &unsigned_value);
                if (len <= 0) {
                    if (len == 0) {
                        if (error_code) {
                            *error_code = ERROR_CODE_REJECT_INVALID_TAG;
                        }
                    } else {
                        if (error_code) {
                            *error_code =
                                ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
                        }
                    }
                    return BACNET_STATUS_REJECT;
                }
                apdu_len += len;
                if ((unsigned_value < BACNET_COLOR_FADE_TIME_MIN) ||
                    (unsigned_value > BACNET_COLOR_FADE_TIME_MAX)) {
                    if (error_code) {
                        *error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
                    }
                    return BACNET_STATUS_REJECT;
                }
                if (value) {
                    value->transit.fade_time = unsigned_value;
                }
            }
            break;
        case BACNET_COLOR_OPERATION_FADE_TO_CCT:
            /* target-color-temperature [2] Unsigned */
            if ((apdu_size - apdu_len) == 0) {
                if (error_code) {
                    *error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
                }
                return BACNET_STATUS_REJECT;
            }
            len = bacnet_unsigned_context_decode(
                apdu, apdu_size - apdu_len, 2, &unsigned_value);
            if (len <= 0) {
                if (len == 0) {
                    if (error_code) {
                        *error_code = ERROR_CODE_REJECT_INVALID_TAG;
                    }
                } else {
                    if (error_code) {
                        *error_code =
                            ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
                    }
                }
                return BACNET_STATUS_REJECT;
            }
            apdu_len += len;
            if (unsigned_value > UINT16_MAX) {
                if (error_code) {
                    *error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
                }
                return BACNET_STATUS_REJECT;
            }
            if (value) {
                value->target.color_temperature = unsigned_value;
            }
            if ((apdu_size - apdu_len) != 0) {
                /* fade-time [3] Unsigned (100.. 86400000) OPTIONAL */
                len = bacnet_unsigned_context_decode(
                    &apdu[apdu_len], apdu_size - apdu_len, 3, &unsigned_value);
                if (len <= 0) {
                    if (len == 0) {
                        if (error_code) {
                            *error_code = ERROR_CODE_REJECT_INVALID_TAG;
                        }
                    } else {
                        if (error_code) {
                            *error_code =
                                ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
                        }
                    }
                    return BACNET_STATUS_REJECT;
                }
                apdu_len += len;
                if ((unsigned_value < BACNET_COLOR_FADE_TIME_MIN) ||
                    (unsigned_value > BACNET_COLOR_FADE_TIME_MAX)) {
                    if (error_code) {
                        *error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
                    }
                    return BACNET_STATUS_REJECT;
                }
                if (value) {
                    value->transit.fade_time = unsigned_value;
                }
            }
            break;
        case BACNET_COLOR_OPERATION_RAMP_TO_CCT:
            /* target-color-temperature [2] Unsigned */
            if ((apdu_size - apdu_len) == 0) {
                if (error_code) {
                    *error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
                }
                return BACNET_STATUS_REJECT;
            }
            len = bacnet_unsigned_context_decode(
                apdu, apdu_size - apdu_len, 2, &unsigned_value);
            if (len <= 0) {
                if (len == 0) {
                    if (error_code) {
                        *error_code = ERROR_CODE_REJECT_INVALID_TAG;
                    }
                } else {
                    if (error_code) {
                        *error_code =
                            ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
                    }
                }
                return BACNET_STATUS_REJECT;
            }
            apdu_len += len;
            if (unsigned_value > UINT16_MAX) {
                if (error_code) {
                    *error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
                }
                return BACNET_STATUS_REJECT;
            }
            if (value) {
                value->target.color_temperature = unsigned_value;
            }
            if ((apdu_size - apdu_len) != 0) {
                /* ramp-rate      [4] Unsigned (1..30000) */
                len = bacnet_unsigned_context_decode(
                    &apdu[apdu_len], apdu_size - apdu_len, 4, &unsigned_value);
                if (len <= 0) {
                    if (len == 0) {
                        if (error_code) {
                            *error_code = ERROR_CODE_REJECT_INVALID_TAG;
                        }
                    } else {
                        if (error_code) {
                            *error_code =
                                ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
                        }
                    }
                    return BACNET_STATUS_REJECT;
                }
                apdu_len += len;
                if ((unsigned_value < BACNET_COLOR_RAMP_RATE_MIN) ||
                    (unsigned_value > BACNET_COLOR_RAMP_RATE_MAX)) {
                    if (error_code) {
                        *error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
                    }
                    return BACNET_STATUS_REJECT;
                }
                if (value) {
                    value->transit.ramp_rate = unsigned_value;
                }
            }
            break;
        case BACNET_COLOR_OPERATION_STEP_UP_CCT:
        case BACNET_COLOR_OPERATION_STEP_DOWN_CCT:
            /* step-increment [5] Unsigned (1..30000) */
            if ((apdu_size - apdu_len) == 0) {
                if (error_code) {
                    *error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
                }
                return BACNET_STATUS_REJECT;
            }
            len = bacnet_unsigned_context_decode(
                apdu, apdu_size - apdu_len, 3, &unsigned_value);
            if (len <= 0) {
                if (len == 0) {
                    if (error_code) {
                        *error_code = ERROR_CODE_REJECT_INVALID_TAG;
                    }
                } else {
                    if (error_code) {
                        *error_code =
                            ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
                    }
                }
                return BACNET_STATUS_REJECT;
            }
            apdu_len += len;
            if ((unsigned_value < BACNET_COLOR_STEP_INCREMENT_MIN) ||
                (unsigned_value > BACNET_COLOR_STEP_INCREMENT_MAX)) {
                if (error_code) {
                    *error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
                }
                return BACNET_STATUS_REJECT;
            }
            if (value) {
                value->transit.step_increment = unsigned_value;
            }
            break;
        case BACNET_COLOR_OPERATION_STOP:
            break;
        default:
            break;
    }

    return apdu_len;
}

/**
 * @brief Copy the BACnetColorCommand complex data from src to dst
 * @param dst - destination structure
 * @param src - source structure
 * @return true if successfully copied
 */
bool color_command_copy(BACNET_COLOR_COMMAND *dst, BACNET_COLOR_COMMAND *src)
{
    bool status = false;

    if (dst && src) {
        memcpy(dst, src, sizeof(BACNET_COLOR_COMMAND));
        status = true;
    }

    return status;
}

/**
 * @brief Compare the BACnetColorCommand complex data
 * @param value1 - BACNET_COLOR_COMMAND structure
 * @param value2 - BACNET_COLOR_COMMAND structure
 * @return true if the same
 */
bool color_command_same(
    BACNET_COLOR_COMMAND *value1, BACNET_COLOR_COMMAND *value2)
{
    bool status = false;

    if (value1 && value2 && (value1->operation == value2->operation)) {
        switch (value1->operation) {
            case BACNET_COLOR_OPERATION_NONE:
                status = true;
                break;
            case BACNET_COLOR_OPERATION_FADE_TO_COLOR:
                if ((value1->target.color.x_coordinate ==
                        value2->target.color.x_coordinate) &&
                    (value1->target.color.y_coordinate ==
                        value2->target.color.y_coordinate) &&
                    (value1->transit.fade_time == value2->transit.fade_time)) {
                    status = true;
                }
                break;
            case BACNET_COLOR_OPERATION_FADE_TO_CCT:
                if ((value1->target.color_temperature ==
                        value2->target.color_temperature) &&
                    (value1->transit.fade_time == value2->transit.fade_time)) {
                    status = true;
                }
                break;
            case BACNET_COLOR_OPERATION_RAMP_TO_CCT:
                if ((value1->target.color_temperature ==
                        value2->target.color_temperature) &&
                    (value1->transit.ramp_rate == value2->transit.ramp_rate)) {
                    status = true;
                }
                break;
            case BACNET_COLOR_OPERATION_STEP_UP_CCT:
            case BACNET_COLOR_OPERATION_STEP_DOWN_CCT:
                if (value1->transit.step_increment ==
                    value2->transit.step_increment) {
                    status = true;
                }
                break;
            case BACNET_COLOR_OPERATION_STOP:
                status = true;
                break;
            default:
                break;
        }
    }

    return status;
}
