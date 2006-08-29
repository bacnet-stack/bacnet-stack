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
#include "datalink.h"
#include "handlers.h"
#include "device.h"
#include "hardware.h"
#include "iam.h"
/* for readproperty handler */
#include "config.h"
#include "txbuf.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "bacerror.h"
#include "apdu.h"
#include "npdu.h"
#include "abort.h"
#include "rp.h"

/* buffer used for encoding RP apdu */
static uint8_t Temp_Buf[MAX_APDU];
/* buffer used for receiving */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* address where message came from */
static BACNET_ADDRESS src;
/* address used to send */
static BACNET_ADDRESS my_address;

/* see demo/handler/h_rp.c for a more complete example */
void My_Read_Property_Handler(uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src, BACNET_CONFIRMED_SERVICE_DATA * service_data)
{
    BACNET_READ_PROPERTY_DATA data;
    int len = 0;
    int pdu_len = 0;
    bool send = false;
    bool error = false;
    int bytes_sent = 0;
    BACNET_ERROR_CLASS error_class = ERROR_CLASS_OBJECT;
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    BACNET_NPDU_DATA npdu_data;

    len = rp_decode_service_request(service_request, service_len, &data);
    /* bad decoding - send an abort */
    if (len < 0) {
        pdu_len = abort_encode_apdu(&Handler_Transmit_Buffer[0],
            service_data->invoke_id, ABORT_REASON_OTHER);
    } else if (service_data->segmented_message) {
        pdu_len = abort_encode_apdu(&Handler_Transmit_Buffer[0],
            service_data->invoke_id,
            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED);
    } else {
        switch (data.object_type) {
        case OBJECT_DEVICE:
            /* FIXME: probably need a length limitation sent with encode */
            if (data.object_instance == Device_Object_Instance_Number()) {
                len = Device_Encode_Property_APDU(&Temp_Buf[0],
                    data.object_property,
                    data.array_index, &error_class, &error_code);
                if (len >= 0) {
                    /* encode the APDU portion of the packet */
                    data.application_data = &Temp_Buf[0];
                    data.application_data_len = len;
                    /* FIXME: probably need a length limitation sent with encode */
                    pdu_len =
                        rp_ack_encode_apdu(&Handler_Transmit_Buffer[0],
                        service_data->invoke_id, &data);
                } else
                    error = true;
            } else
                error = true;
            break;
        default:
            error = true;
            break;
        }
    }
    if (error) {
        pdu_len = bacerror_encode_apdu(&Handler_Transmit_Buffer[0],
            service_data->invoke_id,
            SERVICE_CONFIRMED_READ_PROPERTY, error_class, error_code);
    }
    npdu_encode_confirmed_apdu(&npdu_data, MESSAGE_PRIORITY_NORMAL);
    bytes_sent = datalink_send_pdu(src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);      /* number of bytes of data */

    return;
}


/****************************************************************************
* DESCRIPTION: Handles our calling our module level milisecond counters
* PARAMETERS:  none
* RETURN:      none
* ALGORITHM:   none
* NOTES:       none
*****************************************************************************/
static void Check_Timer_Milliseconds(void)
{
    /* We might have missed some so keep doing it until we have got them all */
    while (Milliseconds) {
        dlmstp_millisecond_timer();
        Milliseconds--;
    }
}

void main(void)
{
    unsigned timeout = 100;     /* milliseconds */
    uint16_t pdu_len = 0;


    /* we need to handle who-is 
       to support dynamic device binding to us */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS,
        handler_who_is);
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler
        (handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler
        (SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);

    Device_Set_Object_Instance_Number(5);
    dlmstp_set_my_address(0x05);
    dlmstp_init();

    init_hardware();

    /* broadcast an I-Am on startup */
    iam_send(&Handler_Transmit_Buffer[0]);
    /* loop forever */
    for (;;) {
        WATCHDOG_TIMER();

        /* input */
        Check_Timer_Milliseconds();
        dlmstp_task();
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);

        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }

        /* output */

    }

    return;
}
