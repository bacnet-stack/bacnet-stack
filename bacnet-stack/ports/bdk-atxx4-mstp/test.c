/**************************************************************************
*
* Copyright (C) 2010 Steve Karg <skarg@users.sourceforge.net>
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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "hardware.h"
#include "timer.h"
#include "serial.h"
#include "input.h"
#include "bo.h"

/* timer for test task */
static struct itimer Test_Timer;
/* MAC Address of MS/TP */
static uint8_t MSTP_MAC_Address;

void test_init(
    void)
{
    timer_interval_start_seconds(&Test_Timer, 1);
}

void test_task(
    void)
{
    char buffer[32] = "BACnet: 0000000\r\n";
    uint8_t nbytes = 17;
    uint8_t data_register = 0;

    if (timer_interval_expired(&Test_Timer)) {
        timer_interval_reset(&Test_Timer);
        MSTP_MAC_Address = input_address();
        buffer[8] = (MSTP_MAC_Address & BIT0) ? '1' : '0';
        buffer[9] = (MSTP_MAC_Address & BIT1) ? '1' : '0';
        buffer[10] = (MSTP_MAC_Address & BIT2) ? '1' : '0';
        buffer[11] = (MSTP_MAC_Address & BIT3) ? '1' : '0';
        buffer[12] = (MSTP_MAC_Address & BIT4) ? '1' : '0';
        buffer[13] = (MSTP_MAC_Address & BIT5) ? '1' : '0';
        buffer[14] = (MSTP_MAC_Address & BIT6) ? '1' : '0';
        serial_bytes_send((uint8_t *) buffer, nbytes);
    }
    if (serial_byte_get(&data_register)) {
        /* echo the character */
        serial_byte_send(data_register);
        if (data_register == '0') {
            Binary_Output_Present_Value_Set(0, 0, BINARY_INACTIVE);
            Binary_Output_Present_Value_Set(1, 0, BINARY_INACTIVE);
        }
        if (data_register == '1') {
            Binary_Output_Present_Value_Set(0, 0, BINARY_ACTIVE);
            Binary_Output_Present_Value_Set(1, 0, BINARY_ACTIVE);
        }
        if (data_register == '2') {
            Binary_Output_Present_Value_Set(0, 0, BINARY_NULL);
            Binary_Output_Present_Value_Set(1, 0, BINARY_NULL);
        }
        serial_byte_send('\r');
        serial_byte_send('\n');
        serial_byte_transmit_complete();
    }
}
