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
#include <string.h>
#include "hardware.h"
#include "init.h"
#include "stack.h"
#include "timer.h"
#include "input.h"
#include "led.h"
#include "adc.h"
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
#include "av.h"
#include "bi.h"
#include "bo.h"

/* local version override */
const char *BACnet_Version = "1.0";
/* MAC Address of MS/TP */
static uint8_t MSTP_MAC_Address;
/* timer for device communications control */
static struct itimer DCC_Timer;
#define DCC_CYCLE_SECONDS 1
/* timer for test task */
static struct itimer Test_Timer;

/* For porting to IAR, see:
   http://www.avrfreaks.net/wiki/index.php/Documentation:AVR_GCC/IarToAvrgcc*/

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

    /* set up our confirmed service unrecognized service handler - required! */
    apdu_set_unrecognized_service_handler_handler
        (handler_unrecognized_service);
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
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
    /* start the cyclic 1 second timer for DCC */
    timer_interval_start_seconds(&DCC_Timer, DCC_CYCLE_SECONDS);
    /* Hello World! */
    Send_I_Am(&Handler_Transmit_Buffer[0]);
}

static uint8_t PDUBuffer[MAX_MPDU];
static void bacnet_task(
    void)
{
    uint8_t mstp_mac_address;
    uint16_t pdu_len;
    BACNET_ADDRESS src; /* source address */
    uint8_t value;
    bool button_value;
    uint8_t i;
    BACNET_BINARY_PV binary_value = BINARY_INACTIVE;


    mstp_mac_address = input_address();
    if (MSTP_MAC_Address != mstp_mac_address) {
        /* address changed! */
        MSTP_MAC_Address = mstp_mac_address;
        dlmstp_set_mac_address(MSTP_MAC_Address);
        Send_I_Am(&Handler_Transmit_Buffer[0]);
    }
    /* handle the inputs */
    value = adc_result(7);
    Analog_Input_Present_Value_Set(0, value);
    for (i = 0; i < 5; i++) {
        button_value = input_button_value(i);
        if (button_value) {
            binary_value = BINARY_ACTIVE;
        }
        Binary_Input_Present_Value_Set(i, binary_value);
    }
    /* handle the communication timer */
    if (timer_interval_expired(&DCC_Timer)) {
        timer_interval_reset(&DCC_Timer);
        dcc_timer_seconds(DCC_CYCLE_SECONDS);
    }
    /* handle the messaging */
    pdu_len = datalink_receive(&src, &PDUBuffer[0], sizeof(PDUBuffer), 0);
    if (pdu_len) {
        npdu_handler(&src, &PDUBuffer[0], pdu_len);
    }
}

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

int main(
    void)
{
    init();
    adc_init();
    led_init();
    input_init();
    timer_init();
    seeprom_init();
    rs485_init();
    serial_init();
    bacnet_init();
    test_init();
    /* Enable global interrupts */
    __enable_interrupt();
    for (;;) {
        wdt_reset();
        input_task();
        bacnet_task();
        led_task();
        test_task();
    }
}
