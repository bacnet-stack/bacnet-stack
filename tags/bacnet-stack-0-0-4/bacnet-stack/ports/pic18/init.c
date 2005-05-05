/**************************************************************************
*
* Copyright (C) 2003 Mark Norton
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
* Functional
* Description:  Handles the init code for the Microchip microprocessor
*
*********************************************************************/
#include <string.h>
#include <stdint.h>
#include "hardware.h"

// define this to enable ICD - debugger
//#define USE_ICD
// ------------------------- Configuration Bits ----------------------------
#pragma romdata CONFIG
#ifdef USE_ICD
_CONFIG_DECL (
  _CONFIG1H_DEFAULT & _OSCS_OFF_1H & _OSC_HS_1H,
  _CONFIG2L_DEFAULT & _BOR_ON_2L & _BORV_27_2L & _PWRT_ON_2L,
  _CONFIG2H_DEFAULT & _WDT_OFF_2H & _WDTPS_128_2H,
  _CONFIG3H_DEFAULT & _CCP2MUX_OFF_3H,
  _CONFIG4L_DEFAULT & _STVR_ON_4L & _LVP_OFF_4L & _DEBUG_ON_4L,
  _CONFIG5L_DEFAULT & _CP0_OFF_5L & _CP1_OFF_5L & _CP2_OFF_5L & _CP3_OFF_5L ,
  _CONFIG5H_DEFAULT & _CPB_OFF_5H & _CPD_OFF_5H,
  _CONFIG6L_DEFAULT & _WRT0_OFF_6L & _WRT1_OFF_6L & _WRT2_OFF_6L & _WRT3_OFF_6L ,
  _CONFIG6H_DEFAULT & _WPC_OFF_6H & _WPB_OFF_6H & _WPD_OFF_6H,
  _CONFIG7L_DEFAULT & _EBTR0_OFF_7L & _EBTR1_OFF_7L & _EBTR2_OFF_7L & _EBTR3_OFF_7L,
  _CONFIG7H_DEFAULT & _EBTRB_OFF_7H);
#else
_CONFIG_DECL (
  _CONFIG1H_DEFAULT & _OSCS_OFF_1H & _OSC_HS_1H,
  _CONFIG2L_DEFAULT & _BOR_ON_2L & _BORV_27_2L & _PWRT_ON_2L,
  _CONFIG2H_DEFAULT & _WDT_ON_2H & _WDTPS_128_2H,
  _CONFIG3H_DEFAULT & _CCP2MUX_OFF_3H,
  _CONFIG4L_DEFAULT & _STVR_ON_4L & _LVP_OFF_4L & _DEBUG_OFF_4L,
  _CONFIG5L_DEFAULT & _CP0_OFF_5L & _CP1_OFF_5L & _CP2_OFF_5L & _CP3_OFF_5L ,
  _CONFIG5H_DEFAULT & _CPB_OFF_5H & _CPD_OFF_5H,
  _CONFIG6L_DEFAULT & _WRT0_OFF_6L & _WRT1_OFF_6L & _WRT2_OFF_6L & _WRT3_OFF_6L ,
  _CONFIG6H_DEFAULT & _WPC_OFF_6H & _WPB_OFF_6H & _WPD_OFF_6H,
  _CONFIG7L_DEFAULT & _EBTR0_OFF_7L & _EBTR1_OFF_7L & _EBTR2_OFF_7L & _EBTR3_OFF_7L,
  _CONFIG7H_DEFAULT & _EBTRB_OFF_7H);

#endif // USE_ICD
#pragma romdata

/****************************************************************************
* DESCRIPTION: Initializes the PIC, its timers, WDT, etc.
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
void init_hardware(void)
{
  // If the processor gets a power on reset then we can do something.
  // We should not get a reset unless there has been 
  // some kind of power line disturbance.
  if (RCONbits.POR)
  {
    //do something special!
  }

  GLOBAL_INT_DISABLE();

  /* Setup PORT A */
  TRISA=PORT_A_TRIS_MASK;

  /* PORT A can have analog inputs or digital IO */
  ADCON1 = NO_ANALOGS;

  /* Setup PORT B */
  TRISB=PORT_B_TRIS_MASK;

  /* Setup PORT C */
  TRISC=PORT_C_TRIS_MASK;

  /* setup zero cross interrupt to trigger on a low to high edge */
  INTCON2bits.INTEDG0 = 1;

  // setup ABUS
  //ABUS_LED_OFF();
  //SSPADD = ABUS_DEFAULT_ADDR;
  //SSPCON1 = (ABUS_SLAVE_MASK | ABUS_CLK_ENABLE | ABUS_MODE_SETUP);
  //ABUS_Clear_SSPBUF();

  // setup timer2 to reset every 1ms
  CloseTimer2();
  PR2 = 250;
  OpenTimer2(T2_PS_1_4 & T2_POST_1_5 & 0x7F);

  // Setup our interrupt priorities ---------> all low priority
  RCONbits.IPEN = 1;
  IPR1 = 0;
  IPR2 = 0;
  INTCON2bits.TMR0IP = 0;
  INTCON2bits.RBIP = 0;
  INTCON3 = 0;

  /* Enable interrupts */
  TIMER2_INT_ENABLE();
  PERIPHERAL_INT_ENABLE();
  GLOBAL_INT_ENABLE();

// Turn on the Zero cross interrupt
  INTCONbits.INT0F = 0;
  INTCONbits.INT0E = 1;
}