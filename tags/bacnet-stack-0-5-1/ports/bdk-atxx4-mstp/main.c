/************************************************************************
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
*************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "hardware.h"
#include "init.h"
#include "stack.h"
#include "timer.h"
#include "input.h"
#include "led.h"
#include "nvdata.h"
#include "timer.h"
#include "dcc.h"
#include "rs485.h"
#include "serial.h"
#include "datalink.h"
#include "npdu.h"
#include "handlers.h"
#include "client.h"
#include "txbuf.h"
#include "iam.h"
#include "device.h"
#include "ai.h"
#include "bi.h"
#include "bo.h"

/* local version override */
const char *BACnet_Version = "1.0";
/* MAC Address of MS/TP */
static uint8_t MSTP_MAC_Address;

/* For porting to IAR, see:
   http://www.avrfreaks.net/wiki/index.php/Documentation:AVR_GCC/IarToAvrgcc*/

#if defined(__GNUC__) && (__GNUC__ > 4)
/* AVR fuse settings */
FUSES = {
    /* External Ceramic Resonator - configuration */
    /* Full Swing Crystal Oscillator Clock Selection */
    /* Ceramic resonator, slowly rising power 1K CK 14CK + 65 ms */
    /* note: fuses are enabled by clearing the bit, so
       any fuses listed below are cleared fuses. */
    .low = (FUSE_CKSEL3 & FUSE_SUT0 & FUSE_SUT1),
        /* BOOTSZ configuration:
           BOOTSZ1 BOOTSZ0 Boot Size
           ------- ------- ---------
           1       1      512
           1       0     1024
           0       1     2048
           0       0     4096
         */
        /* note: fuses are enabled by clearing the bit, so
           any fuses listed below are cleared fuses. */
        .high =
        (FUSE_BOOTSZ1 & FUSE_BOOTRST & FUSE_EESAVE & FUSE_SPIEN & FUSE_JTAGEN),
        /* Brown-out detection VCC=2.7V */
        /* BODLEVEL configuration 
           BODLEVEL2 BODLEVEL1 BODLEVEL0 Voltage
           --------- --------- --------- --------
           1         1         1     disabled
           1         1         0       1.8V
           1         0         1       2.7V
           1         0         0       4.3V
         */
        /* note: fuses are enabled by clearing the bit, so
           any fuses listed below are cleared fuses. */
.extended = (FUSE_BODLEVEL1 & FUSE_BODLEVEL0),};

/* AVR lock bits - unlocked */
LOCKBITS = LOCKBITS_DEFAULT;
#endif

bool seeprom_version_test(
    void)
{
    uint16_t version = 0;
    uint16_t id = 0;
    bool status = false;

    seeprom_bytes_read(NV_SEEPROM_TYPE_0, (uint8_t *) & id, 2);
    seeprom_bytes_read(NV_SEEPROM_VERSION_0, (uint8_t *) & version, 2);

    if ((id == SEEPROM_ID) && (version == SEEPROM_VERSION)) {
        status = true;
    } else {
        version = SEEPROM_VERSION;
        id = SEEPROM_ID;
        seeprom_bytes_write(NV_SEEPROM_TYPE_0, (uint8_t *) & id, 2);
        seeprom_bytes_write(NV_SEEPROM_VERSION_0, (uint8_t *) & version, 2);
    }

    return status;
}

static void bacnet_init(
    void)
{
    MSTP_MAC_Address = input_address();
    dlmstp_set_mac_address(MSTP_MAC_Address);
    dlmstp_init(NULL);

    if (!seeprom_version_test()) {
        /* invalid version data */
    }
    /* initialize objects */
    Device_Init();
    Binary_Output_Init();

    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    /* Set the handlers for any confirmed services that we support. */
    /* We must implement read property - it's required! */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROP_MULTIPLE,
        handler_read_property_multiple);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_REINITIALIZE_DEVICE,
        handler_reinitialize_device);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROPERTY,
        handler_write_property);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);

    Send_I_Am(&Handler_Transmit_Buffer[0]);
}

static uint8_t PDUBuffer[MAX_MPDU];
static void bacnet_task(
    void)
{
    uint8_t mstp_mac_address = 0;
    uint16_t pdu_len = 0;
    BACNET_ADDRESS src; /* source address */

    mstp_mac_address = input_address();
    if (MSTP_MAC_Address != mstp_mac_address) {
        MSTP_MAC_Address = mstp_mac_address;
        dlmstp_set_mac_address(MSTP_MAC_Address);
        Send_I_Am(&Handler_Transmit_Buffer[0]);
    }
    if (timer_elapsed_seconds(TIMER_DCC, 1)) {
        dcc_timer_seconds(1);
    }
    pdu_len = datalink_receive(&src, &PDUBuffer[0], sizeof(PDUBuffer), 0);
    if (pdu_len) {
        npdu_handler(&src, &PDUBuffer[0], pdu_len);
    }
}

void idle_init(
    void)
{
    timer_reset(TIMER_LED_3);
    timer_reset(TIMER_LED_4);
}

void idle_task(
    void)
{
#if 0
    /* blink the leds */
    if (timer_elapsed_seconds(TIMER_LED_3, 1)) {
        timer_reset(TIMER_LED_3);
        led_toggle(LED_3);
    }
    if (timer_elapsed_milliseconds(TIMER_LED_4, 125)) {
        timer_reset(TIMER_LED_4);
        led_toggle(LED_4);
    }
#endif
}

int main(
    void)
{
    init();
    led_init();
    input_init();
    timer_init();
    seeprom_init();
    rs485_init();
    serial_init();
    bacnet_init();
    idle_init();
    /* Enable global interrupts */
    __enable_interrupt();
    for (;;) {
        input_task();
        bacnet_task();
        led_task();
        idle_task();
    }
}
