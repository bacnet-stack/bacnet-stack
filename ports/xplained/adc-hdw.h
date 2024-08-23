/**
 * @file
 * @brief ADC configuration
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2014
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef ADC_HDW_H
#define ADC_HDW_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  void adc_init (void);
  uint16_t adc_result_12bit(uint8_t channel);
  uint16_t adc_result_10bit(uint8_t channel);
  uint8_t adc_result_8bit(uint8_t channel);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
