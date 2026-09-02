/**
 * @file
 * @brief Send a ConfirmedPrivateTransfer request with one or more values.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/dcc.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/ptransfer.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"

static uint8_t Private_Transfer_Buffer[MAX_APDU];

/**
 * @brief Send a confirmed private transfer request with one or more values.
 * @param pdu Output buffer for the encoded APDU.
 * @param max_pdu Maximum size of the output buffer in bytes.
 * @param device_id BACnet device instance to send the request to.
 * @param vendor_id Vendor identifier for the private transfer.
 * @param service_number Vendor-defined private service number.
 * @param value_list Linked list of application values to encode.
 * @return Invoke ID of the transaction, or 0 on failure.
 */
uint8_t Send_Private_Transfer_Request(
    uint8_t *pdu,
    size_t max_pdu,
    uint32_t device_id,
    uint16_t vendor_id,
    uint32_t service_number,
    const BACNET_APPLICATION_DATA_VALUE *value_list)
{
    BACNET_ADDRESS dest;
    BACNET_ADDRESS my_address;
    unsigned max_apdu = 0;
    uint8_t invoke_id = 0;
    bool status = false;
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_PRIVATE_TRANSFER_DATA private_data;

    if (!dcc_communication_enabled()) {
        return 0;
    }
    status = address_get_by_device(device_id, &max_apdu, &dest);
    if (status) {
        invoke_id = tsm_next_free_invokeID();
    }
    if (!invoke_id) {
        return 0;
    }
    /* encode the value list to the internal buffer */
    private_data.vendorID = vendor_id;
    private_data.serviceNumber = service_number;
    len = bacapp_encode_data_list(NULL, value_list);
    if (len > sizeof(Private_Transfer_Buffer)) {
        tsm_free_invoke_id(invoke_id);
        debug_fprintf(
            stderr,
            "Failed to Send ConfirmedPrivateTransfer Request "
            "(exceeds internal buffer size)!\n");
        return 0;
    }
    private_data.serviceParameters = Private_Transfer_Buffer;
    private_data.serviceParametersLen =
        bacapp_encode_data_list(private_data.serviceParameters, value_list);
    /* construct the NPDU */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, true, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(&pdu[0], &dest, &my_address, &npdu_data);
    /* append the APDU */
    len = ptransfer_service_header_encode(
        &pdu[pdu_len], max_pdu - pdu_len, invoke_id);
    if (len <= 0) {
        tsm_free_invoke_id(invoke_id);
        return 0;
    }
    pdu_len += len;
    len = private_transfer_request_service_encode(
        &pdu[pdu_len], max_pdu - pdu_len, &private_data);
    if (len <= 0) {
        tsm_free_invoke_id(invoke_id);
        return 0;
    }
    pdu_len += len;
    tsm_set_confirmed_unsegmented_transaction(
        invoke_id, &dest, &npdu_data, &pdu[0], (uint16_t)pdu_len);
    bytes_sent = datalink_send_pdu(&dest, &npdu_data, &pdu[0], pdu_len);
    if (bytes_sent <= 0) {
        tsm_free_invoke_id(invoke_id);
        debug_perror("Failed to Send PrivateTransfer Request");
        return 0;
    }

    return invoke_id;
}
