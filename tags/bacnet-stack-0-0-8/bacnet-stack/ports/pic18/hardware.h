/**************************************************************************
*
* Copyright (C) 2003 Mark Norton and Steve Karg
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
*
* Functional
* Description:  Defines the hardware implementation for the Microchip
*               microprocessor used in the Synergy lighting control project.
*
*********************************************************************/
#ifndef HARDWARE_H
#define HARDWARE_H

#include <p18F452.h>
#include <portb.h>
#include <timers.h>

/****************************************************************************
 *                            Card IO                                       *
 ****************************************************************************/
/*
   TRIS masks are:
   0 = OUTPUT
   1 = INPUT

   The IO on this card is as follows:
   RA0 - SDA - SEEPROM (input)
   RA1 - SCL - SEEPROM (input)
   RA2 - not used (input)
   RA3 - not used (input)
   RA4 - LK2a - jumper (input)
   RA5 - LK2b - jumper (input)

   TRISA - 0011 1111 - 3Fh

   RB0 - INT - Zero Cross Interrupt (input)
   RB1 - LED - I2C Bus Indication (output)
   RB2 - LED - Labeled 'DATA' (output)
   RB3 - not used (input)
   RB4 - CTS input for RS-232 (not used unless LT1180A chip is there)
   RB5 - RTS output for RS-232 (not used unless LT1180A chip is there)
   RB6 - PGC - in circuit programming (input)
   RB7 - PGD - in circuit programming (input)

   TRISB - 1101 1001 - D9h

   RC0 - QH of 74165 shift register (input)
   RC1 - SHIFTREG_CKL of 74165 shift register (output)
   RC2 - SHIFTREG_LOAD of 74165 shift register (output)
   RC3 - SCL for I2C bus (input)
   RC4 - SDA for I2C bus (input)
   RC5 - RS-485 TXEN (or RS232) (output)
   RC6 - RS-485 TXD (or RS232) (output)
   RC7 - RS-485 RXD (or RS232) (input)

   TRISC - 1001 1001 - 99h

*/

#define  PORT_A_TRIS_MASK        0x3F
#define  PORT_B_TRIS_MASK        0xD9
#define  PORT_C_TRIS_MASK        0x99

/* hardware mapping of functionality */
#define DATA_LED_ON()  PORTBbits.RB2=0
#define DATA_LED_OFF() PORTBbits.RB2=1

#define ABUS_LED_ON()  PORTBbits.RB1=0
#define ABUS_LED_OFF() PORTBbits.RB1=1

#define RS485_TRANSMIT_DISABLE()  PORTCbits.RC5=0
#define RS485_TRANSMIT_ENABLE()   PORTCbits.RC5=1

/* note: board is inverted logic */
#define JUMPER_LK2_TOP_OFF() PORTAbits.RA4
#define JUMPER_LK2_TOP_ON() (!PORTAbits.RA4)
#define JUMPER_LK2_BOTTOM_OFF() PORTAbits.RA5
#define JUMPER_LK2_BOTTOM_ON() (!PORTAbits.RA5)

#define ZERO_CROSS               PORTBbits.RB0

#define I2C_CLK_LATCH            LATCbits.LATC3
#define I2C_DATA_LATCH           LATCbits.LATC4
#define I2C_CLK                  PORTCbits.RC3
#define I2C_DATA                 PORTCbits.RC4
#define I2C_CLK_HI_Z             TRISCbits.TRISC3
#define I2C_SDA_HI_Z             TRISCbits.TRISC4

#define EEPROM_DATA_LATCH        LATAbits.LATA0
#define EEPROM_CLK_LATCH         LATAbits.LATA1
#define EEPROM_SDA               PORTAbits.RA0
#define EEPROM_CLK               PORTAbits.RA1
#define EEPROM_SDA_HI_Z          TRISAbits.TRISA0
#define EEPROM_CLK_HI_Z          TRISAbits.TRISA1

#define SHIFTREG_LOAD            PORTCbits.RC2
#define SHIFTREG_CLK             PORTCbits.RC1
#define SHIFTREG_DATA            PORTCbits.RC0

#define NO_ANALOGS             0x06 // None
#define ALL_ANALOG             0x00 // RA0 RA1 RA2 RA3 RA5 RE0 RE1 RE2 Ref=Vdd
#define ANALOG_RA3_REF         0x01 // RA0 RA1 RA2 RA5 RE0 RE1 RE2 Ref=RA3
#define A_ANALOG               0x02 // RA0 RA1 RA2 RA3 RA5 Ref=Vdd
#define A_ANALOG_RA3_REF       0x03 // RA0 RA1 RA2 RA5 Ref=RA3
#define RA0_RA1_RA3_ANALOG     0x04 // RA0 RA1 RA3 Ref=Vdd
#define RA0_RA1_ANALOG_RA3_REF 0x05 // RA0 RA1 Ref=RA3

#define ANALOG_RA3_RA2_REF              0x08
#define ANALOG_NOT_RE1_RE2              0x09
#define ANALOG_NOT_RE1_RE2_REF_RA3      0x0A
#define ANALOG_NOT_RE1_RE2_REF_RA3_RA2  0x0B
#define A_ANALOG_RA3_RA2_REF            0x0C
#define RA0_RA1_ANALOG_RA3_RA2_REF      0x0D
#define RA0_ANALOG                      0x0E
#define RA0_ANALOG_RA3_RA2_REF          0x0F

// Constants used for SETUP_ADC() are:
#define ADC_OFF                0
#define ADC_START              4
#define ADC_CLOCK_DIV_2        1
#define ADC_CLOCK_DIV_4    0x101
#define ADC_CLOCK_DIV_8     0x41
#define ADC_CLOCK_DIV_16   0x141
#define ADC_CLOCK_DIV_32    0x81
#define ADC_CLOCK_DIV_64   0x181
#define ADC_CLOCK_INTERNAL  0xc1
#define ADC_DONE_MASK       0x04

#define SET_ADC_CHAN(x) ADCON0 = (ADC_CLOCK_DIV_32 | ((x) << 3))

#define T1_DISABLED         0
#define T1_INTERNAL         0x85
#define T1_EXTERNAL         0x87
#define T1_EXTERNAL_SYNC    0x83

#define T1_CLK_OUT          8

#define T1_DIV_BY_1         0
#define T1_DIV_BY_2         0x10
#define T1_DIV_BY_4         0x20
#define T1_DIV_BY_8         0x30
#define SETUP_TIMER1(mode) T1CON = (mode)

#define T2_DISABLED         0
#define T2_DIV_BY_1         4
#define T2_DIV_BY_4         5
#define T2_DIV_BY_16        6
#define SETUP_TIMER2(mode, period, postscale) \
{ \
  T2CON = ((mode) | ((postscale)-1)<<3); \
  PR2 = (period); \
}

#define T3_DISABLED         0
#define T3_INTERNAL         0x85
#define T3_EXTERNAL         0x87
#define T3_EXTERNAL_SYNC    0x83

#define T3_DIV_BY_1         0
#define T3_DIV_BY_2         0x10
#define T3_DIV_BY_4         0x20
#define T3_DIV_BY_8         0x30
#define SETUP_TIMER3(mode) T3CON = (mode)

#define CCP_OFF                         0
#define CCP_CAPTURE_FE                  4
#define CCP_CAPTURE_RE                  5
#define CCP_CAPTURE_DIV_4               6
#define CCP_CAPTURE_DIV_16              7
#define CCP_COMPARE_SET_ON_MATCH        8
#define CCP_COMPARE_CLR_ON_MATCH        9
#define CCP_COMPARE_INT                 0xA
#define CCP_COMPARE_RESET_TIMER         0xB
#define CCP_PWM        0xC
#define CCP_PWM_PLUS_1 0x1c
#define CCP_PWM_PLUS_2 0x2c
#define CCP_PWM_PLUS_3 0x3c
#define SETUP_CCP1(mode) CCP1CON = (mode)
#define SETUP_CCP2(mode) CCP2CON = (mode)

#define WATCHDOG_TIMER() \
{ \
  _asm \
  CLRWDT \
  _endasm \
}

#define GLOBAL_INT_ENABLE() INTCONbits.GIE = 1
#define GLOBAL_INT_DISABLE() INTCONbits.GIE = 0

#define PERIPHERAL_INT_ENABLE() INTCONbits.PEIE = 1
#define PERIPHERAL_INT_DISABLE() INTCONbits.PEIE = 0

#define TIMER0_INT_ENABLE() INTCONbits.TMR0IE = 1
#define TIMER0_INT_DISABLE() INTCONbits.TMR0IE = 0

#define TIMER2_INT_ENABLE() PIE1bits.TMR2IE = 1
#define TIMER2_INT_DISABLE() PIE1bits.TMR2IE = 0

#define CCP2_INT_ENABLE() PIE2bits.CCP2IE = 1
#define CCP2_INT_DISABLE() PIE2bits.CCP2IE = 0

#define CCP1_INT_ENABLE() PIE1bits.CCP1IE = 1
#define CCP1_INT_DISABLE() PIE1bits.CCP1IE = 0

#define ABUS_INT_ENABLE() PIE1bits.SSPIE = 1
#define ABUS_INT_DISABLE() PIE1bits.SSPIE = 0
#define ABUS_INT_FLAG_CLEAR() PIR1bits.SSPIF = 0

#define USART_RX_INT_DISABLE() PIE1bits.RCIE = 0
#define USART_RX_INT_ENABLE() PIE1bits.RCIE = 1

#define USART_TX_INTERRUPT() PIE1bits.TXIE
#define USART_TX_INT_DISABLE() PIE1bits.TXIE = 0
#define USART_TX_INT_ENABLE()  PIE1bits.TXIE = 1
#define USART_TX_ENABLE() TXSTAbits.TXEN = 1
#define USART_TX_INT_FLAG_CLEAR() PIR1bits.TXIF = 0
#define USART_TX_EMPTY() TXSTAbits.TRMT
#define USART_CONTINUOUS_RX_ENABLE() RCSTAbits.CREN = 1
#define USART_CONTINUOUS_RX_DISABLE() RCSTAbits.CREN = 0
#define USART_RX_COMPLETE() PIR1bits.RCIF
#define USART_RX_STATUS() RCSTAbits
#define USART_RX_STATUS() RCSTAbits
#define USART_TRANSMIT(x) TXREG = (x)
#define USART_RECEIVE() RCREG
#define USART_RX_FRAME_ERROR() rcstabits.FERR
// combine the sequence correctly
#define USART_RX_SETUP() PIE1bits.RCIE = 1; RCSTAbits.CREN = 1 
#define USART_TX_SETUP() PIE1bits.TXIE = 1; TXSTAbits.TXEN = 1


#endif /* HARDWARE_H */
