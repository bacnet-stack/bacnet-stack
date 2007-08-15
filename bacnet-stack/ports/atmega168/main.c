/**************************************************************************
*
* Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
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

#include <stdbool.h>
#include <stdint.h>
#include "hardware.h"
#include "timer.h"
#include "rs485.h"

/* For porting to IAR, see:
   http://www.avrfreaks.net/wiki/index.php/Documentation:AVR_GCC/IarToAvrgcc*/

static uint16_t Transmit_Timer = 0;
#define MAX_FRAME 5
static uint8_t Transmit_Frame[MAX_FRAME] = {0xAA, 0x55, 0x01, 0x45, 0xAB };

void init(void)
{

    /* Initialize I/O ports */
    /* For Port DDRx (Data Direction) Input=1, Output=1 */
    /* For Port PORTx (Bit Value) TriState=0, High=1 */

    DDRB = 0;
    PORTB = 0;
    DDRC = 0;
    PORTC = 0;
    DDRD = 0;
    PORTD = 0;

	/* Configure the watchdog timer */

	/* Configure USART */
    RS485_Set_Baud_Rate(38400);
    RS485_Initialize();

    /* Configure Timer0 for millisecond timer */
    timer_initialize();
}

void task_milliseconds(void)
{
    while (Timer_Milliseconds) {
	    Timer_Milliseconds--;
		/* add other millisecond timer tasks here */
        Transmit_Timer++;
	}
}

int main(void)
{
    init();
	for (;;) {
	    task_milliseconds();
		/* other tasks */
        if (Transmit_Timer > 1000) {
            Transmit_Timer = 0;
            RS485_Send_Frame(NULL,Transmit_Frame, MAX_FRAME);
        }
	}

    return 0;
}
