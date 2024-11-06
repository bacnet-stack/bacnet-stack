/**************************************************************************
*
* Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
*
* SPDX-License-Identifier: MIT
*
*********************************************************************/
#ifndef ADC_H
#define ADC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void adc_enable(
        uint8_t index); /* 0..7 = ADC0..ADC7, respectively */
    uint8_t adc_result_8bit(
        uint8_t index); /* 0..7 = ADC0..ADC7, respectively */
    uint16_t adc_result_10bit(
        uint8_t index); /* 0..7 = ADC0..ADC7, respectively */
    void adc_init(
        void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
