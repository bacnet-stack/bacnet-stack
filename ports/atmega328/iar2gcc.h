/**
 * @brief This module manages the common IAR and GCC compiler differences
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef IAR2GCC_H
#define IAR2GCC_H

#if !defined(F_CPU)
#error You must define F_CPU - clock frequency!
#endif
/* IAR */
#if defined(__IAR_SYSTEMS_ICC__) || defined(__IAR_SYSTEMS_ASM__)
#include <inavr.h>
#include <ioavr.h>
#include <intrinsics.h>
#include <stdint.h>
/* BitValue is used alot in GCC examples */
#ifndef _BV
#define _BV(bit_num) (1 << (bit_num))
#endif

/* inline function */
static inline void _delay_us(
    uint8_t microseconds)
{
    do {
        __delay_cycles(F_CPU / 1000000UL);
    } while (microseconds--);
}

#if (__VER__ > 700)
#define DDA0 DDRA0
#define DDA1 DDRA1
#define DDA2 DDRA2
#define DDA3 DDRA3
#define DDA4 DDRA4
#define DDA5 DDRA5
#define DDA6 DDRA6
#define DDA7 DDRA7

#define DDB0 DDRB0
#define DDB1 DDRB1
#define DDB2 DDRB2
#define DDB3 DDRB3
#define DDB4 DDRB4
#define DDB5 DDRB5
#define DDB6 DDRB6
#define DDB7 DDRB7

#define DDC0 DDRC0
#define DDC1 DDRC1
#define DDC2 DDRC2
#define DDC3 DDRC3
#define DDC4 DDRC4
#define DDC5 DDRC5
#define DDC6 DDRC6
#define DDC7 DDRC7

#define DDD0 DDRD0
#define DDD1 DDRD1
#define DDD2 DDRD2
#define DDD3 DDRD3
#define DDD4 DDRD4
#define DDD5 DDRD5
#define DDD6 DDRD6
#define DDD7 DDRD7
#endif
#endif

#if defined(__GNUC__)
#include <util/delay.h>
#endif

/* adjust some definitions to common versions */
#if defined (__CROSSWORKS_AVR)
#if (__TARGET_PROCESSOR == ATmega644P)
#define PRR PRR0
#define UBRR0 UBRR0W
#define UBRR1 UBRR1W

#define PA0 PORTA0
#define PA1 PORTA1
#define PA2 PORTA2
#define PA3 PORTA3
#define PA4 PORTA4
#define PA5 PORTA5
#define PA6 PORTA6
#define PA7 PORTA7

#define PB0 PORTB0
#define PB1 PORTB1
#define PB2 PORTB2
#define PB3 PORTB3
#define PB4 PORTB4
#define PB5 PORTB5
#define PB6 PORTB6
#define PB7 PORTB7

#define PC0 PORTC0
#define PC1 PORTC1
#define PC2 PORTC2
#define PC3 PORTC3
#define PC4 PORTC4
#define PC5 PORTC5
#define PC6 PORTC6
#define PC7 PORTC7

#define PD0 PORTD0
#define PD1 PORTD1
#define PD2 PORTD2
#define PD3 PORTD3
#define PD4 PORTD4
#define PD5 PORTD5
#define PD6 PORTD6
#define PD7 PORTD7

#endif
#endif

/* Input/Output Registers */
#if defined(__GNUC__)
#include <avr/io.h>

typedef struct {
    unsigned char bit0 : 1;
    unsigned char bit1 : 1;
    unsigned char bit2 : 1;
    unsigned char bit3 : 1;
    unsigned char bit4 : 1;
    unsigned char bit5 : 1;
    unsigned char bit6 : 1;
    unsigned char bit7 : 1;
} BitRegisterType;

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif

#define GPIO_BITREG(port, bitnum) \
    ((volatile BitRegisterType *)_SFR_MEM_ADDR(port))->bit##bitnum

#define PINA_Bit0 GPIO_BITREG(PINA, 0)
#define PINA_Bit1 GPIO_BITREG(PINA, 1)
#define PINA_Bit2 GPIO_BITREG(PINA, 2)
#define PINA_Bit3 GPIO_BITREG(PINA, 3)
#define PINA_Bit4 GPIO_BITREG(PINA, 4)
#define PINA_Bit5 GPIO_BITREG(PINA, 5)
#define PINA_Bit6 GPIO_BITREG(PINA, 6)
#define PINA_Bit7 GPIO_BITREG(PINA, 7)

#define PORTA_Bit0 GPIO_BITREG(PORTA, 0)
#define PORTA_Bit1 GPIO_BITREG(PORTA, 1)
#define PORTA_Bit2 GPIO_BITREG(PORTA, 2)
#define PORTA_Bit3 GPIO_BITREG(PORTA, 3)
#define PORTA_Bit4 GPIO_BITREG(PORTA, 4)
#define PORTA_Bit5 GPIO_BITREG(PORTA, 5)
#define PORTA_Bit6 GPIO_BITREG(PORTA, 6)
#define PORTA_Bit7 GPIO_BITREG(PORTA, 7)

#define PINB_Bit0 GPIO_BITREG(PINB, 0)
#define PINB_Bit1 GPIO_BITREG(PINB, 1)
#define PINB_Bit2 GPIO_BITREG(PINB, 2)
#define PINB_Bit3 GPIO_BITREG(PINB, 3)
#define PINB_Bit4 GPIO_BITREG(PINB, 4)
#define PINB_Bit5 GPIO_BITREG(PINB, 5)
#define PINB_Bit6 GPIO_BITREG(PINB, 6)
#define PINB_Bit7 GPIO_BITREG(PINB, 7)

#define PORTB_Bit0 GPIO_BITREG(PORTB, 0)
#define PORTB_Bit1 GPIO_BITREG(PORTB, 1)
#define PORTB_Bit2 GPIO_BITREG(PORTB, 2)
#define PORTB_Bit3 GPIO_BITREG(PORTB, 3)
#define PORTB_Bit4 GPIO_BITREG(PORTB, 4)
#define PORTB_Bit5 GPIO_BITREG(PORTB, 5)
#define PORTB_Bit6 GPIO_BITREG(PORTB, 6)
#define PORTB_Bit7 GPIO_BITREG(PORTB, 7)

#define PINC_Bit0 GPIO_BITREG(PINC, 0)
#define PINC_Bit1 GPIO_BITREG(PINC, 1)
#define PINC_Bit2 GPIO_BITREG(PINC, 2)
#define PINC_Bit3 GPIO_BITREG(PINC, 3)
#define PINC_Bit4 GPIO_BITREG(PINC, 4)
#define PINC_Bit5 GPIO_BITREG(PINC, 5)
#define PINC_Bit6 GPIO_BITREG(PINC, 6)
#define PINC_Bit7 GPIO_BITREG(PINC, 7)

#define PORTC_Bit0 GPIO_BITREG(PORTC, 0)
#define PORTC_Bit1 GPIO_BITREG(PORTC, 1)
#define PORTC_Bit2 GPIO_BITREG(PORTC, 2)
#define PORTC_Bit3 GPIO_BITREG(PORTC, 3)
#define PORTC_Bit4 GPIO_BITREG(PORTC, 4)
#define PORTC_Bit5 GPIO_BITREG(PORTC, 5)
#define PORTC_Bit6 GPIO_BITREG(PORTC, 6)
#define PORTC_Bit7 GPIO_BITREG(PORTC, 7)

#define PIND_Bit0 GPIO_BITREG(PIND, 0)
#define PIND_Bit1 GPIO_BITREG(PIND, 1)
#define PIND_Bit2 GPIO_BITREG(PIND, 2)
#define PIND_Bit3 GPIO_BITREG(PIND, 3)
#define PIND_Bit4 GPIO_BITREG(PIND, 4)
#define PIND_Bit5 GPIO_BITREG(PIND, 5)
#define PIND_Bit6 GPIO_BITREG(PIND, 6)
#define PIND_Bit7 GPIO_BITREG(PIND, 7)

#define PORTD_Bit0 GPIO_BITREG(PORTD, 0)
#define PORTD_Bit1 GPIO_BITREG(PORTD, 1)
#define PORTD_Bit2 GPIO_BITREG(PORTD, 2)
#define PORTD_Bit3 GPIO_BITREG(PORTD, 3)
#define PORTD_Bit4 GPIO_BITREG(PORTD, 4)
#define PORTD_Bit5 GPIO_BITREG(PORTD, 5)
#define PORTD_Bit6 GPIO_BITREG(PORTD, 6)
#define PORTD_Bit7 GPIO_BITREG(PORTD, 7)

#define GPIOR0_Bit0 GPIO_BITREG(GPIOR0, 0)
#define GPIOR0_Bit1 GPIO_BITREG(GPIOR0, 1)
#define GPIOR0_Bit2 GPIO_BITREG(GPIOR0, 2)
#define GPIOR0_Bit3 GPIO_BITREG(GPIOR0, 3)
#define GPIOR0_Bit4 GPIO_BITREG(GPIOR0, 4)
#define GPIOR0_Bit5 GPIO_BITREG(GPIOR0, 5)
#define GPIOR0_Bit6 GPIO_BITREG(GPIOR0, 6)
#define GPIOR0_Bit7 GPIO_BITREG(GPIOR0, 7)

#define GPIOR1_Bit0 GPIO_BITREG(GPIOR1, 0)
#define GPIOR1_Bit1 GPIO_BITREG(GPIOR1, 1)
#define GPIOR1_Bit2 GPIO_BITREG(GPIOR1, 2)
#define GPIOR1_Bit3 GPIO_BITREG(GPIOR1, 3)
#define GPIOR1_Bit4 GPIO_BITREG(GPIOR1, 4)
#define GPIOR1_Bit5 GPIO_BITREG(GPIOR1, 5)
#define GPIOR1_Bit6 GPIO_BITREG(GPIOR1, 6)
#define GPIOR1_Bit7 GPIO_BITREG(GPIOR1, 7)

#define GPIOR2_Bit0 GPIO_BITREG(GPIOR2, 0)
#define GPIOR2_Bit1 GPIO_BITREG(GPIOR2, 1)
#define GPIOR2_Bit2 GPIO_BITREG(GPIOR2, 2)
#define GPIOR2_Bit3 GPIO_BITREG(GPIOR2, 3)
#define GPIOR2_Bit4 GPIO_BITREG(GPIOR2, 4)
#define GPIOR2_Bit5 GPIO_BITREG(GPIOR2, 5)
#define GPIOR2_Bit6 GPIO_BITREG(GPIOR2, 6)
#define GPIOR2_Bit7 GPIO_BITREG(GPIOR2, 7)

#endif

/* Global Interrupts */
#if defined(__GNUC__)
#define __enable_interrupt() sei()
#define __disable_interrupt() cli()
#endif

/* Interrupts */
#if defined(__ICCAVR__)
#define PRAGMA(x) _Pragma(#x)
#define ISR(vec) \
    /* function prototype for use with "require protoptypes" option.  */ \
    PRAGMA( vector=vec ) __interrupt void handler_##vec(void); \
    PRAGMA( vector=vec ) __interrupt void handler_##vec(void)
#elif defined(__GNUC__)
#include <avr/interrupt.h>
#endif

/* Flash */
#if defined(__ICCAVR__)
#define FLASH_DECLARE(x) __flash x
#elif defined(__GNUC__)
#define FLASH_DECLARE(x) x __attribute__((__progmem__))
#endif

/* EEPROM */
#if defined(__ICCAVR__)
#define EEPROM_DECLARE(x) __eeprom x
#endif
#if defined(__GNUC__)
#include <avr/eeprom.h>
#define EEPROM_DECLARE(x) x __attribute__((section(".eeprom")))
#endif

/* IAR intrinsic routines */
#if defined(__GNUC__)
/* FIXME: intrinsic routines: map to assembler for size/speed */
#define __multiply_unsigned(x, y) ((x) * (y))
/* FIXME: __root means to not optimize or strip */
#define __root
#endif

#endif
