/**
 * @file
 * @brief BACnetShedLevel complex data type encode and decode
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date December 2025
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdint.h>
#include <stdio.h>
#include "bacnet/bacdcode.h"
#include "bacnet/shed_level.h"

/**
 * @brief Encode a BACnetShedLevel value.
 *
 *  BACnetShedLevel ::= CHOICE {
 *      percent[0]  Unsigned,
 *      level[1]    Unsigned,
 *      amount[2]   Real
 *  }
 *
 * @param apdu - buffer to encode to
 * @param value - value to encode
 * @return number of bytes encoded
 */
int bacnet_shed_level_encode(uint8_t *apdu, const BACNET_SHED_LEVEL *value)
{
    int apdu_len = 0;

    if (!value) {
        return 0;
    }
    switch (value->type) {
        case BACNET_SHED_TYPE_PERCENT:
            apdu_len = encode_context_unsigned(apdu, 0, value->value.percent);
            break;
        case BACNET_SHED_TYPE_LEVEL:
            apdu_len = encode_context_unsigned(apdu, 1, value->value.level);
            break;
        case BACNET_SHED_TYPE_AMOUNT:
            apdu_len = encode_context_real(apdu, 2, value->value.amount);
            break;
        default:
            break;
    }

    return apdu_len;
}

/**
 * @brief Decode a BACnetShedLevel value.
 *
 *  BACnetShedLevel ::= CHOICE {
 *      percent[0]  Unsigned,
 *      level[1]    Unsigned,
 *      amount[2]   Real
 *  }
 *
 * @param apdu - buffer to decode to
 * @param apdu_size - size of the buffer
 * @param value - value to encode, or NULL for size only
 * @return number of bytes decoded, or BACNET_STATUS_ERROR on error
 */
int bacnet_shed_level_decode(
    const uint8_t *apdu, size_t apdu_size, BACNET_SHED_LEVEL *value)
{
    int apdu_len = 0;
    BACNET_TAG tag = { 0 };
    BACNET_UNSIGNED_INTEGER unsigned_value = 0;
    float real_value = 0.0f;

    if (!apdu) {
        return BACNET_STATUS_ERROR;
    }
    apdu_len = bacnet_tag_decode(apdu, apdu_size, &tag);
    if (apdu_len <= 0) {
        return BACNET_STATUS_ERROR;
    }
    switch (tag.number) {
        case 0:
            /* percent - Unsigned */
            apdu_len = bacnet_unsigned_context_decode(
                apdu, apdu_size, tag.number, &unsigned_value);
            if (apdu_len > 0) {
                if (value) {
                    value->value.percent = unsigned_value;
                    value->type = BACNET_SHED_TYPE_PERCENT;
                }
            }
            break;
        case 1:
            /* level - Unsigned */
            apdu_len = bacnet_unsigned_context_decode(
                apdu, apdu_size, tag.number, &unsigned_value);
            if (apdu_len > 0) {
                if (value) {
                    value->value.level = unsigned_value;
                    value->type = BACNET_SHED_TYPE_LEVEL;
                }
            }
            break;

        case 2:
            apdu_len = bacnet_real_context_decode(
                apdu, apdu_size, tag.number, &real_value);
            if (apdu_len > 0) {
                if (value) {
                    value->type = BACNET_SHED_TYPE_AMOUNT;
                    value->value.amount = real_value;
                }
            }
            break;
        default:
            return BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * @brief Compare the BACnetShedLevel complex data
 * @param value1 - BACNET_SHED_LEVEL structure
 * @param value2 - BACNET_SHED_LEVEL structure
 * @return true if the same
 */
bool bacnet_shed_level_same(
    const BACNET_SHED_LEVEL *value1, const BACNET_SHED_LEVEL *value2)
{
    bool status = false;

    if (value1 && value2) {
        status = true;
        if (value1->type != value2->type) {
            status = false;
        } else {
            switch (value1->type) {
                case BACNET_SHED_TYPE_PERCENT:
                    if (value1->value.percent != value2->value.percent) {
                        status = false;
                    }
                    break;
                case BACNET_SHED_TYPE_LEVEL:
                    if (value1->value.level != value2->value.level) {
                        status = false;
                    }
                    break;
                case BACNET_SHED_TYPE_AMOUNT:
                    if (islessgreater(
                            value1->value.amount, value2->value.amount)) {
                        status = false;
                    }
                    break;
                default:
                    break;
            }
        }
    }

    return status;
}

/**
 * @brief Compare the BACnetShedLevel complex data
 * @param value1 - BACNET_SHED_LEVEL structure
 * @param value2 - BACNET_SHED_LEVEL structure
 * @return true if the same
 */
bool bacnet_shed_level_copy(
    BACNET_SHED_LEVEL *dest, const BACNET_SHED_LEVEL *src)
{
    if (!dest || !src) {
        return false;
    }

    memcpy(dest, src, sizeof(BACNET_SHED_LEVEL));

    return true;
}

/**
 * @brief Print a value to a string for EPICS
 * @param str - destination string, or NULL for length only
 * @param str_len - length of the destination string, or 0 for length only
 * @param value - value to be printed
 * @return number of characters written to the string
 */
int bacapp_snprintf_shed_level(
    char *str, size_t str_len, const BACNET_SHED_LEVEL *value)
{
    int length = 0;

    switch (value->type) {
        case BACNET_SHED_TYPE_PERCENT:
            length = bacnet_snprintf(
                str, str_len, length, "%u%%", (unsigned)value->value.percent);
            break;
        case BACNET_SHED_TYPE_LEVEL:
            length = bacnet_snprintf(
                str, str_len, length, "%u", (unsigned)value->value.level);
            break;
        case BACNET_SHED_TYPE_AMOUNT:
            length = bacnet_snprintf(
                str, str_len, length, "%f", (double)value->value.amount);
            break;
        default:
            break;
    }

    return length;
}

/**
 * @brief Parse a string into a BACnet Shed Level value
 * @param value [out] The BACnet Shed Level value
 * @param argv [in] The string to parse
 * @return True on success, else False
 */
bool bacnet_shed_level_from_ascii(BACNET_SHED_LEVEL *value, const char *argv)
{
    bool status = false;
    int count;
    unsigned percent, level;
    float amount;
    const char *percentage;
    const char *decimal_point;

    if (!status) {
        percentage = strchr(argv, '%');
        if (percentage) {
            count = sscanf(argv, "%u", &percent);
            if (count == 1) {
                value->type = BACNET_SHED_TYPE_PERCENT;
                value->value.percent = percent;
                status = true;
            }
        }
    }
    if (!status) {
        decimal_point = strchr(argv, '.');
        if (decimal_point) {
            count = sscanf(argv, "%f", &amount);
            if (count == 1) {
                value->type = BACNET_SHED_TYPE_AMOUNT;
                value->value.amount = amount;
                status = true;
            }
        }
    }
    if (!status) {
        count = sscanf(argv, "%u", &level);
        if (count == 1) {
            value->type = BACNET_SHED_TYPE_LEVEL;
            value->value.level = level;
            status = true;
        }
    }

    return status;
}
