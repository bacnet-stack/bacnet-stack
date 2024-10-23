/**
 * @file
 * @brief Performs linear interpolation using single precision floating
 * point math or integer math, or a mixture of both. Linear interpolation
 * is a method of constructing new data points within the range of a discrete
 * set of known data points.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2011
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/basic/sys/linear.h"

/**
 * Linearly Interpolate the values between y1 and y3 based on x.
 *
 * @param  x1 - first x value, where x1 <= x2 <= x3 or x1 >= x2 >= x3
 * @param  x2 - intermediate x value, where x1 <= x2 <= x3 or x1 >= x2 >= x3
 * @param  x3 - last x value, where x1 <= x2 <= x3 or x1 >= x2 >= x3
 * @param  y1 - first y value, where y1 <= y2 <= y3 or y1 >= y2 >= y3
 * @param  y3 - last y value, where y1 <= y2 <= y3 or y1 >= y2 >= y3
 * @return y2 - an intermediate y value y1 <= y2 <= y3 or y1 >= y2 >= y3
 *         and the y value is linearly proportional to x1, x2, and x2.
 */
float linear_interpolate(float x1, float x2, float x3, float y1, float y3)
{
    float y2;

    if (y3 > y1) {
        y2 = y1 + (((x2 - x1) * (y3 - y1)) / (x3 - x1));
    } else {
        y2 = y1 - (((x2 - x1) * (y1 - y3)) / (x3 - x1));
    }

    return y2;
}

/**
 * Linearly Interpolate the values between y1 and y3 based on x
 * and round up the result.  Useful for integer interpolation.
 *
 * @param  x1 - first x value, where x1 <= x2 <= x3 or x1 >= x2 >= x3
 * @param  x2 - intermediate x value, where x1 <= x2 <= x3 or x1 >= x2 >= x3
 * @param  x3 - last x value, where x1 <= x2 <= x3 or x1 >= x2 >= x3
 * @param  y1 - first y value, where y1 <= y2 <= y3 or y1 >= y2 >= y3
 * @param  y3 - last y value, where y1 <= y2 <= y3 or y1 >= y2 >= y3
 * @return y2 - an intermediate y value y1 <= y2 <= y3 or y1 >= y2 >= y3
 *         and the y value is linearly proportional to x1, x2, and x2.
 */
float linear_interpolate_round(float x1, float x2, float x3, float y1, float y3)
{
    float y2;

    y2 = linear_interpolate(x1, x2, x3, y1, y3);
    /* round away from zero */
    if (y2 > 0.0f) {
        y2 += 0.5f;
    } else if (y2 < 0.0f) {
        y2 -= 0.5f;
    }

    return y2;
}

/**
 * Linearly Interpolate the values between y1 and y3 based on x
 * using integer math.
 *
 * @param  x1 - first x value, where x1 <= x2 <= x3 or x1 >= x2 >= x3
 * @param  x2 - intermediate x value, where x1 <= x2 <= x3 or x1 >= x2 >= x3
 * @param  x3 - last x value, where x1 <= x2 <= x3 or x1 >= x2 >= x3
 * @param  y1 - first y value, where y1 <= y2 <= y3 or y1 >= y2 >= y3
 * @param  y3 - last y value, where y1 <= y2 <= y3 or y1 >= y2 >= y3
 * @return y2 - an intermediate y value y1 <= y2 <= y3 or y1 >= y2 >= y3
 *         and the y value is linearly proportional to x1, x2, and x2.
 */
long linear_interpolate_int(long x1, long x2, long x3, long y1, long y3)
{
    long y2;

    if (y3 > y1) {
        y2 = y1 + (((x2 - x1) * (y3 - y1)) / (x3 - x1));
    } else {
        y2 = y1 - (((x2 - x1) * (y1 - y3)) / (x3 - x1));
    }

    return y2;
}
