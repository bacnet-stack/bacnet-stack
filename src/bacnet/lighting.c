/**
 * @file
 * @brief API for BACnetLightingCommand and BACnetColorCommand
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date June 2022
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
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

/** @file lighting.c  Manipulate BACnet lighting command values */

/**
 * Encodes into bytes from the lighting-command structure
 *
 *  BACnetLightingCommand ::= SEQUENCE {
 *      operation [0] BACnetLightingOperation,
 *      target-level [1] REAL (0.0..100.0) OPTIONAL,
 *      ramp-rate [2] REAL (0.1..100.0) OPTIONAL,
 *      step-increment [3] REAL (0.1..100.0) OPTIONAL,
 *      fade-time [4] Unsigned (100..86400000) OPTIONAL,
 *      priority [5] Unsigned (1..16) OPTIONAL
 *  }
 *  -- Note that the combination of level, ramp-rate,
 *  --  step-increment, and fade-time fields is
 *  --  dependent on the specific lighting operation.
 *
 * @param apdu - buffer to hold the bytes, or null for length
 * @param value - lighting command value to encode
 *
 * @return  number of bytes encoded
 */
int lighting_command_encode(uint8_t *apdu, const BACNET_LIGHTING_COMMAND *data)
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
 *  BACnetLightingCommand ::= SEQUENCE {
 *      operation [0] BACnetLightingOperation,
 *      target-level [1] REAL (0.0..100.0) OPTIONAL,
 *      ramp-rate [2] REAL (0.1..100.0) OPTIONAL,
 *      step-increment [3] REAL (0.1..100.0) OPTIONAL,
 *      fade-time [4] Unsigned (100..86400000) OPTIONAL,
 *      priority [5] Unsigned (1..16) OPTIONAL
 *  }
 *  -- Note that the combination of level, ramp-rate,
 *  --  step-increment, and fade-time fields is
 *  --  dependent on the specific lighting operation.
 *
 * @param apdu - buffer to hold the bytes
 * @param tag_number - tag number to encode this chunk
 * @param value - lighting command value to encode
 *
 * @return  number of bytes encoded, or 0 if unable to encode.
 */
int lighting_command_encode_context(
    uint8_t *apdu, uint8_t tag_number, const BACNET_LIGHTING_COMMAND *value)
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
 *  BACnetLightingCommand ::= SEQUENCE {
 *      operation [0] BACnetLightingOperation,
 *      target-level [1] REAL (0.0..100.0) OPTIONAL,
 *      ramp-rate [2] REAL (0.1..100.0) OPTIONAL,
 *      step-increment [3] REAL (0.1..100.0) OPTIONAL,
 *      fade-time [4] Unsigned (100..86400000) OPTIONAL,
 *      priority [5] Unsigned (1..16) OPTIONAL
 *  }
 *  -- Note that the combination of level, ramp-rate,
 *  --  step-increment, and fade-time fields is
 *  --  dependent on the specific lighting operation.
 *
 * @param apdu - buffer to hold the bytes
 * @param apdu_size - number of bytes in the buffer to decode
 * @param value - lighting command value to place the decoded values
 *
 * @return  number of bytes decoded, or BACNET_STATUS_ERROR
 */
int lighting_command_decode(
    const uint8_t *apdu, unsigned apdu_size, BACNET_LIGHTING_COMMAND *data)
{
    int len = 0;
    int apdu_len = 0;
    uint32_t enum_value = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    BACNET_LIGHTING_OPERATION operation = BACNET_LIGHTS_NONE;
    float real_value = 0.0;

    /* check for value pointers */
    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    /* operation [0] BACnetLightingOperation */
    len = bacnet_enumerated_context_decode(
        &apdu[apdu_len], apdu_size - apdu_len, 0, &enum_value);
    if (len > 0) {
        apdu_len += len;
        if (unsigned_value <= BACNET_LIGHTS_PROPRIETARY_LAST) {
            operation = (BACNET_LIGHTING_OPERATION)enum_value;
            if (data) {
                data->operation = operation;
                data->use_target_level = false;
                data->use_ramp_rate = false;
                data->use_step_increment = false;
                data->use_fade_time = false;
                data->use_priority = false;
            }
        } else {
            return BACNET_STATUS_ERROR;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    switch (operation) {
        case BACNET_LIGHTS_NONE:
            break;
        case BACNET_LIGHTS_FADE_TO:
            if ((apdu_size - apdu_len) == 0) {
                return BACNET_STATUS_REJECT;
            }
            /* target-level [1] REAL (0.0..100.0) OPTIONAL */
            len = bacnet_real_context_decode(
                &apdu[apdu_len], apdu_size, 1, &real_value);
            if (len > 0) {
                apdu_len += len;
                if (data) {
                    data->target_level = real_value;
                    data->use_target_level = true;
                }
            }
            if ((apdu_size - apdu_len) > 0) {
                /* Tag 4: fade-time - OPTIONAL */
                len = bacnet_unsigned_context_decode(
                    &apdu[apdu_len], apdu_size, 4, &unsigned_value);
                if (len > 0) {
                    apdu_len += len;
                    if (data) {
                        data->fade_time = (uint32_t)unsigned_value;
                        data->use_fade_time = true;
                    }
                } else {
                    return BACNET_STATUS_ERROR;
                }
            }
            if ((apdu_size - apdu_len) > 0) {
                /* priority [5] Unsigned (1..16) OPTIONAL */
                len = bacnet_unsigned_context_decode(
                    &apdu[apdu_len], apdu_size, 5, &unsigned_value);
                if (len > 0) {
                    apdu_len += len;
                    if (data) {
                        data->priority = (uint8_t)unsigned_value;
                        data->use_priority = true;
                    }
                }
            }
            break;
        case BACNET_LIGHTS_RAMP_TO:
            if ((apdu_size - apdu_len) == 0) {
                return BACNET_STATUS_REJECT;
            }
            /* target-level [1] REAL (0.0..100.0) OPTIONAL */
            len = bacnet_real_context_decode(
                &apdu[apdu_len], apdu_size, 1, &real_value);
            if (len > 0) {
                apdu_len += len;
                if (data) {
                    data->target_level = real_value;
                    data->use_target_level = true;
                }
            }
            if ((apdu_size - apdu_len) > 0) {
                /* ramp-rate [2] REAL (0.1..100.0) OPTIONAL */
                len = bacnet_real_context_decode(
                    &apdu[apdu_len], apdu_size, 2, &real_value);
                if (len > 0) {
                    apdu_len += len;
                    if (data) {
                        data->ramp_rate = real_value;
                        data->use_ramp_rate = true;
                    }
                }
            }
            if ((apdu_size - apdu_len) > 0) {
                /* priority [5] Unsigned (1..16) OPTIONAL */
                len = bacnet_unsigned_context_decode(
                    &apdu[apdu_len], apdu_size, 5, &unsigned_value);
                if (len > 0) {
                    apdu_len += len;
                    if (data) {
                        data->priority = (uint8_t)unsigned_value;
                        data->use_priority = true;
                    }
                }
            }
            break;
        case BACNET_LIGHTS_STEP_UP:
        case BACNET_LIGHTS_STEP_DOWN:
        case BACNET_LIGHTS_STEP_ON:
        case BACNET_LIGHTS_STEP_OFF:
            if ((apdu_size - apdu_len) > 0) {
                /* step-increment [3] REAL (0.1..100.0) OPTIONAL */
                len = bacnet_real_context_decode(
                    &apdu[apdu_len], apdu_size, 3, &real_value);
                if (len > 0) {
                    apdu_len += len;
                    if (data) {
                        data->step_increment = real_value;
                        data->use_step_increment = true;
                    }
                }
            }
            if ((apdu_size - apdu_len) > 0) {
                /* priority [5] Unsigned (1..16) OPTIONAL */
                len = bacnet_unsigned_context_decode(
                    &apdu[apdu_len], apdu_size, 5, &unsigned_value);
                if (len > 0) {
                    apdu_len += len;
                    if (data) {
                        data->priority = (uint8_t)unsigned_value;
                        data->use_priority = true;
                    }
                }
            }
            break;
        case BACNET_LIGHTS_WARN:
        case BACNET_LIGHTS_WARN_OFF:
        case BACNET_LIGHTS_WARN_RELINQUISH:
        case BACNET_LIGHTS_STOP:
            if ((apdu_size - apdu_len) > 0) {
                /* priority [5] Unsigned (1..16) OPTIONAL */
                len = bacnet_unsigned_context_decode(
                    &apdu[apdu_len], apdu_size, 5, &unsigned_value);
                if (len > 0) {
                    apdu_len += len;
                    if (data) {
                        data->priority = (uint8_t)unsigned_value;
                        data->use_priority = true;
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
    BACNET_LIGHTING_COMMAND *dst, const BACNET_LIGHTING_COMMAND *src)
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
    const BACNET_LIGHTING_COMMAND *dst, const BACNET_LIGHTING_COMMAND *src)
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
 * @brief Convert BACnetLightingCommand to ASCII for printing
 * @param value - struct to convert to ASCII
 * @param buf - ASCII output buffer (or NULL for size)
 * @param buf_size - ASCII output buffer capacity (or 0 for size)
 *
 * @return the number of characters which would be generated for the given
 *  input, excluding the trailing null. negative is returned if the
 *  capacity was not sufficient.
 *
 * @note buf and buf_size may be null and zero to return only the size
 */
int lighting_command_to_ascii(
    const BACNET_LIGHTING_COMMAND *value, char *buf, size_t buf_size)
{
    int len = 0;
    float target_level = -1.0F;
    float ramp_rate = 0.0F;
    float step_increment = 0.0;
    uint32_t fade_time = 0;
    uint8_t priority = BACNET_NO_PRIORITY;

    if (!value) {
        return 0;
    }
    switch (value->operation) {
        case BACNET_LIGHTS_NONE:
            len = snprintf(buf, buf_size, "%u", (unsigned)value->operation);
            break;
        case BACNET_LIGHTS_FADE_TO:
            if (value->use_target_level) {
                target_level = value->target_level;
            }
            if (value->use_fade_time) {
                fade_time = value->fade_time;
            }
            if (value->use_priority) {
                priority = value->priority;
            }
            len = snprintf(
                buf, buf_size, "%u,%f,%lu,%u", value->operation,
                (double)target_level, (unsigned long)fade_time,
                (unsigned)priority);
            break;
        case BACNET_LIGHTS_RAMP_TO:
            if (value->use_target_level) {
                target_level = value->target_level;
            }
            if (value->use_ramp_rate) {
                ramp_rate = value->ramp_rate;
            }
            if (value->use_priority) {
                priority = value->priority;
            }
            len = snprintf(
                buf, buf_size, "%u,%f,%f,%u", (unsigned)value->operation,
                (double)target_level, (double)ramp_rate, (unsigned)priority);
            break;
        case BACNET_LIGHTS_STEP_UP:
        case BACNET_LIGHTS_STEP_DOWN:
        case BACNET_LIGHTS_STEP_ON:
        case BACNET_LIGHTS_STEP_OFF:
            if (value->use_step_increment) {
                step_increment = value->step_increment;
            }
            if (value->use_priority) {
                priority = value->priority;
            }
            len = snprintf(
                buf, buf_size, "%u,%f,%u", (unsigned)value->operation,
                (double)step_increment, (unsigned)priority);
            break;
        case BACNET_LIGHTS_WARN:
        case BACNET_LIGHTS_WARN_OFF:
        case BACNET_LIGHTS_WARN_RELINQUISH:
        case BACNET_LIGHTS_STOP:
            if (value->use_priority) {
                priority = value->priority;
            }
            len = snprintf(
                buf, buf_size, "%u,%u", (unsigned)value->operation,
                (unsigned)priority);
            break;
        default:
            len = snprintf(buf, buf_size, "%u", (unsigned)value->operation);
            break;
    }

    return len;
}

/**
 * @brief Parse an ASCII string for a BACnetLightingCommand
 * @param value [out] BACnetLightingCommand structure to store the results
 * @param argv [in] nul terminated ASCII string to parse
 * @return true if the address was parsed
 */
bool lighting_command_from_ascii(
    BACNET_LIGHTING_COMMAND *value, const char *argv)
{
    bool status = false;
    BACNET_LIGHTING_OPERATION operation = BACNET_LIGHTS_NONE;
    unsigned a = 0;
    float b = 0.0, c = 0.0, d = 0.0, min = 0.0, max = 0.0;
    int count;

    if (!value) {
        return false;
    }
    if (!argv) {
        return false;
    }
    count = sscanf(argv, "%u,%f,%f,%f", &a, &b, &c, &d);
    if (count >= 1) {
        operation = a;
    } else {
        return false;
    }
    switch (operation) {
        case BACNET_LIGHTS_NONE:
            value->operation = operation;
            value->use_target_level = false;
            value->use_ramp_rate = false;
            value->use_step_increment = false;
            value->use_fade_time = false;
            value->use_priority = false;
            status = true;
            break;
        case BACNET_LIGHTS_FADE_TO:
            value->operation = operation;
            if (count >= 2) {
                /* (0.0..100.0) OPTIONAL */
                if (isgreaterequal(b, 0.0) && islessequal(b, 100.0)) {
                    value->use_target_level = true;
                    value->target_level = b;
                } else {
                    value->use_target_level = false;
                }
            } else {
                value->use_target_level = false;
            }
            if (count >= 3) {
                /* (100..86400000) OPTIONAL */
                if (isgreaterequal(c, 100.0) && islessequal(c, 86400000.0)) {
                    value->use_fade_time = true;
                    value->fade_time = (uint32_t)c;
                } else {
                    value->use_fade_time = false;
                }
            } else {
                value->use_fade_time = false;
            }
            if (count >= 4) {
                min = (float)BACNET_MIN_PRIORITY;
                max = (float)BACNET_MAX_PRIORITY;
                if (isgreaterequal(d, min) && islessequal(d, max)) {
                    value->use_priority = true;
                    value->priority = (uint8_t)d;
                } else {
                    value->use_priority = false;
                }
            } else {
                value->use_priority = false;
            }
            value->use_ramp_rate = false;
            value->use_step_increment = false;
            status = true;
            break;
        case BACNET_LIGHTS_RAMP_TO:
            value->operation = operation;
            if (count >= 2) {
                /* (0.0..100.0) OPTIONAL */
                if (isgreaterequal(b, 0.0) && islessequal(b, 100.0)) {
                    value->use_target_level = true;
                    value->target_level = b;
                } else {
                    value->use_target_level = false;
                }
            } else {
                value->use_target_level = false;
            }
            if (count >= 3) {
                /* (0.1..100.0) OPTIONAL */
                if (isgreaterequal(c, 0.1) && islessequal(c, 100.0)) {
                    value->use_ramp_rate = true;
                    value->ramp_rate = c;
                } else {
                    value->use_ramp_rate = false;
                }
            } else {
                value->use_ramp_rate = false;
            }
            if (count >= 4) {
                min = (float)BACNET_MIN_PRIORITY;
                max = (float)BACNET_MAX_PRIORITY;
                if (isgreaterequal(d, min) && islessequal(d, max)) {
                    value->use_priority = true;
                    value->priority = (uint8_t)d;
                } else {
                    value->use_priority = false;
                }
            } else {
                value->use_priority = false;
            }
            value->use_fade_time = false;
            value->use_step_increment = false;
            status = true;
            break;
        case BACNET_LIGHTS_STEP_UP:
        case BACNET_LIGHTS_STEP_DOWN:
        case BACNET_LIGHTS_STEP_ON:
        case BACNET_LIGHTS_STEP_OFF:
            value->operation = operation;
            /* (0.1..100.0) OPTIONAL */
            if (count >= 2) {
                if (isgreaterequal(b, 0.1) && islessequal(b, 100.0)) {
                    value->use_step_increment = true;
                    value->ramp_rate = b;
                } else {
                    value->step_increment = false;
                }
            } else {
                value->step_increment = false;
            }
            if (count >= 3) {
                min = (float)BACNET_MIN_PRIORITY;
                max = (float)BACNET_MAX_PRIORITY;
                if (isgreaterequal(c, min) && islessequal(c, max)) {
                    value->use_priority = true;
                    value->priority = (uint8_t)c;
                } else {
                    value->use_priority = false;
                }
            } else {
                value->use_priority = false;
            }
            value->use_target_level = false;
            value->use_ramp_rate = false;
            value->use_fade_time = false;
            status = true;
            break;
        case BACNET_LIGHTS_WARN:
        case BACNET_LIGHTS_WARN_OFF:
        case BACNET_LIGHTS_WARN_RELINQUISH:
        case BACNET_LIGHTS_STOP:
            value->operation = operation;
            if (count >= 2) {
                min = (float)BACNET_MIN_PRIORITY;
                max = (float)BACNET_MAX_PRIORITY;
                if (isgreaterequal(b, min) && islessequal(b, max)) {
                    value->use_priority = true;
                    value->priority = (uint8_t)b;
                } else {
                    value->use_priority = false;
                }
            } else {
                value->use_priority = false;
            }
            value->use_target_level = false;
            value->use_ramp_rate = false;
            value->use_step_increment = false;
            value->use_fade_time = false;
            status = true;
            break;
        default:
            value->operation = operation;
            value->use_target_level = false;
            value->use_ramp_rate = false;
            value->use_step_increment = false;
            value->use_fade_time = false;
            value->use_priority = false;
            status = true;
            break;
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
int xy_color_encode(uint8_t *apdu, const BACNET_XY_COLOR *value)
{
    int len = 0;
    int apdu_len = 0;

    if (value) {
        /* x-coordinate REAL */
        len = encode_application_real(apdu, value->x_coordinate);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        /* y-coordinate REAL */
        len = encode_application_real(apdu, value->y_coordinate);
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
    uint8_t *apdu, uint8_t tag_number, const BACNET_XY_COLOR *value)
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
 * @return the number of apdu bytes consumed, or BACNET_STATUS_ERROR
 */
int xy_color_decode(
    const uint8_t *apdu, uint32_t apdu_size, BACNET_XY_COLOR *value)
{
    float real_value;
    int len = 0;
    int apdu_len = 0;

    if (apdu) {
        len = bacnet_real_application_decode(
            &apdu[apdu_len], apdu_size, &real_value);
        if (len > 0) {
            if (value) {
                value->x_coordinate = real_value;
            }
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
        }
        len = bacnet_real_application_decode(
            &apdu[apdu_len], apdu_size, &real_value);
        if (len > 0) {
            if (value) {
                value->y_coordinate = real_value;
            }
            apdu_len += len;
        } else {
            return BACNET_STATUS_ERROR;
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
 * @return the number of apdu bytes consumed, or BACNET_STATUS_ERROR
 */
int xy_color_context_decode(
    const uint8_t *apdu,
    uint32_t apdu_size,
    uint8_t tag_number,
    BACNET_XY_COLOR *value)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_XY_COLOR color = { 0.0, 0.0 };

    if (!bacnet_is_opening_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;
    len = xy_color_decode(&apdu[apdu_len], apdu_size - apdu_len, &color);
    if (len > 0) {
        apdu_len += len;
        if (value) {
            value->x_coordinate = color.x_coordinate;
            value->y_coordinate = color.y_coordinate;
        }
    } else {
        return BACNET_STATUS_ERROR;
    }
    if (!bacnet_is_closing_tag_number(
            &apdu[apdu_len], apdu_size - apdu_len, tag_number, &len)) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len += len;

    return apdu_len;
}

/**
 * @brief Set the BACnetxyColor complex data for x and y coordinates
 * @param dst - destination BACNET_XY_COLOR structure
 * @param x - x coordinate
 * @param x - y coordinate
 */
void xy_color_set(BACNET_XY_COLOR *dst, float x, float y)
{
    if (dst) {
        dst->x_coordinate = x;
        dst->y_coordinate = y;
    }
}

/**
 * @brief Copy the BACnetxyColor complex data from src to dst
 * @param dst - destination BACNET_XY_COLOR structure
 * @param src - source BACNET_XY_COLOR structure
 * @return true if successfully copied
 */
int xy_color_copy(BACNET_XY_COLOR *dst, const BACNET_XY_COLOR *src)
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
bool xy_color_same(const BACNET_XY_COLOR *value1, const BACNET_XY_COLOR *value2)
{
    bool status = false;

    if (value1 && value2) {
        if (!islessgreater(value1->x_coordinate, value2->x_coordinate) &&
            !islessgreater(value1->y_coordinate, value2->y_coordinate)) {
            status = true;
        }
    }

    return status;
}

/**
 * Convert BACnetXYcolor to ASCII for printing
 *
 * @param value - struct to convert to ASCII
 * @param buf - ASCII output buffer (or NULL for size)
 * @param buf_size - ASCII output buffer capacity (or 0 for size)
 *
 * @return the number of characters which would be generated for the given
 *  input, excluding the trailing null. negative is returned if the
 *  capacity was not sufficient.
 *
 * @note buf and buf_size may be null and zero to return only the size
 */
int xy_color_to_ascii(const BACNET_XY_COLOR *value, char *buf, size_t buf_size)
{
    return snprintf(
        buf, buf_size, "(%f,%f)", (double)value->x_coordinate,
        (double)value->x_coordinate);
}

/**
 * @brief Parse an ASCII string for a BACnetXYcolor
 * @param mac [out] BACNET_XY_COLOR structure to store the results
 * @param arg [in] nul terminated ASCII string to parse
 * @return true if the address was parsed
 */
bool xy_color_from_ascii(BACNET_XY_COLOR *value, const char *argv)
{
    bool status = false;
    int count;
    float x, y;

    count = sscanf(argv, "%f,%f", &x, &y);
    if (count == 2) {
        value->x_coordinate = x;
        value->y_coordinate = y;
        status = true;
    } else {
#if defined(BACAPP_COLOR_RGB_CONVERSION_ENABLED)
        uint8_t red, green, blue;
        unsigned rgb_max;

        rgb_max = color_rgb_count();
        count = color_rgb_from_ascii(&red, &green, &blue, argv);
        if (count < rgb_max) {
            color_rgb_to_xy(red, green, blue, &x, &y, NULL);
            value->x_coordinate = x;
            value->y_coordinate = y;
            status = true;
        }
#endif
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
int color_command_encode(uint8_t *apdu, const BACNET_COLOR_COMMAND *value)
{
    int len = 0;
    int apdu_len = 0;
    BACNET_UNSIGNED_INTEGER unsigned_value;

    if (value) {
        /* operation [0] BACnetColorOperation */
        len = encode_context_enumerated(apdu, 0, value->operation);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        switch (value->operation) {
            case BACNET_COLOR_OPERATION_NONE:
                break;
            case BACNET_COLOR_OPERATION_FADE_TO_COLOR:
                /* target-color [1] BACnetxyColor */
                len = xy_color_context_encode(apdu, 1, &value->target.color);
                apdu_len += len;
                if (apdu) {
                    apdu += len;
                }
                if ((value->transit.fade_time >= BACNET_COLOR_FADE_TIME_MIN) &&
                    (value->transit.fade_time <= BACNET_COLOR_FADE_TIME_MAX)) {
                    /* fade-time [3] Unsigned (100.. 86400000) OPTIONAL */
                    unsigned_value = value->transit.fade_time;
                    len = encode_context_unsigned(apdu, 3, unsigned_value);
                    apdu_len += len;
                }
                break;
            case BACNET_COLOR_OPERATION_FADE_TO_CCT:
                /* target-color-temperature [2] Unsigned */
                unsigned_value = value->target.color_temperature;
                len = encode_context_unsigned(apdu, 2, unsigned_value);
                apdu_len += len;
                if (apdu) {
                    apdu += len;
                }
                if ((value->transit.fade_time >= BACNET_COLOR_FADE_TIME_MIN) &&
                    (value->transit.fade_time <= BACNET_COLOR_FADE_TIME_MAX)) {
                    /* fade-time [3] Unsigned (100.. 86400000) OPTIONAL */
                    unsigned_value = value->transit.fade_time;
                    len = encode_context_unsigned(apdu, 3, unsigned_value);
                    apdu_len += len;
                }
                break;
            case BACNET_COLOR_OPERATION_RAMP_TO_CCT:
                /* target-color-temperature [2] Unsigned */
                unsigned_value = value->target.color_temperature;
                len = encode_context_unsigned(apdu, 2, unsigned_value);
                apdu_len += len;
                if (apdu) {
                    apdu += len;
                }
                if ((value->transit.ramp_rate >= BACNET_COLOR_RAMP_RATE_MIN) &&
                    (value->transit.ramp_rate <= BACNET_COLOR_RAMP_RATE_MAX)) {
                    /* ramp-rate      [4] Unsigned (1..30000) OPTIONAL */
                    unsigned_value = value->transit.ramp_rate;
                    len = encode_context_unsigned(apdu, 4, unsigned_value);
                    apdu_len += len;
                }
                break;
            case BACNET_COLOR_OPERATION_STEP_UP_CCT:
            case BACNET_COLOR_OPERATION_STEP_DOWN_CCT:
                if ((value->transit.step_increment >=
                     BACNET_COLOR_STEP_INCREMENT_MIN) &&
                    (value->transit.step_increment <=
                     BACNET_COLOR_STEP_INCREMENT_MAX)) {
                    /* step-increment [5] Unsigned (1..30000) OPTIONAL */
                    unsigned_value = value->transit.step_increment;
                    len = encode_context_unsigned(apdu, 5, unsigned_value);
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
    uint8_t *apdu, uint8_t tag_number, const BACNET_COLOR_COMMAND *value)
{
    int len = 0;
    int apdu_len = 0;

    if (value) {
        len = encode_opening_tag(apdu, tag_number);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = color_command_encode(apdu, value);
        apdu_len += len;
        if (apdu) {
            apdu += len;
        }
        len = encode_closing_tag(apdu, tag_number);
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
int color_command_decode(
    const uint8_t *apdu,
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
        &apdu[apdu_len], apdu_size - apdu_len, 0, &unsigned_value);
    if (len > 0) {
        apdu_len += len;
    } else {
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
            len = xy_color_context_decode(
                &apdu[apdu_len], apdu_size - apdu_len, 1, &color);
            if (len > 0) {
                apdu_len += len;
            } else {
                if (error_code) {
                    *error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
                }
                return BACNET_STATUS_REJECT;
            }
            if (value) {
                value->target.color.x_coordinate = color.x_coordinate;
                value->target.color.y_coordinate = color.y_coordinate;
            }
            if ((apdu_size - apdu_len) > 0) {
                /* fade-time [3] Unsigned (100.. 86400000) OPTIONAL */
                len = bacnet_unsigned_context_decode(
                    &apdu[apdu_len], apdu_size - apdu_len, 3, &unsigned_value);
                if (len > 0) {
                    apdu_len += len;
                } else {
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
                if ((unsigned_value < BACNET_COLOR_FADE_TIME_MIN) ||
                    (unsigned_value > BACNET_COLOR_FADE_TIME_MAX)) {
                    if (error_code) {
                        *error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
                    }
                    return BACNET_STATUS_REJECT;
                }
            } else {
                unsigned_value = 0;
            }
            if (value) {
                value->transit.fade_time = unsigned_value;
            }
            break;
        case BACNET_COLOR_OPERATION_FADE_TO_CCT:
            /* target-color-temperature [2] Unsigned */
            len = bacnet_unsigned_context_decode(
                &apdu[apdu_len], apdu_size - apdu_len, 2, &unsigned_value);
            if (len > 0) {
                apdu_len += len;
            } else {
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
            if (unsigned_value > UINT16_MAX) {
                if (error_code) {
                    *error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
                }
                return BACNET_STATUS_REJECT;
            }
            if (value) {
                value->target.color_temperature = unsigned_value;
            }
            if ((apdu_size - apdu_len) > 0) {
                /* fade-time [3] Unsigned (100.. 86400000) OPTIONAL */
                len = bacnet_unsigned_context_decode(
                    &apdu[apdu_len], apdu_size - apdu_len, 3, &unsigned_value);
                if (len > 0) {
                    apdu_len += len;
                } else {
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
                if ((unsigned_value < BACNET_COLOR_FADE_TIME_MIN) ||
                    (unsigned_value > BACNET_COLOR_FADE_TIME_MAX)) {
                    if (error_code) {
                        *error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
                    }
                    return BACNET_STATUS_REJECT;
                }
            } else {
                unsigned_value = 0;
            }
            if (value) {
                value->transit.fade_time = unsigned_value;
            }
            break;
        case BACNET_COLOR_OPERATION_RAMP_TO_CCT:
            /* target-color-temperature [2] Unsigned */
            len = bacnet_unsigned_context_decode(
                &apdu[apdu_len], apdu_size - apdu_len, 2, &unsigned_value);
            if (len > 0) {
                apdu_len += len;
            } else {
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
            if (unsigned_value > UINT16_MAX) {
                if (error_code) {
                    *error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
                }
                return BACNET_STATUS_REJECT;
            }
            if (value) {
                value->target.color_temperature = unsigned_value;
            }
            if ((apdu_size - apdu_len) > 0) {
                /* ramp-rate      [4] Unsigned (1..30000) OPTIONAL */
                len = bacnet_unsigned_context_decode(
                    &apdu[apdu_len], apdu_size - apdu_len, 4, &unsigned_value);
                if (len > 0) {
                    apdu_len += len;
                } else {
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
                if ((unsigned_value < BACNET_COLOR_RAMP_RATE_MIN) ||
                    (unsigned_value > BACNET_COLOR_RAMP_RATE_MAX)) {
                    if (error_code) {
                        *error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
                    }
                    return BACNET_STATUS_REJECT;
                }
            } else {
                unsigned_value = 0;
            }
            if (value) {
                value->transit.ramp_rate = unsigned_value;
            }
            break;
        case BACNET_COLOR_OPERATION_STEP_UP_CCT:
        case BACNET_COLOR_OPERATION_STEP_DOWN_CCT:
            if ((apdu_size - apdu_len) > 0) {
                /* step-increment [5] Unsigned (1..30000) OPTIONAL */
                len = bacnet_unsigned_context_decode(
                    &apdu[apdu_len], apdu_size - apdu_len, 5, &unsigned_value);
                if (len > 0) {
                    apdu_len += len;
                } else {
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
                if ((unsigned_value < BACNET_COLOR_STEP_INCREMENT_MIN) ||
                    (unsigned_value > BACNET_COLOR_STEP_INCREMENT_MAX)) {
                    if (error_code) {
                        *error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
                    }
                    return BACNET_STATUS_REJECT;
                }
            } else {
                unsigned_value = 0;
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
bool color_command_copy(
    BACNET_COLOR_COMMAND *dst, const BACNET_COLOR_COMMAND *src)
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
    const BACNET_COLOR_COMMAND *value1, const BACNET_COLOR_COMMAND *value2)
{
    bool status = false;

    if (value1 && value2 && (value1->operation == value2->operation)) {
        switch (value1->operation) {
            case BACNET_COLOR_OPERATION_NONE:
                status = true;
                break;
            case BACNET_COLOR_OPERATION_FADE_TO_COLOR:
                if (!islessgreater(
                        value1->target.color.x_coordinate,
                        value2->target.color.x_coordinate) &&
                    !islessgreater(
                        value1->target.color.y_coordinate,
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
