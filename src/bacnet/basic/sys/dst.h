/**
 * @file
 * @brief This file contains the function prototypes for for the module.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 1997
 * @note Public domain algorithms from ACM
 * @copyright SPDX-License-Identifier: CC-PDDC
 */
#ifndef BACNET_BASIC_SYS_DST_H
#define BACNET_BASIC_SYS_DST_H
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

struct daylight_savings_data {
    bool Ordinal : 1;
    uint8_t Begin_Month;
    uint8_t Begin_Day;
    uint8_t Begin_Which_Day;
    uint8_t End_Month;
    uint8_t End_Day;
    uint8_t End_Which_Day;
    uint16_t Epoch_Year;
    uint8_t Epoch_Day;
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

bool dst_active(
    struct daylight_savings_data *dst,
    uint16_t year,
    uint8_t month,
    uint8_t day,
    uint8_t hour,
    uint8_t minute,
    uint8_t second);
void dst_init(
    struct daylight_savings_data *data,
    bool automatic,
    uint8_t begin_month,
    uint8_t begin_day,
    uint8_t begin_which_day,
    uint8_t end_month,
    uint8_t end_day,
    uint8_t end_which_day,
    uint8_t epoch_day,
    uint16_t epoch_year);
/* initialization */
void dst_init_defaults(struct daylight_savings_data *data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
