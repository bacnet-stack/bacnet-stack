/**************************************************************************
*
* Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*********************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "hardware.h"
/* me */
#include "adc.h"

/* prescale select bits */
#if   (F_CPU >> 1) < 1000000
#define ADPS_8BIT    (1)
#define ADPS_10BIT   (3)
#elif (F_CPU >> 2) < 1000000
#define ADPS_8BIT    (2)
#define ADPS_10BIT   (4)
#elif (F_CPU >> 3) < 1000000
#define ADPS_8BIT    (3)
#define ADPS_10BIT   (5)
#elif (F_CPU >> 4) < 1000000
#define ADPS_8BIT    (4)
#define ADPS_10BIT   (6)
#elif (F_CPU >> 5) < 1000000
#define ADPS_8BIT    (5)
#define ADPS_10BIT   (7)
#else
#error  "ADC: F_CPU too large for accuracy."
#endif

/* we could have array of ADC results */
static volatile uint8_t Sample_Result;

ISR(ADC_vect)
{
    /* since we configured as ADLAR=1, get value from ADCH */
    Sample_Result = ADCH;
}

uint8_t adc_result(
    uint8_t channel)
{       /* 0..7 = ADC0..ADC7, respectively */
    return Sample_Result;
}

void adc_init(
    void)
{
    /*  set prescaler */
    ADCSRA |= ADPS_8BIT;
    /* Initial channel selection */
    /* ADLAR = Left Adjust Result
       REFSx = hardware setup: cap on AREF
     */
    ADMUX = 7 /* channel */  | (1 << ADLAR) | (0 << REFS1) | (1 << REFS0);
    /*  ADEN = Enable
       ADSC = Start conversion
       ADIF = Interrupt Flag
       ADIE = Interrupt Enable
       ADATE = Auto Trigger Enable
     */
    ADCSRA |= (1 << ADEN) | (1 << ADIE) | (1 << ADIF) | (1 << ADATE);
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
    ADCSRB |= (0 << ADTS2) | (0 << ADTS1) | (0 << ADTS0);
    /* start the conversions */
    ADCSRA |= (1 << ADSC);
    /* Clear the Power Reduction bit */
    BIT_CLEAR(PRR, PRADC);
}
