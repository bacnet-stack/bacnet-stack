/**
 * @file
 * @brief This file contains the function prototypes for the days calculations
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 1997
 * @copyright SPDX-License-Identifier: CC-PDDC
 */
#ifndef DAYS_H
#define DAYS_H
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
bool days_is_leap_year(uint16_t year);
BACNET_STACK_EXPORT
uint8_t days_per_month(uint16_t year, uint8_t month);
BACNET_STACK_EXPORT
uint32_t days_per_year(uint16_t year);
BACNET_STACK_EXPORT
uint16_t days_of_year(uint16_t year, uint8_t month, uint8_t day);
BACNET_STACK_EXPORT
uint16_t days_of_year_remaining(uint16_t year, uint8_t month, uint8_t day);
BACNET_STACK_EXPORT
void days_of_year_to_month_day(
    uint32_t days, uint16_t year, uint8_t *pMonth, uint8_t *pDay);
BACNET_STACK_EXPORT
uint32_t days_apart(uint16_t year1,
    uint8_t month1,
    uint8_t day1,
    uint16_t year2,
    uint8_t month2,
    uint8_t day2);
BACNET_STACK_EXPORT
uint32_t days_since_epoch(uint16_t epoch_year,
    uint16_t year,
    uint8_t month,
    uint8_t day);
BACNET_STACK_EXPORT
void days_since_epoch_to_date(
    uint16_t epoch_year,
    uint32_t days,
    uint16_t * pYear,
    uint8_t * pMonth,
    uint8_t * pDay);

BACNET_STACK_EXPORT
bool days_date_is_valid(uint16_t year,
    uint8_t month,
    uint8_t day);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
