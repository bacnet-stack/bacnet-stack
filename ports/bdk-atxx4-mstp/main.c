/************************************************************************
 *
 * Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "hardware.h"
#include "init.h"
#include "stack.h"
#include "bacnet/basic/sys/mstimer.h"
#include "input.h"
#include "led.h"
#include "adc.h"
#include "nvdata.h"
#include "bacnet/basic/sys/mstimer.h"
#include "rs485.h"
#include "serial.h"
#include "bacnet.h"
#include "test.h"
#include "watchdog.h"
#include "bacnet/version.h"

/* global - currently the version of the stack */
char *BACnet_Version = BACNET_VERSION_TEXT;

/* For porting to IAR, see:
   http://www.avrfreaks.net/wiki/index.php/Documentation:AVR_GCC/IarToAvrgcc*/

int main(void)
{
    init();
    /* Configure the watchdog timer - Disabled for debugging */
#ifdef NDEBUG
    watchdog_init(2000);
#else
    watchdog_init(0);
#endif
    mstimer_init();
    adc_init();
    input_init();
    seeprom_init();
    rs485_init();
    serial_init();
    led_init();
    bacnet_init();
    test_init();
    /* Enable global interrupts */
    __enable_interrupt();
    for (;;) {
        watchdog_reset();
        input_task();
        bacnet_task();
        led_task();
        test_task();
    }
}
