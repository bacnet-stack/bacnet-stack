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
*
*********************************************************************/
#include "hardware.h"

static uint8_t Address_Switch;

void input_task(
    void)
{
    uint8_t value;
    static uint8_t old_value = 0;

    value = BITMASK_CHECK(PINA, 0x7F);
    if (value != old_value) {
        old_value = value;
    } else {
        if (old_value != Address_Switch) {
            Address_Switch = old_value;
        }
    }
}

uint8_t input_address(void)
{
    return Address_Switch;
}

void input_init(void)
{
    /* configure the port pins */
    BITMASK_CLEAR(DDRA, 
        _BV(DDA0) |
        _BV(DDA1) |
        _BV(DDA2) |
        _BV(DDA3) |
        _BV(DDA4) |
        _BV(DDA5) |
        _BV(DDA6)
        );
}
