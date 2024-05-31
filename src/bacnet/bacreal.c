/**
 * @file
 * @brief BACnet single precision REAL encode and decode functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacstr.h"
#include "bacnet/bacint.h"
#include "bacnet/bacreal.h"
#include "bacnet/basic/sys/bigend.h"

#ifndef BACNET_USE_DOUBLE
#define BACNET_USE_DOUBLE 1
#endif

/* from clause 20.2.6 Encoding of a Real Number Value */
/* returns the number of apdu bytes consumed */
int decode_real(uint8_t *apdu, float *real_value)
{
    union {
        uint8_t byte[4];
        float real_value;
    } my_data;

    if (apdu) {
        /* NOTE: assumes the compiler stores float as IEEE-754 float */
        if (big_endian()) {
            my_data.byte[0] = apdu[0];
            my_data.byte[1] = apdu[1];
            my_data.byte[2] = apdu[2];
            my_data.byte[3] = apdu[3];
        } else {
            my_data.byte[0] = apdu[3];
            my_data.byte[1] = apdu[2];
            my_data.byte[2] = apdu[1];
            my_data.byte[3] = apdu[0];
        }
        if (real_value) {
            *real_value = my_data.real_value;
        }
    }

    return 4;
}

int decode_real_safe(uint8_t *apdu, uint32_t len_value, float *real_value)
{
    if (len_value != 4) {
        if (real_value) {
            *real_value = 0.0f;
        }
        return (int)len_value;
    } else {
        return decode_real(apdu, real_value);
    }
}

/* from clause 20.2.6 Encoding of a Real Number Value */
/* returns the number of apdu bytes consumed */
int encode_bacnet_real(float value, uint8_t *apdu)
{
    union {
        uint8_t byte[4];
        float real_value;
    } my_data;

    /* NOTE: assumes the compiler stores float as IEEE-754 float */
    my_data.real_value = value;
    if (apdu) {
        if (big_endian()) {
            apdu[0] = my_data.byte[0];
            apdu[1] = my_data.byte[1];
            apdu[2] = my_data.byte[2];
            apdu[3] = my_data.byte[3];
        } else {
            apdu[0] = my_data.byte[3];
            apdu[1] = my_data.byte[2];
            apdu[2] = my_data.byte[1];
            apdu[3] = my_data.byte[0];
        }
    }

    return 4;
}

#if BACNET_USE_DOUBLE

/* from clause 20.2.7 Encoding of a Double Precision Real Number Value */
/* returns the number of apdu bytes consumed */
int decode_double(uint8_t *apdu, double *double_value)
{
    union {
        uint8_t byte[8];
        double double_value;
    } my_data;

    if (apdu) {
        /* NOTE: assumes the compiler stores float as IEEE-754 float */
        if (big_endian()) {
            my_data.byte[0] = apdu[0];
            my_data.byte[1] = apdu[1];
            my_data.byte[2] = apdu[2];
            my_data.byte[3] = apdu[3];
            my_data.byte[4] = apdu[4];
            my_data.byte[5] = apdu[5];
            my_data.byte[6] = apdu[6];
            my_data.byte[7] = apdu[7];
        } else {
            my_data.byte[0] = apdu[7];
            my_data.byte[1] = apdu[6];
            my_data.byte[2] = apdu[5];
            my_data.byte[3] = apdu[4];
            my_data.byte[4] = apdu[3];
            my_data.byte[5] = apdu[2];
            my_data.byte[6] = apdu[1];
            my_data.byte[7] = apdu[0];
        }
        if (double_value) {
            *double_value = my_data.double_value;
        }
    }

    return 8;
}

int decode_double_safe(uint8_t *apdu, uint32_t len_value, double *double_value)
{
    if (len_value != 8) {
        if (double_value) {
            *double_value = 0.0;
        }
        return (int)len_value;
    } else {
        return decode_double(apdu, double_value);
    }
}

/* from clause 20.2.7 Encoding of a Double Precision Real Number Value */
/* returns the number of apdu bytes consumed */
int encode_bacnet_double(double value, uint8_t *apdu)
{
    union {
        uint8_t byte[8];
        double double_value;
    } my_data;

    /* NOTE: assumes the compiler stores float as IEEE-754 float */
    my_data.double_value = value;
    if (apdu) {
        if (big_endian()) {
            apdu[0] = my_data.byte[0];
            apdu[1] = my_data.byte[1];
            apdu[2] = my_data.byte[2];
            apdu[3] = my_data.byte[3];
            apdu[4] = my_data.byte[4];
            apdu[5] = my_data.byte[5];
            apdu[6] = my_data.byte[6];
            apdu[7] = my_data.byte[7];
        } else {
            apdu[0] = my_data.byte[7];
            apdu[1] = my_data.byte[6];
            apdu[2] = my_data.byte[5];
            apdu[3] = my_data.byte[4];
            apdu[4] = my_data.byte[3];
            apdu[5] = my_data.byte[2];
            apdu[6] = my_data.byte[1];
            apdu[7] = my_data.byte[0];
        }
    }

    return 8;
}
#endif
