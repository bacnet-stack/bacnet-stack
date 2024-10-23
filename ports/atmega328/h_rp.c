/**
 * @brief This module manages the BACnet ReadProperty handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "bacnet/config.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/services.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/rp.h"
/* demo objects */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/av.h"
#include "bacnet/basic/object/bv.h"

/* Encodes the property APDU and returns the length,
   or sets the error, and returns -1 */
static int Encode_Property_APDU(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = -1;

    /* handle each object type */
    switch (rpdata->object_type) {
        case OBJECT_DEVICE:
            /* Test for case of indefinite Device object instance */
            if (rpdata->object_instance == BACNET_MAX_INSTANCE) {
                rpdata->object_instance = Device_Object_Instance_Number();
            }
            if (Device_Valid_Object_Instance_Number(rpdata->object_instance)) {
                apdu_len = Device_Read_Property(rpdata);
            }
            break;
        case OBJECT_ANALOG_VALUE:
            if (Analog_Value_Valid_Instance(rpdata->object_instance)) {
                apdu_len = Analog_Value_Read_Property(rpdata);
            }
            break;
        case OBJECT_BINARY_VALUE:
            if (Binary_Value_Valid_Instance(rpdata->object_instance)) {
                apdu_len = Binary_Value_Read_Property(rpdata);
            }
            break;
        default:
            rpdata->error_class = ERROR_CLASS_OBJECT;
            rpdata->error_code = ERROR_CODE_UNKNOWN_OBJECT;
            break;
    }

    return apdu_len;
}

void handler_read_property(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    BACNET_READ_PROPERTY_DATA data;
    int len = 0;
    int ack_len = 0;
    int property_len = 0;
    int pdu_len = 0;
    int ack_end_len = 0;
    BACNET_NPDU_DATA npdu_data;
    int bytes_sent = 0;
    BACNET_ADDRESS my_address;

    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], src, &my_address, &npdu_data);
    if (service_data->segmented_message) {
        /* we don't support segmentation - send an abort */
        len = abort_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
        goto RP_ABORT;
    }
    len = rp_decode_service_request(service_request, service_len, &data);
    if (len < 0) {
        /* bad decoding - send an abort */
        len = abort_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            ABORT_REASON_OTHER, true);
        goto RP_ABORT;
    }
    /* default case will be error */
    ack_len = rp_ack_encode_apdu_init(
        &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id, &data);
    ack_end_len = rp_ack_encode_apdu_object_property_end(NULL);
    data.application_data = &Handler_Transmit_Buffer[pdu_len + ack_len];
    data.application_data_len =
        sizeof(Handler_Transmit_Buffer) - (pdu_len + ack_len + ack_end_len);
    data.error_class = ERROR_CLASS_OBJECT;
    data.error_code = ERROR_CODE_UNKNOWN_OBJECT;
    property_len = Encode_Property_APDU(&data);
    if (property_len >= 0) {
        len = rp_ack_encode_apdu_object_property_end(
            &Handler_Transmit_Buffer[pdu_len + property_len + ack_len]);
        len += ack_len + property_len;
    } else if (property_len == BACNET_STATUS_ABORT) {
        /* BACnet APDU too small to fit data,
           so proper response is Abort */
        len = abort_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
    } else {
        len = bacerror_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            SERVICE_CONFIRMED_READ_PROPERTY, data.error_class, data.error_code);
    }
RP_ABORT:
    pdu_len += len;
    bytes_sent = datalink_send_pdu(
        src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    (void)bytes_sent;

    return;
}
