/**************************************************************************
*
* Copyright (C) 2012 Steve Karg <skarg@users.sourceforge.net>
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
#ifndef BITS_H
#define BITS_H

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
