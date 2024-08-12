/**************************************************************************
*
* Copyright (C) 2012 Steve Karg <skarg@users.sourceforge.net>
*
* SPDX-License-Identifier: MIT
*
*********************************************************************/
#ifndef BITS_H
#define BITS_H

#include <zephyr/sys/util.h>  /* defines BIT(n) */

/********************************************************************
* Bit Masks
*********************************************************************/
#define BIT0   BIT( 0)
#define BIT1   BIT( 1)
#define BIT2   BIT( 2)
#define BIT3   BIT( 3)
#define BIT4   BIT( 4)
#define BIT5   BIT( 5)
#define BIT6   BIT( 6)
#define BIT7   BIT( 7)
#define BIT8   BIT( 8)
#define BIT9   BIT( 9)
#define BIT10  BIT(10)
#define BIT11  BIT(11)
#define BIT12  BIT(12)
#define BIT13  BIT(13)
#define BIT14  BIT(14)
#define BIT15  BIT(15)
#define BIT16  BIT(16)
#define BIT17  BIT(17)
#define BIT18  BIT(18)
#define BIT19  BIT(19)
#define BIT20  BIT(20)
#define BIT21  BIT(21)
#define BIT22  BIT(22)
#define BIT23  BIT(23)
#define BIT24  BIT(24)
#define BIT25  BIT(25)
#define BIT26  BIT(26)
#define BIT27  BIT(27)
#define BIT28  BIT(28)
#define BIT29  BIT(29)
#define BIT30  BIT(30)
#define BIT31  BIT(31)

/* a=register, b=bit number to act upon 0-n */
#define BIT_SET(a,b) ((a) |= (1<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1<<(b)))
#define BIT_CHECK(a,b) ((a) & (1<<(b)))

/* x=target variable, y=mask */
#define BITMASK_SET(x,y) ((x) |= (y))
#define BITMASK_CLEAR(x,y) ((x) &= (~(y)))
#define BITMASK_FLIP(x,y) ((x) ^= (y))
#define BITMASK_CHECK(x,y) (((x) & (y)) == (y))

#ifndef _BV
#define _BV(x) (1<<(x))
#endif

#endif
