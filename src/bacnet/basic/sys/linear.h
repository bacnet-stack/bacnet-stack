/**
 * @file
 * @brief API for linear interpolation library
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2011
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_SYS_LINEAR_H
#define BACNET_SYS_LINEAR_H

/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

float linear_interpolate(float x1, float x2, float x3, float y1, float y3);

float linear_interpolate_round(
    float x1, float x2, float x3, float y1, float y3);

long linear_interpolate_int(long x1, long x2, long x3, long y1, long y3);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
