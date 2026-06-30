/**
 * @file
 * @brief This file contains some value comparison and bounds helpers
 * @author Anthony Loiseau <anthony.loiseau@allcircuits.com>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_SYS_COMPARE_H
#define BACNET_SYS_COMPARE_H

/**
 * @brief Return the maximum of two values
 * @param a First value
 * @param b Second value
 * @return a or b, whichever is the bigger one
 * @attention Arguments can be evaluated more than once
 */
#define BACNET_MAX(a, b) (((a) > (b)) ? (a) : (b))

/**
 * @brief Return the minimum of two values
 * @param a First value
 * @param b Second value
 * @return a or b, whichever is the smaller one
 * @attention Arguments can be evaluated more than once
 */
#define BACNET_MIN(a, b) (((a) < (b)) ? (a) : (b))

/**
 * @brief Clamp a value to a specified inclusive range
 * @param min Minimum value for the result
 * @param value Value to clamp within min and max inclusive
 * @param max Maximum value for the result
 * @return min, value or max depending on value
 * @note Given value is not modified by this call
 * @attention Arguments can be evaluated more than once
 */
#define BACNET_CLAMP(min, value, max) \
    BACNET_MIN(BACNET_MAX((value), (min)), (max))

#endif /* BACNET_SYS_COMPARE_H */
