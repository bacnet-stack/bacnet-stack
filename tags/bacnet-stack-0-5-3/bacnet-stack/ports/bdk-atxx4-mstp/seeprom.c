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
#include <stdlib.h>
#include "hardware.h"
#include "seeprom.h"

/* the SEEPROM chip select bits A2, A1, and A0 are grounded */
/* control byte is 0xAx */
#ifndef SEEPROM_I2C_ADDRESS
#define SEEPROM_I2C_ADDRESS 0xA0
#endif

/* SEEPROM Clock Frequency */
#ifndef SEEPROM_I2C_CLOCK
#define SEEPROM_I2C_CLOCK 400000UL
#endif

/* max number of bytes that can be written in a single write */
#ifndef SEEPROM_PAGE_SIZE
#define SEEPROM_PAGE_SIZE 128
#endif

/* word addressing - is it 8-bit or 16-bit */
#ifndef SEEPROM_WORD_ADDRESS_16BIT
#define SEEPROM_WORD_ADDRESS_16BIT 1
#endif

/* The lower 3 bits of TWSR are reserved on the ATmega163 */
#define TW_STATUS_MASK (_BV(TWS7)|_BV(TWS6)|_BV(TWS5)|_BV(TWS4)|_BV(TWS3))
/* start condition transmitted */
#define TW_START 0x08
/* repeated start condition transmitted */
#define TW_REP_START 0x10
/* ***Master Transmitter*** */
/* SLA+W transmitted, ACK received */
#define TW_MT_SLA_ACK 0x18
/* SLA+W transmitted, NACK received */
#define TW_MT_SLA_NACK 0x20
/* data transmitted, ACK received */
#define TW_MT_DATA_ACK 0x28
/* data transmitted, NACK received */
#define TW_MT_DATA_NACK 0x30
/* arbitration lost in SLA+W or data */
#define TW_MT_ARB_LOST 0x38
/* ***Master Receiver*** */
/* arbitration lost in SLA+R or NACK */
#define TW_MR_ARB_LOST 0x38
/* SLA+R transmitted, ACK received */
#define TW_MR_SLA_ACK 0x40
/* SLA+R transmitted, NACK received */
#define TW_MR_SLA_NACK 0x48
/* data received, ACK returned */
#define TW_MR_DATA_ACK 0x50
/* data received, NACK returned */
#define TW_MR_DATA_NACK 0x58

/* SLA+R address */
#define TW_READ 1
/* SLA+W address */
#define TW_WRITE 0

/*
 * Maximal number of iterations to wait for a device to respond for a
 * selection.  Should be large enough to allow for a pending write to
 * complete, but low enough to properly abort an infinite loop in case
 * a slave is broken or not present at all.  With 100 kHz TWI clock,
 * transfering the start condition and SLA+R/W packet takes about 10
 * µs.  The longest write period is supposed to not exceed ~ 10 ms.
 * Thus, normal operation should not require more than 100 iterations
 * to get the device to respond to a selection.
 */
#define MAX_ITER        200

/*************************************************************************
* DESCRIPTION: Return bytes from SEEPROM memory at address
* RETURN: number of bytes read, or -1 on error
* NOTES: none
**************************************************************************/
int seeprom_bytes_read(
    uint16_t eeaddr,    /* SEEPROM starting memory address */
    uint8_t * buf,      /* data to store */
    int len)
{       /* number of bytes of data to read */
    uint8_t sla, twcr, n = 0;
    int rv = 0;
    uint8_t twst;       /* status - only valid while TWINT is set. */

#if SEEPROM_WORD_ADDRESS_16BIT
    /* 16bit address devices need only TWI Device Address */
    sla = SEEPROM_I2C_ADDRESS;
#else
    /* patch high bits of EEPROM address into SLA */
    sla = SEEPROM_I2C_ADDRESS | (((eeaddr >> 8) & 0x07) << 1);
#endif
    /* Note [8] First cycle: master transmitter mode */
  restart:
    if (n++ >= MAX_ITER) {
        return -1;
    }
  begin:
    /* send start condition */
    TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
    /* wait for transmission */
    while ((TWCR & _BV(TWINT)) == 0);
    twst = TWSR & TW_STATUS_MASK;
    switch (twst) {
        case TW_REP_START:
            /* OK, but should not happen */
        case TW_START:
            break;
        case TW_MT_ARB_LOST:
            /* Note [9] */
            goto begin;
        default:
            /* error: not in start condition */
            /* NB: do /not/ send stop condition */
            return -1;
    }

    /* Note [10] */
    /* send SLA+W */
    TWDR = sla | TW_WRITE;
    /* clear interrupt to start transmission */
    TWCR = _BV(TWINT) | _BV(TWEN);
    /* wait for transmission */
    while ((TWCR & _BV(TWINT)) == 0);
    twst = TWSR & TW_STATUS_MASK;
    switch (twst) {
        case TW_MT_SLA_ACK:
            break;
        case TW_MT_SLA_NACK:
            /* nack during select: device busy writing */
            /* Note [11] */
            goto restart;
        case TW_MT_ARB_LOST:
            /* re-arbitrate */
            goto begin;
        default:
            /* must send stop condition */
            goto error;
    }
#if SEEPROM_WORD_ADDRESS_16BIT
    /* 16 bit word address device, send high 8 bits of addr */
    TWDR = (eeaddr >> 8);
    /* clear interrupt to start transmission */
    TWCR = _BV(TWINT) | _BV(TWEN);
    /* wait for transmission */
    while ((TWCR & _BV(TWINT)) == 0);
    twst = TWSR & TW_STATUS_MASK;
    switch (twst) {
        case TW_MT_DATA_ACK:
            break;
        case TW_MT_DATA_NACK:
            goto quit;
        case TW_MT_ARB_LOST:
            goto begin;
        default:
            /* must send stop condition */
            goto error;
    }
#endif
    /* low 8 bits of addr */
    TWDR = eeaddr;
    /* clear interrupt to start transmission */
    TWCR = _BV(TWINT) | _BV(TWEN);
    /* wait for transmission */
    while ((TWCR & _BV(TWINT)) == 0);
    twst = TWSR & TW_STATUS_MASK;
    switch (twst) {
        case TW_MT_DATA_ACK:
            break;
        case TW_MT_DATA_NACK:
            goto quit;
        case TW_MT_ARB_LOST:
            goto begin;
        default:
            /* must send stop condition */
            goto error;
    }

    /* Note [12] Next cycle(s): master receiver mode */
    /* send repeated start condition */
    TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN);
    /* wait for transmission */
    while ((TWCR & _BV(TWINT)) == 0);
    twst = TWSR & TW_STATUS_MASK;
    switch (twst) {
        case TW_START:
            /* OK, but should not happen */
        case TW_REP_START:
            break;
        case TW_MT_ARB_LOST:
            goto begin;
        default:
            goto error;
    }

    /* send SLA+R */
    TWDR = sla | TW_READ;
    /* clear interrupt to start transmission */
    TWCR = _BV(TWINT) | _BV(TWEN);
    /* wait for transmission */
    while ((TWCR & _BV(TWINT)) == 0);
    twst = TWSR & TW_STATUS_MASK;
    switch (twst) {
        case TW_MR_SLA_ACK:
            break;
        case TW_MR_SLA_NACK:
            goto quit;
        case TW_MR_ARB_LOST:
            goto begin;
        default:
            goto error;
    }
    /* Note [13] */
    twcr = _BV(TWINT) | _BV(TWEN) | _BV(TWEA);
    for (; len > 0; len--) {
        if (len == 1) {
            /* send NAK this time */
            twcr = _BV(TWINT) | _BV(TWEN);
        }
        /* clear int to start transmission */
        TWCR = twcr;
        /* wait for transmission */
        while ((TWCR & _BV(TWINT)) == 0);
        twst = TWSR & TW_STATUS_MASK;
        switch (twst) {
            case TW_MR_DATA_NACK:
                /* force end of loop */
                len = 0;
                /* FALLTHROUGH */
            case TW_MR_DATA_ACK:
                *buf = TWDR;
                buf++;
                rv++;
                break;
            default:
                goto error;
        }
    }
  quit:
    /* Note [14] */
    /* send stop condition */
    TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN);
    return rv;
  error:
    rv = -1;
    goto quit;
}

/*************************************************************************
* DESCRIPTION: Write some data and wait until it is sent
* RETURN: number of bytes written, or -1 on error
* NOTES: none
**************************************************************************/
int seeprom_bytes_write(
    uint16_t eeaddr,    /* SEEPROM starting memory address */
    uint8_t * buf,      /* data to send */
    int len)
{       /* number of bytes of data */
    uint8_t sla, n = 0;
    int rv = 0;
    uint16_t endaddr;
    uint8_t twst;       /* status - only valid while TWINT is set. */

    if ((eeaddr + len) < (eeaddr | (SEEPROM_PAGE_SIZE - 1))) {
        endaddr = eeaddr + len;
    } else {
        endaddr = (eeaddr | (SEEPROM_PAGE_SIZE - 1)) + 1;
    }
    len = endaddr - eeaddr;
#if SEEPROM_WORD_ADDRESS_16BIT
    /* 16bit address devices need only TWI Device Address */
    sla = SEEPROM_I2C_ADDRESS;
#else
    /* patch high bits of EEPROM address into SLA */
    sla = SEEPROM_I2C_ADDRESS | (((eeaddr >> 8) & 0x07) << 1);
#endif
  restart:
    if (n++ >= MAX_ITER) {
        return -1;
    }
  begin:
    /* Note [15] */
    TWCR = _BV(TWINT) | _BV(TWSTA) | _BV(TWEN); /* send start condition */
    while ((TWCR & _BV(TWINT)) == 0);   /* wait for transmission */
    twst = TWSR & TW_STATUS_MASK;
    switch (twst) {
        case TW_REP_START:
            /* OK, but should not happen */
        case TW_START:
            break;
        case TW_MT_ARB_LOST:
            goto begin;
        default:
            /* error: not in start condition */
            /* NB: do /not/ send stop condition */
            return -1;
    }
    /* send SLA+W */
    TWDR = sla | TW_WRITE;
    /* clear interrupt to start transmission */
    TWCR = _BV(TWINT) | _BV(TWEN);
    /* wait for transmission */
    while ((TWCR & _BV(TWINT)) == 0);
    twst = TWSR & TW_STATUS_MASK;
    switch (twst) {
        case TW_MT_SLA_ACK:
            break;
        case TW_MT_SLA_NACK:
            /* nack during select: device busy writing */
            goto restart;
        case TW_MT_ARB_LOST:
            /* re-arbitrate */
            goto begin;
        default:
            /* must send stop condition */
            goto error;
    }
#if SEEPROM_WORD_ADDRESS_16BIT
    /* 16 bit word address device, send high 8 bits of addr */
    TWDR = (eeaddr >> 8);
    /* clear interrupt to start transmission */
    TWCR = _BV(TWINT) | _BV(TWEN);
    /* wait for transmission */
    while ((TWCR & _BV(TWINT)) == 0);
    twst = TWSR & TW_STATUS_MASK;
    switch (twst) {
        case TW_MT_DATA_ACK:
            break;
        case TW_MT_DATA_NACK:
            goto quit;
        case TW_MT_ARB_LOST:
            goto begin;
        default:
            /* must send stop condition */
            goto error;
    }
#endif
    /* low 8 bits of addr */
    TWDR = eeaddr;
    /* clear interrupt to start transmission */
    TWCR = _BV(TWINT) | _BV(TWEN);
    /* wait for transmission */
    while ((TWCR & _BV(TWINT)) == 0) {
    };
    twst = TWSR & TW_STATUS_MASK;
    switch (twst) {
        case TW_MT_DATA_ACK:
            break;
        case TW_MT_DATA_NACK:
            goto quit;
        case TW_MT_ARB_LOST:
            goto begin;
        default:
            /* must send stop condition */
            goto error;
    }
    for (; len > 0; len--) {
        TWDR = *buf++;
        /* start transmission */
        TWCR = _BV(TWINT) | _BV(TWEN);
        /* wait for transmission */
        while ((TWCR & _BV(TWINT)) == 0);
        twst = TWSR & TW_STATUS_MASK;
        switch (twst) {
            case TW_MT_DATA_NACK:
                /* device write protected -- Note [16] */
                goto error;
            case TW_MT_DATA_ACK:
                rv++;
                break;
            default:
                goto error;
        }
    }
  quit:
    /* send stop condition */
    TWCR = _BV(TWINT) | _BV(TWSTO) | _BV(TWEN);

    return rv;

  error:
    rv = -1;
    goto quit;
}

/*************************************************************************
* Description: Initialize the SEEPROM TWI connection
* Returns: none
* Notes: none
**************************************************************************/
void seeprom_init(
    void)
{
    /* bit rate prescaler */
    TWSR = 0;
    TWCR = _BV(TWEN) | _BV(TWEA);
    /* bit rate */
    TWBR = (F_CPU / SEEPROM_I2C_CLOCK - 16) / 2;
    /* my address */
    TWAR = 0;
}
