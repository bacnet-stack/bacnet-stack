/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @brief Bitwise helper macros
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_SYS_BITS_H
#define BACNET_SYS_BITS_H

/********************************************************************
* Bit Masks
*********************************************************************/
#ifndef BIT
#define BIT(x)  (1<<(x))
#endif
#ifndef _BV
#define _BV(x)  (1<<(x))
#endif

/* a=register, b=bit number to act upon 0-n */
#ifndef BIT_SET
#define BIT_SET(a,b) ((a) |= (1<<(b)))
#endif
#ifndef BIT_CLEAR
#define BIT_CLEAR(a,b) ((a) &= ~(1<<(b)))
#endif
#ifndef BIT_FLIP
#define BIT_FLIP(a,b) ((a) ^= (1<<(b)))
#endif
#ifndef BIT_CHECK
#define BIT_CHECK(a,b) ((a) & (1<<(b)))
#endif

/* x=target variable, y=mask */
#ifndef BITMASK_SET
#define BITMASK_SET(x,y) ((x) |= (y))
#endif
#ifndef BITMASK_CLEAR
#define BITMASK_CLEAR(x,y) ((x) &= (~(y)))
#endif
#ifndef BITMASK_FLIP
#define BITMASK_FLIP(x,y) ((x) ^= (y))
#endif
#ifndef BITMASK_CHECK
#define BITMASK_CHECK(x,y) (((x) & (y)) == (y))
#endif

#endif
