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
#include <string.h>             /* for memmove */
#include <p18F6720.h>
#include <stdlib.h>
#include <string.h>
#include "stdint.h"
#include "hardware.h"
/* BACnet */
#include "apdu.h"
#include "datalink.h"
#include "dcc.h"
#include "handlers.h"
#include "iam.h"
#include "txbuf.h"

volatile uint8_t        Milliseconds = 0;
volatile uint8_t        System_Seconds = 0;
volatile uint8_t        Zero_Cross_Timeout = 0;

static void BACnet_Service_Handlers_Init(void)
{
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS,
        handler_who_is);
    /* Set the handlers for any confirmed services that we support. */
    /* We must implement read property - it's required! */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property);
    //apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROPERTY,
    //    handler_write_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_REINITIALIZE_DEVICE,
        handler_reinitialize_device);
    //apdu_set_unconfirmed_handler
    //    (SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION,
    //    handler_timesync_utc);
    //apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION,
    //    handler_timesync);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler
        (SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
}

void Reinitialize(void)
{
  uint8_t i;
  char  name = 0;

  _asm reset _endasm
}

void Global_Int(
  enum INT_STATE  state)  /* FIX ME: add comment */
{
  static uint8_t  intstate = 0;

  switch (state)
  {
    case INT_DISABLED:
      intstate >>= 2;
      intstate |= (INTCON & 0xC0);
      break;
    case INT_ENABLED:
      INTCONbits.GIE = 1;
      INTCONbits.PEIE = 1;
      intstate <<= 2;
      break;
    case INT_RESTORE:
      INTCON |= (intstate & 0xC0);
      intstate <<= 2;
      break;
    default:
      break;
  }
}

void Initialize_Variables(void)
{
  /* Check to see if we need to initialize our eeproms */
  ENABLE_TIMER4_INT();
  /* interrupts must be enabled before we read our inputs */
  Global_Int(INT_ENABLED);
  BACnet_Service_Handlers_Init();
  dlmstp_init();
  /* Start our time from now */
  Milliseconds = 0;
  System_Seconds = 0;
}

void Verify_Ints(void)
{
  /* Make sure the Abus data and clock lines are inputs. Also make sure
   * that global interrupts are enabled. */
  if (!TRISCbits.TRISC3)
    TRISCbits.TRISC3 = 1;
  if (!TRISCbits.TRISC4)
    TRISCbits.TRISC4 = 1;
  if (!INTCONbits.GIE)
    INTCONbits.GIE = 1;
  if (!INTCONbits.PEIE)
    INTCONbits.PEIE = 1;
  /* if (!INTCONbits.INT0E) £
   * INTCONbits.INT0E=1; */
  if (!PIE1bits.TMR2IE)
    PIE1bits.TMR2IE = 1;
}

void MainTasks(void)
{
  /* Handle our millisecond counters */
  while (Milliseconds)
  {
    --Milliseconds;
  }
  /* Handle our seconds counters */
  while (System_Seconds)
  {
    dcc_timer_seconds(1);
    System_Seconds--;
  }
}

void main(void)
{
  /* Note that before main is called, the SCL line is £
   * toggled 256 times to clear any I2C devices that £
   * may be holding the data line low £
   * Reset POR bit */
  RCONbits.NOT_POR = 1;
  RCONbits.NOT_RI = 1;
  Initialize_Variables();
  /* Handle anything that needs to be done on powerup */
  /* Greet the BACnet world! */
  iam_send(&Handler_Transmit_Buffer[0]);
  /* Main loop */
  while (TRUE)
  {
    RESTART_WDT();
    Verify_Ints();
    dlmstp_task();
    MainTasks();
  }
}
