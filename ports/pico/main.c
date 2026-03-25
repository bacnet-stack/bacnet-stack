/**************************************************************************
 *
 * Copyright (C) 2025 Testimony Adams <adamstestimony@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/

#include <stdint.h>

#include "pico/stdlib.h"

#include "dlenv.h"
#include "mstimer_init.h"

#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/npdu.h"

static uint8_t PDUBuffer[MAX_MPDU + 16];

int main(void)
{
    uint16_t pdu_len = 0;
    BACNET_ADDRESS src = { 0 };

    stdio_init_all();
    systimer_init();

    address_init();
    Device_Set_Object_Instance_Number(12345);
    Device_Init(NULL);

    if (!pico_dlenv_init(2)) {
        while (true) {
            tight_loop_contents();
        }
    }

    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, handler_write_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);

    Send_I_Am(&Handler_Transmit_Buffer[0]);

    for (;;) {
        pdu_len = datalink_receive(&src, &PDUBuffer[0], MAX_MPDU, 0);
        if (pdu_len) {
            npdu_handler(&src, &PDUBuffer[0], pdu_len);
        }
        tsm_timer_milliseconds(1);
        tight_loop_contents();
    }
}
