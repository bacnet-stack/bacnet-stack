/**
 * @brief This module manages the Analog to Digital Converter (ADC)
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef ADC_H
#define ADC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void adc_enable(uint8_t index);
uint8_t adc_result_8bit(uint8_t index);
uint16_t adc_result_10bit(uint8_t index);
uint16_t adc_millivolts(uint8_t index);
void adc_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
