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
#include "datalink.h"
#include "npdu.h"
#include "txbuf.h"
#include "iam.h"

/* For porting to IAR, see:
   http://www.avrfreaks.net/wiki/index.php/Documentation:AVR_GCC/IarToAvrgcc*/

/* Notice: Fuse Bit Settings

    

 */


void init(void)
{
    /* Initialize the Clock Prescaler for ATmega48/88/168 */
    /* The default CLKPSx bits are factory set to 0011 */
    /* Enbable the Clock Prescaler */
    CLKPR = _BV(CLKPCE);
    /* CLKPS3 CLKPS2 CLKPS1 CLKPS0 Clock Division Factor
       ------ ------ ------ ------ ---------------------
          0      0      0      0             1
          0      0      0      1             2
          0      0      1      0             4
          0      0      1      1             8
          0      1      0      0            16
          0      1      0      1            32
          0      1      1      0            64
          0      1      1      1           128
          1      0      0      0           256
          1      x      x      x      Reserved
    */
    /* Set the CLKPS3..0 bits to Prescaler of 1 */
    CLKPR = 0;
    /* Initialize I/O ports */
    /* For Port DDRx (Data Direction) Input=0, Output=1 */
    /* For Port PORTx (Bit Value) TriState=0, High=1 */
    DDRB = 0;
    PORTB = 0;
    DDRC = 0;
    PORTC = 0;
    DDRD = 0;
    PORTD = 0;

    /* Configure the watchdog timer - Disabled for testing */
    BIT_CLEAR(MCUSR,WDRF);
    WDTCSR = 0;

    /* Configure USART */
    RS485_Initialize();
    RS485_Set_Baud_Rate(38400);

    /* Configure Timer0 for millisecond timer */
    Timer_Initialize();

    /* Set the LED ports OFF */
    BIT_SET(PORTD,PD4);
    BIT_SET(PORTD,PD5);
    /* Configure the LED ports as outputs */
    BIT_SET(DDRD,DDD4);
    BIT_SET(DDRD,DDD5);

    /* Enable global interrupts */
    sei();
}

static uint8_t NPDU_Timer;

static void NDPU_Timers(void)
{
    if (NPDU_Timer) {
        NPDU_Timer--;
        if (NPDU_Timer == 0) {
            BIT_SET(PORTD,PD5);
        }
    }
}

static void NPDU_LED_On(void)
{
    BIT_CLEAR(PORTD,PD5);
    NPDU_Timer = 20;
}

void task_milliseconds(void)
{
    while (Timer_Milliseconds) {
        Timer_Milliseconds--;
        /* add other millisecond timer tasks here */
        RS485_LED_Timers();
        NDPU_Timers();
    }
}

static uint8_t Address_Switch;

void input_switch_read(void)
{
    uint8_t value;
    static uint8_t old_value = 0;

    value = BITMASK_CHECK(PINC,0x0F) | (BITMASK_CHECK(PINB,0x07)<<4);
    if (value != old_value) {
        old_value = value;
    } else {
        if (old_value != Address_Switch) {
           Address_Switch = old_value;
#if defined(BACDL_MSTP)
           dlmstp_set_mac_address(Address_Switch);
#endif
        }
    }
}

#if defined(TEST_MSTP)
void apdu_handler(BACNET_ADDRESS * src,     /* source address */
        uint8_t * apdu,         /* APDU data */
        uint16_t pdu_len)      /* for confirmed messages */
{
    (void)src;
    (void)apdu;
    (void)pdu_len;
}
#endif

static uint8_t PDUBuffer[MAX_MPDU];
int main(void)
{
    uint16_t pdu_len = 0;
    BACNET_ADDRESS src; /* source address */
    
    init();
#if defined(BACDL_MSTP)
    RS485_Set_Baud_Rate(38400);
    dlmstp_set_max_master(127);
    dlmstp_set_max_info_frames(1);
#endif
    datalink_init(NULL);
    /* broadcast an I-Am on startup */
#if !defined(TEST_MSTP)
    iam_send(&Handler_Transmit_Buffer[0]);
#endif
    for (;;) {
        input_switch_read();
        task_milliseconds();
        /* other tasks */
        /* BACnet handling */
        pdu_len = datalink_receive(&src, &PDUBuffer[0], sizeof(PDUBuffer), 0);
        if (pdu_len) {
#if !defined(TEST_MSTP)
            npdu_handler(&src, &PDUBuffer[0], pdu_len);
#endif
            NPDU_LED_On();
        }
    }

    return 0;
}
