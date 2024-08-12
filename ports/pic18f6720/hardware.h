/**************************************************************************
*
* Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
*
* SPDX-License-Identifier: MIT
*
*********************************************************************/
#ifndef HARDWARE_H
#define HARDWARE_H

#include <p18f6720.h>
#include <stdint.h>
#include <stdbool.h>

#define RS485_TX_ENABLE         PORTEbits.RE3
#define RS485_RX_DISABLE        PORTGbits.RG0

#define TURN_OFF_COMPARATORS()  CMCON = 0x07

enum INT_STATE { INT_DISABLED, INT_ENABLED, INT_RESTORE };

#define RESTART_WDT()       { _asm CLRWDT _endasm }

/* *************************************************************************
  define ENABLE_GLOBAL_INT() INTCONbits.GIE = 1 �
  #define DISABLE_GLOBAL_INT() INTCONbits.GIE = 0 �
  #define ENABLE_PERIPHERAL_INT() INTCONbits.PEIE = 1 �
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

#endif /* HARDWARE_H */
