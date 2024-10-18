/**************************************************************************
*
* Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
*
* SPDX-License-Identifier: MIT
*
*********************************************************************/
#ifndef HARDWARE_H
#define HARDWARE_H

#include <p18f87j60.h>
#include <stdint.h>
#include <stdbool.h>

/* PORTA.0 Photocell Input PORTA.1 LED Row6 PORTA.2 LED Row5 PORTA.3 LED
 * Row4 PORTA.4 Square Wave input from RTC PORTA.5 LCD RW PORTB.0 Zero
 * Cross PORTB.1 USB RXF# PORTB.2 USB TXE# PORTB.3 Keypad Row Enable
 * (74HC373 Output Control) PORTB.4 Keypad Row Gate (74HC373 Gate)
 * PORTB.5 Switch Input Latch & Keypad Column Gate (74HC373 Gate) PORTB.6
 * ICD connection PORTB.7 ICD connection PORTC.0 Pilot Latch PORTC.1
 * Pilot Output Enable (low) PORTC.2 Piezo PORTC.3 I2C clock PORTC.4 I2C
 * data PORTC.5 RS232 enable (low) PORTC.6 RS232 Tx PORTC.7 RS232 Rx
 * PORTD.0 Data bus PORTD.1 Data bus PORTD.2 Data bus PORTD.3 Data bus
 * PORTD.4 Data bus PORTD.5 Data bus PORTD.6 Data bus PORTD.7 Data bus
 * PORTE.0 USB RD PORTE.1 USB WR PORTE.2 LCD RS PORTE.3 485 transmit
 * enable PORTE.4 Relay data latch PORTE.5 Switch Input Clock PORTE.6
 * Switch Input High/Low PORTE.7 Switch Input Data PORTF.0 LED Row2
 * PORTF.1 LED Row1 PORTF.2 LED Col5 PORTF.3 LED Col4 PORTF.4 LED Col3
 * PORTF.5 LED Col2 PORTF.6 LED Col1 PORTF.7 LED Col0 PORTG.0 485 receive
 * enable PORTG.1 485 Tx PORTG.2 485 Rx PORTG.3 LCD E PORTG.4 LED Row0 */
#define RS485_TX_ENABLE         PORTEbits.RE3
#define RS485_RX_DISABLE        PORTGbits.RG0

#define LEDPORT                 PORTG
#define LEDTRIS                 TRISG
#define LED_ROW1                PORTGbits.RG1
#define LED_ROW2                PORTGbits.RG2
#define LED_ROW3                PORTGbits.RG3
#define LED_ROW4                PORTGbits.RG4

#define TURN_OFF_COMPARATORS()  CMCON = 0x07

enum INT_STATE { INT_DISABLED, INT_ENABLED, INT_RESTORE };

#define RESTART_WDT()       { _asm CLRWDT _endasm }

/* *************************************************************************
  define ENABLE_GLOBAL_INT() INTCONbits.GIE = 1
  #define DISABLE_GLOBAL_INT() INTCONbits.GIE = 0
  #define ENABLE_PERIPHERAL_INT() INTCONbits.PEIE = 1
  #define DISABLE_PERIPHERAL_INT() INTCONbits.PEIE = 0
 *************************************************************************** */
#define ENABLE_HIGH_INT()     INTCONbits.GIE = 1
#define DISABLE_HIGH_INT()    INTCONbits.GIE = 0

#define ENABLE_LOW_INT()      INTCONbits.PEIE = 1
#define DISABLE_LOW_INT()     INTCONbits.PEIE = 0

#define ENABLE_TIMER0_INT()   INTCONbits.TMR0IE = 1
#define DISABLE_TIMER0_INT()  INTCONbits.TMR0IE = 0

#define ENABLE_TIMER2_INT()   PIE1bits.TMR2IE = 1
#define DISABLE_TIMER2_INT()  PIE1bits.TMR2IE = 0

#define ENABLE_TIMER4_INT()   PIE3bits.TMR4IE = 1
#define DISABLE_TIMER4_INT()  PIE3bits.TMR4IE = 0

#define ENABLE_CCP2_INT()     PIE2bits.CCP2IE = 1
#define DISABLE_CCP2_INT()    PIE2bits.CCP2IE = 0

#define ENABLE_CCP1_INT()     PIE1bits.CCP1IE = 1
#define DISABLE_CCP1_INT()    PIE1bits.CCP1IE = 0

#define ENABLE_ABUS_INT()     PIE1bits.SSPIE = 1
#define DISABLE_ABUS_INT()    PIE1bits.SSPIE = 0
#define CLEAR_ABUS_FLAG()     PIR1bits.SSPIF = 0

#define SETUP_CCP1(x)         CCP1CON = x
#define SETUP_CCP2(x)         CCP2CON = x

#define DISABLE_RX_INT()      PIE1bits.RCIE = 0
#define ENABLE_RX_INT()       PIE1bits.RCIE = 1

#define DISABLE_TX_INT()      PIE1bits.TXIE = 0
#define ENABLE_TX_INT()       PIE1bits.TXIE = 1

#if CLOCKSPEED == 20
#define DELAY_US(x)           { _asm                 \
                                  MOVLW x            \
                                LOOP:                \
                                  NOP                \
                                  NOP                \
                                  DECFSZ WREG, 1, 0  \
                                  BRA LOOP           \
                                _endasm }
#endif

#define setup_timer4(mode, period, postscale) \
  T4CON = (mode | (postscale - 1) << 3); \
  PR4 = period

#define setup_timer2(mode, period, postscale) \
  T2CON = (mode | (postscale - 1) << 3); \
  PR2 = period


/* Global Vars */
extern uint8_t Piezo_Timer;
extern volatile bool DataPortLocked;

#endif /* HARDWARE_H */
