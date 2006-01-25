/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
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

#include <stddef.h>
#include <stdint.h>
#include "mstp.h"
#include "bytes.h"
#include "crc.h"
#include "rs485.h"
#include "ringbuf.h"
#include "init.h"
#include "timer.h"
#include "hardware.h"

volatile struct mstp_port_struct_t MSTP_Port; // port data
static uint8_t MSTP_MAC_Address = 0x05; // local MAC address

/****************************************************************************
* DESCRIPTION: Handles our calling our module level milisecond counters
* PARAMETERS:  none
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
static void Check_Timer_Milliseconds(void)
{
  // We might have missed some so keep doing it until we have got them all
  while (Milliseconds)
  {
    MSTP_Millisecond_Timer(&MSTP_Port);
    Milliseconds--;
  }
}


void main(void)
{
  init_hardware();
  RS485_Initialize();
  MSTP_Init(&MSTP_Port,MSTP_MAC_Address);

  // loop forever
  for (;;)
  {
    WATCHDOG_TIMER();

    // input
    Check_Timer_Milliseconds();
    // note: also called by RS-485 Receive ISR
    RS485_Check_UART_Data(&MSTP_Port);
    MSTP_Receive_Frame_FSM(&MSTP_Port);

    // process


    // output
    RS485_Process_Tx_Message();
    MSTP_Master_Node_FSM(&MSTP_Port);
    
  }
  
  return;
}
