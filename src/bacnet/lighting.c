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
 * @return  number of bytes encoded
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
    float real_value = 0.0;

    (void)apdu_max_len;
    /* check for value pointers */
    if (apdu_max_len && data) {
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
                data->operation = (BACNET_LIGHTING_OPERATION)enum_value;
            } else {
                return BACNET_STATUS_ERROR;
            }
        }
        apdu_len += len;
        /* Tag 1: target-level - OPTIONAL */
        if (decode_is_context_tag(&apdu[apdu_len], 1)) {
            len = decode_tag_number_and_value(
                &apdu[apdu_len], &tag_number, &len_value_type);
            apdu_len += len;
            len = decode_real(&apdu[apdu_len], &real_value);
            data->target_level = real_value;
            apdu_len += len;
            data->use_target_level = true;
        } else {
            data->use_target_level = false;
        }
        /* Tag 2: ramp-rate - OPTIONAL */
        if (decode_is_context_tag(&apdu[apdu_len], 2)) {
            len = decode_tag_number_and_value(
                &apdu[apdu_len], &tag_number, &len_value_type);
            apdu_len += len;
            len = decode_real(&apdu[apdu_len], &real_value);
            data->ramp_rate = real_value;
            data->use_ramp_rate = true;
        } else {
            data->use_ramp_rate = false;
        }
        /* Tag 3: step-increment - OPTIONAL */
        if (decode_is_context_tag(&apdu[apdu_len], 3)) {
            len = decode_tag_number_and_value(
                &apdu[apdu_len], &tag_number, &len_value_type);
            apdu_len += len;
            len = decode_real(&apdu[apdu_len], &real_value);
            data->step_increment = real_value;
            data->use_step_increment = true;
        } else {
            data->use_step_increment = false;
        }
        /* Tag 4: fade-time - OPTIONAL */
        if (decode_is_context_tag(&apdu[apdu_len], 4)) {
            len = decode_tag_number_and_value(
                &apdu[apdu_len], &tag_number, &len_value_type);
            apdu_len += len;
            len = decode_unsigned(
                &apdu[apdu_len], len_value_type, &unsigned_value);
            data->fade_time = (uint32_t)unsigned_value;
            data->use_fade_time = true;
        } else {
            data->use_fade_time = false;
        }
        /* Tag 5: priority - OPTIONAL */
        if (decode_is_context_tag(&apdu[apdu_len], 4)) {
            len = decode_tag_number_and_value(
                &apdu[apdu_len], &tag_number, &len_value_type);
            apdu_len += len;
            len = decode_unsigned(
                &apdu[apdu_len], len_value_type, &unsigned_value);
            data->priority = (uint8_t)unsigned_value;
            data->use_priority = true;
        } else {
            data->use_priority = false;
        }
    }

    return len;
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
    BACNET_XY_COLOR color;

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

/**
 * @brief Convert sRGB to CIE xy
 * @param r - R value of sRGB
 * @param g - G value of sRGB
 * @param b - B value of sRGB
 * @param x_coordinate - return x of CIE xy
 * @param y_coordinate - return y of CIE xy
 * @param brightness - return brightness of the CIE xy color
 * @note Taken from Philips Hue Application Design Notes
 *  RGB to xy Color conversion
 *  See also: http://en.wikipedia.org/wiki/Srgb
 */
void color_rgb_to_xy(uint8_t r, uint8_t g, uint8_t b,
    float *x_coordinate, float *y_coordinate, float *brightness)
{
    /*  Color to xy
        We start with the color to xy conversion,
        which we will do in a couple of steps:

        1. Get the RGB values from your color object
        and convert them to be between 0 and 1.
        So the RGB color (255, 0, 100) becomes (1.0, 0.0, 0.39) */
    float red = (float)r;
    float green = (float)g;
    float blue = (float)b;

    red /= 255.0;
    green /= 255.0;
    blue /= 255.0;

    /* Apply a gamma correction to the RGB values,
       which makes the color more vivid and more the
       like the color displayed on the screen of your device.
       This gamma correction is also applied to the screen
       of your computer or phone, thus we need this to create
       the same color on the light as on screen. */
    red = (red > 0.04045f) ?
        pow((red + 0.055f) / (1.0f + 0.055f), 2.4f) :
        (red / 12.92f);
    green = (green > 0.04045f) ?
        pow((green + 0.055f) / (1.0f + 0.055f), 2.4f) :
        (green / 12.92f);
    blue = (blue > 0.04045f) ?
        pow((blue + 0.055f) / (1.0f + 0.055f), 2.4f) :
        (blue / 12.92f);

    /*  Convert the RGB values to XYZ using the
        Wide RGB D65 conversion formula */
    float X = red * 0.649926f + green * 0.103455f + blue * 0.197109f;
    float Y = red * 0.234327f + green * 0.743075f + blue * 0.022598f;
    float Z = red * 0.0000000f + green * 0.053077f + blue * 1.035763f;

    /* Calculate the xy values from the XYZ values */
    float x = X / (X + Y + Z);
    float y = Y / (X + Y + Z);

    if (x_coordinate) {
        *x_coordinate = x;
    }
    if (y_coordinate) {
        *y_coordinate = y;
    }
    /*  note:
        Check if the found xy value is within the
        color gamut of the light, if not continue with step 6,
        otherwise step 7 When we sent a value which the light
        is not capable of, the resulting color might not be optimal.
        Therefor we try to only sent values which are inside
        the color gamut of the selected light.

        Calculate the closest point on the color gamut triangle
        and use that as xy value.

        The closest value is calculated by making a perpendicular line
        to one of the lines the triangle consists of and when it is
        then still not inside the triangle, we choose the closest
        corner point of the triangle. */

    /*  Use the Y value of XYZ as brightness
        The Y value indicates the brightness
        of the converted color. */
    if (brightness) {
        *brightness = Y;
    }
}

/**
 * @brief Convert sRGB from CIE xy and brightness
 * @param red - return R value of sRGB
 * @param green - return G value of sRGB
 * @param blue - return B value of sRGB
 * @param x_coordinate - x of CIE xy
 * @param y_coordinate - y of CIE xy
 * @param brightness - brightness of the CIE xy color
 * @note Taken from Philips Hue Application Design Notes
 *  RGB to xy Color conversion
 *  See also: http://en.wikipedia.org/wiki/Srgb
 */
void color_rgb_from_xy(uint8_t *red, uint8_t *green, uint8_t *blue,
    float x_coordinate, float y_coordinate, float brightness)
{
    /*  xy to RGB color
        The xy to color conversion is almost the same,
        but in reverse order.

        Check if the xy value is within the color gamut of the lamp,
        if not continue with step 2, otherwise step 3
        We do this to calculate the most accurate color
        that the given lamp can actually do.

        Calculate the closest point on the color
        gamut triangle and use that as xy value
        See step 6 of color to xy. */

    /*  Calculate XYZ values */
    float x = x_coordinate;
    float y = y_coordinate;
    float z = 1.0f - x - y;
    float Y = brightness;
    float X = (Y / y) * x;
    float Z = (Y / y) * z;

    /*  Convert to RGB using Wide RGB D65 conversion
       (THIS IS A D50 conversion currently) */
    float r = X * 1.4628067f - Y * 0.1840623f - Z * 0.2743606f;
    float g = -X * 0.5217933f + Y * 1.4472381f + Z * 0.0677227f;
    float b = X * 0.0349342f - Y * 0.0968930f + Z * 1.2884099f;

    /*  Apply reverse gamma correction */
    r = r <= 0.0031308f ? 12.92f * r :
        (1.0f + 0.055f) * pow(r, (1.0f / 2.4f)) - 0.055f;
    g = g <= 0.0031308f ? 12.92f * g :
        (1.0f + 0.055f) * pow(g, (1.0f / 2.4f)) - 0.055f;
    b = b <= 0.0031308f ? 12.92f * b :
        (1.0f + 0.055f) * pow(b, (1.0f / 2.4f)) - 0.055f;

    /*  Convert the RGB values to your color object
        The rgb values from the above formulas are
        between 0.0 and 1.0. */
    if (red) {
        *red = (uint8_t)(r*255.0);
    }
    if (green) {
        *green = (uint8_t)(g*255.0);
    }
    if (blue) {
        *blue = (uint8_t)(b*255.0);
    }
}

/* table for converting RGB to and from ASCII color names */
struct css_color_rgb {
    const char *name;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};
static struct css_color_rgb CSS_Color_RGB_Table[] = {
    {"aliceblue",  240, 248, 255},
    {"antiquewhite", 250, 235, 215},
    {"aqua", 0, 255, 255},
    {"aquamarine", 127, 255, 212},
    {"azure", 240, 255, 255},
    {"beige", 245, 245, 220},
    {"bisque", 255, 228, 196},
    {"black", 0, 0, 0},
    {"blanchedalmond", 255, 235, 205},
    {"blue", 0, 0, 255},
    {"blueviolet", 138, 43, 226},
    {"brown", 165, 42, 42},
    {"burlywood", 222, 184, 135},
    {"cadetblue", 95, 158, 160},
    {"chartreuse", 127, 255, 0},
    {"chocolate", 210, 105, 30},
    {"coral", 255, 127, 80},
    {"cornflowerblue", 100, 149, 237},
    {"cornsilk", 255, 248, 220},
    {"crimson", 220, 20, 60},
    {"cyan", 0, 255, 255},
    {"darkblue", 0, 0, 139},
    {"darkcyan", 0, 139, 139},
    {"darkgoldenrod", 184, 134, 11},
    {"darkgray", 169, 169, 169},
    {"darkgreen", 0, 100, 0},
    {"darkgrey", 169, 169, 169},
    {"darkkhaki", 189, 183, 107},
    {"darkmagenta", 139, 0, 139},
    {"darkolivegreen", 85, 107, 47},
    {"darkorange", 255, 140, 0},
    {"darkorchid", 153, 50, 204},
    {"darkred", 139, 0, 0},
    {"darksalmon", 233, 150, 122},
    {"darkseagreen", 143, 188, 143},
    {"darkslateblue", 72, 61, 139},
    {"darkslategray", 47, 79, 79},
    {"darkslategrey", 47, 79, 79},
    {"darkturquoise", 0, 206, 209},
    {"darkviolet", 148, 0, 211},
    {"deeppink", 255, 20, 147},
    {"deepskyblue", 0, 191, 255},
    {"dimgray", 105, 105, 105},
    {"dimgrey", 105, 105, 105},
    {"dodgerblue", 30, 144, 255},
    {"firebrick", 178, 34, 34},
    {"floralwhite", 255, 250, 240},
    {"forestgreen", 34, 139, 34},
    {"fuchsia", 255, 0, 255},
    {"gainsboro", 220, 220, 220},
    {"ghostwhite", 248, 248, 255},
    {"gold", 255, 215, 0},
    {"goldenrod", 218, 165, 32},
    {"gray", 128, 128, 128},
    {"green", 0, 128, 0},
    {"greenyellow", 173, 255, 47},
    {"grey", 128, 128, 128},
    {"honeydew", 240, 255, 240},
    {"hotpink", 255, 105, 180},
    {"indianred", 205, 92, 92},
    {"indigo", 75, 0, 130},
    {"ivory", 255, 255, 240},
    {"khaki", 240, 230, 140},
    {"lavender", 230, 230, 250},
    {"lavenderblush", 255, 240, 245},
    {"lawngreen", 124, 252, 0},
    {"lemonchiffon", 255, 250, 205},
    {"lightblue", 173, 216, 230},
    {"lightcoral", 240, 128, 128},
    {"lightcyan", 224, 255, 255},
    {"lightgoldenrodyellow", 250, 250, 210},
    {"lightgray", 211, 211, 211},
    {"lightgreen", 144, 238, 144},
    {"lightgrey", 211, 211, 211},
    {"lightpink", 255, 182, 193},
    {"lightsalmon", 255, 160, 122},
    {"lightseagreen", 32, 178, 170},
    {"lightskyblue", 135, 206, 250},
    {"lightslategray", 119, 136, 153},
    {"lightslategrey", 119, 136, 153},
    {"lightsteelblue", 176, 196, 222},
    {"lightyellow", 255, 255, 224},
    {"lime", 0, 255, 0},
    {"limegreen", 50, 205, 50},
    {"linen", 250, 240, 230},
    {"magenta", 255, 0, 255},
    {"maroon", 128, 0, 0},
    {"mediumaquamarine", 102, 205, 170},
    {"mediumblue", 0, 0, 205},
    {"mediumorchid", 186, 85, 211},
    {"mediumpurple", 147, 112, 219},
    {"mediumseagreen", 60, 179, 113},
    {"mediumslateblue", 123, 104, 238},
    {"mediumspringgreen", 0, 250, 154},
    {"mediumturquoise", 72, 209, 204},
    {"mediumvioletred", 199, 21, 133},
    {"midnightblue", 25, 25, 112},
    {"mintcream", 245, 255, 250},
    {"mistyrose", 255, 228, 225},
    {"moccasin", 255, 228, 181},
    {"navajowhite", 255, 222, 173},
    {"navy", 0, 0, 128},
    {"navyblue", 0, 0, 128},
    {"oldlace", 253, 245, 230},
    {"olive", 128, 128, 0},
    {"olivedrab", 107, 142, 35},
    {"orange", 255, 165, 0},
    {"orangered", 255, 69, 0},
    {"orchid", 218, 112, 214},
    {"palegoldenrod", 238, 232, 170},
    {"palegreen", 152, 251, 152},
    {"paleturquoise", 175, 238, 238},
    {"palevioletred", 219, 112, 147},
    {"papayawhip", 255, 239, 213},
    {"peachpuff", 255, 218, 185},
    {"peru", 205, 133, 63},
    {"pink", 255, 192, 203},
    {"plum", 221, 160, 221},
    {"powderblue", 176, 224, 230},
    {"purple", 128, 0, 128},
    {"red", 255, 0, 0},
    {"rosybrown", 188, 143, 143},
    {"royalblue", 65, 105, 225},
    {"saddlebrown", 139, 69, 19},
    {"salmon", 250, 128, 114},
    {"sandybrown", 244, 164, 96},
    {"seagreen", 46, 139, 87},
    {"seashell", 255, 245, 238},
    {"sienna", 160, 82, 45},
    {"silver", 192, 192, 192},
    {"skyblue", 135, 206, 235},
    {"slateblue", 106, 90, 205},
    {"slategray", 112, 128, 144},
    {"slategrey", 112, 128, 144},
    {"snow", 255, 250, 250},
    {"springgreen", 0, 255, 127},
    {"steelblue", 70, 130, 180},
    {"tan", 210, 180, 140},
    {"teal", 0, 128, 128},
    {"thistle", 216, 191, 216},
    {"tomato", 255, 99, 71},
    {"turquoise", 64, 224, 208},
    {"violet", 238, 130, 238},
    {"wheat", 245, 222, 179},
    {"white", 255, 255, 255},
    {"whitesmoke", 245, 245, 245},
    {"yellow", 255, 255, 0},
    {"yellowgreen", 154, 205, 50},
    {NULL, 0, 0, 0}
};

/**
 * @brief Convert sRGB from CIE xy and brightness
 * @param red - return R value of sRGB
 * @param green - return G value of sRGB
 * @param blue - return B value of sRGB
 * @param name - CSS color name from W3C
 * @note Official CSS3 colors from w3.org:
 *  https://www.w3.org/TR/2010/PR-css3-color-20101028/#html4
 *  names do not have spaces
 */
const char * color_rgb_to_ascii(uint8_t red, uint8_t green, uint8_t blue)
{
    const char * name = "";
    unsigned index = 0;

    while (CSS_Color_RGB_Table[index].name) {
        if ((red == CSS_Color_RGB_Table[index].red) &&
            (green == CSS_Color_RGB_Table[index].green) &&
            (blue == CSS_Color_RGB_Table[index].blue)) {
            return CSS_Color_RGB_Table[index].name;
        }
        index++;
    };

    return name;
}

/**
 * @brief Convert sRGB from CIE xy and brightness
 * @param red - return R value of sRGB
 * @param green - return G value of sRGB
 * @param blue - return B value of sRGB
 * @param name - CSS color name from W3C
 * @note Official CSS3 colors from w3.org:
 *  https://www.w3.org/TR/2010/PR-css3-color-20101028/#html4
 *  names do not have spaces
 */
bool color_rgb_from_ascii(uint8_t *red, uint8_t *green, uint8_t *blue,
    const char *name)
{
    bool status = false;
    unsigned index = 0;

    while (CSS_Color_RGB_Table[index].name) {
        if (strcmp(CSS_Color_RGB_Table[index].name, name) == 0) {
            if (red) {
                *red = CSS_Color_RGB_Table[index].red;
            }
            if (green) {
                *green = CSS_Color_RGB_Table[index].green;
            }
            if (blue) {
                *blue = CSS_Color_RGB_Table[index].blue;
            }
            status = true;
            break;
        }
        index++;
    };

    return status;
}
