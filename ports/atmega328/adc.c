/**
 * @brief This module manages the Analog to Digital Converter (ADC)
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include "hardware.h"
/* me */
#include "adc.h"

/* prescale select bits */
#if (F_CPU >> 1) < 1000000
#define ADPS_8BIT (1)
#define ADPS_10BIT (3)
#elif (F_CPU >> 2) < 1000000
#define ADPS_8BIT (2)
#define ADPS_10BIT (4)
#elif (F_CPU >> 3) < 1000000
#define ADPS_8BIT (3)
#define ADPS_10BIT (5)
#elif (F_CPU >> 4) < 1000000
#define ADPS_8BIT (4)
#define ADPS_10BIT (6)
#elif (F_CPU >> 5) < 1000000
#define ADPS_8BIT (5)
#define ADPS_10BIT (7)
#else
#error "ADC: F_CPU too large for accuracy."
#endif

/* full scale ADC voltage constant in millivolts */
#ifndef ADC_MILLIVOLTS_MAX
#define ADC_MILLIVOLTS_MAX 5000L
#endif
/* full scale ADC value  */
#ifndef ADC_VALUE_MAX
#define ADC_VALUE_MAX 1024L
#endif

/* Array of ADC results */
#define ADC_CHANNELS_MAX 8
static volatile uint16_t Sample_Result[ADC_CHANNELS_MAX];
static volatile uint8_t Enabled_Channels;

/**
 * @brief ADC interrupt based acquisition ISR
 */
ISR(ADC_vect)
{
    uint8_t index;
    uint8_t mask;
    uint8_t channels;
    uint16_t value = 0;

    /* determine which conversion finished */
    index = BITMASK_CHECK(ADMUX, ((1 << MUX2) | (1 << MUX1) | (1 << MUX0)));
    /* read the results */
    value = ADCL;
    value |= (ADCH << 8);
    Sample_Result[index] = value;
    channels = Enabled_Channels;
    __enable_interrupt();
    /* clear the mux */
    BITMASK_CLEAR(ADMUX, ((1 << MUX2) | (1 << MUX1) | (1 << MUX0)));
    /* find the next enabled channel */
    while (channels) {
        index++;
        if (index >= ADC_CHANNELS_MAX) {
            index = 0;
        }
        mask = 1 << index;
        if (channels & mask) {
            break;
        }
    }
    /* configure the next channel */
    BITMASK_SET(ADMUX, ((index) << MUX0));
    /* Start the next conversion */
    BIT_SET(ADCSRA, ADSC);
}

/**
 * @brief Enable the ADC channel for interrupt based acquisition
 * @param index - 0..8 = ADC0..ADC8, respectively
 */
void adc_enable(uint8_t index)
{
    if (Enabled_Channels) {
        /* ADC interupt is already started */
        BIT_SET(Enabled_Channels, index);
    } else {
        if (index < ADC_CHANNELS_MAX) {
            /* not running yet */
            BIT_SET(Enabled_Channels, index);
            /* clear the mux */
            BITMASK_CLEAR(ADMUX, ((1 << MUX2) | (1 << MUX1) | (1 << MUX0)));
            /* configure the channel */
            BITMASK_SET(ADMUX, ((index) << MUX0));
            /* Start the next conversion */
            BIT_SET(ADCSRA, ADSC);
        }
    }
}

/**
 * @brief Get the latest ADC channel value (8-bit)
 * @param index - 0..8 = ADC0..ADC8, respectively
 */
uint8_t adc_result_8bit(uint8_t index)
{
    uint8_t result = 0;
    uint8_t sreg;

    if (index < ADC_CHANNELS_MAX) {
        adc_enable(index);
        sreg = SREG;
        __disable_interrupt();
        result = (uint8_t)(Sample_Result[index] >> 2);
        SREG = sreg;
    }

    return result;
}

/**
 * @brief Get the latest ADC channel value (10-bit)
 * @param index - 0..8 = ADC0..ADC8, respectively
 */
uint16_t adc_result_10bit(uint8_t index)
{ /* 0..7 = ADC0..ADC7, respectively */
    uint16_t result = 0;
    uint8_t sreg;

    if (index < ADC_CHANNELS_MAX) {
        adc_enable(index);
        sreg = SREG;
        __disable_interrupt();
        result = Sample_Result[index];
        SREG = sreg;
    }

    return result;
}

/**
 * @brief Get the latest ADC channel value in millivolts
 * @param index - 0..8 = ADC0..ADC8, respectively
 * @return millivolts
 */
uint16_t adc_millivolts(uint8_t index)
{
    uint32_t value;

    value = adc_result_10bit(index);
    value = (value * ADC_MILLIVOLTS_MAX) / ADC_VALUE_MAX;

    return (uint16_t)(value);
}

/**
 * @brief Initialize the ADC for interrupt based acquisition
 */
void adc_init(void)
{
    /* Initial channel selection */
    /* ADLAR = Left Adjust Result
       REFSx = hardware setup: cap on AREF
     */
    ADMUX = (0 << ADLAR) | (0 << REFS1) | (1 << REFS0);
    /*  ADEN = Enable
       ADSC = Start conversion
       ADIF = Interrupt Flag - write 1 to clear!
       ADIE = Interrupt Enable
       ADATE = Auto Trigger Enable
     */
    ADCSRA =
        (1 << ADEN) | (1 << ADIE) | (1 << ADIF) | (0 << ADATE) | ADPS_10BIT;
    /* trigger selection bits
       0 0 0 Free Running mode
       0 0 1 Analog Comparator
       0 1 0 External Interrupt Request 0
       0 1 1 Timer/Counter0 Compare Match
       1 0 0 Timer/Counter0 Overflow
       1 0 1 Timer/Counter1 Compare Match B
       1 1 0 Timer/Counter1 Overflow
       1 1 1 Timer/Counter1 Capture Event
     */
    ADCSRB = (0 << ADTS2) | (0 << ADTS1) | (0 << ADTS0);
    /* disable ADC power reduction */
    PRR &= ((uint8_t) ~(1 << PRADC));
}
