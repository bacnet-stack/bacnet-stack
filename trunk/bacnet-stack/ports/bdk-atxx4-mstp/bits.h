/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2004 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to 
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330 
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#ifndef BITS_H
#define BITS_H

/********************************************************************
* Bit Masks
*********************************************************************/
#define BIT0            (0x01)
#define BIT1            (0x02)
#define BIT2            (0x04)
#define BIT3            (0x08)
#define BIT4            (0x10)
#define BIT5            (0x20)
#define BIT6            (0x40)
#define BIT7            (0x80)
#define BIT8          (0x0100)
#define BIT9          (0x0200)
#define BIT10         (0x0400)
#define BIT11         (0x0800)
#define BIT12         (0x1000)
#define BIT13         (0x2000)
#define BIT14         (0x4000)
#define BIT15         (0x8000)
#define BIT16       (0x010000UL)
#define BIT17       (0x020000UL)
#define BIT18       (0x040000UL)
#define BIT19       (0x080000UL)
#define BIT20       (0x100000UL)
#define BIT21       (0x200000UL)
#define BIT22       (0x400000UL)
#define BIT23       (0x800000UL)
#define BIT24     (0x01000000UL)
#define BIT25     (0x02000000UL)
#define BIT26     (0x04000000UL)
#define BIT27     (0x08000000UL)
#define BIT28     (0x10000000UL)
#define BIT29     (0x20000000UL)
#define BIT30     (0x40000000UL)
#define BIT31     (0x80000000UL)

/* a=register, b=bit number to act upon 0-n */
#define BIT_SET(a,b) ((a) |= (1<<(b)))
#define BIT_CLEAR(a,b) ((a) &= ~(1<<(b)))
#define BIT_FLIP(a,b) ((a) ^= (1<<(b)))
#define BIT_CHECK(a,b) ((a) & (1<<(b)))

/* x=target variable, y=mask */
#define BITMASK_SET(x,y) ((x) |= (y))
#define BITMASK_CLEAR(x,y) ((x) &= (~(y)))
#define BITMASK_FLIP(x,y) ((x) ^= (y))
#define BITMASK_CHECK(x,y) ((x) & (y))

#ifndef _BV
#define _BV(x) (1<<(x))
#endif

#endif
