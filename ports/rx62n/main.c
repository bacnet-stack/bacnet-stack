/************************************************************************
 *
 * Copyright (C) 2011 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include "hardware.h"
#include "bacnet/datalink/ethernet.h"
#include "bacnet/basic/sys/mstimer.h"
#include "led.h"

/** Main function of BACnet demo for RX62N evaluation board */
int main(void)
{
    InitialiseLCD();
    ClearLCD();
    DisplayLCD(LCD_LINE1, "BACnet Demo");
    /* our stuff */
    timer_init();
    led_init();
    ethernet_init(NULL);
    bacnet_init();
    for (;;) {
        bacnet_task();
        led_task();
    }
}
