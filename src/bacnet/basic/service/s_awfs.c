/**
 * @file
 * @brief Send part of an Atomic Write File Stream request.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2006
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/dcc.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/awf.h"
/* basic services, TSM, binding, and datalink */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/services.h"

uint8_t Send_Atomic_Write_File_Stream(
    uint32_t device_id,
    uint32_t file_instance,
    int fileStartPosition,
    const BACNET_OCTET_STRING *fileData)
{
    BACNET_ADDRESS dest;
    BACNET_ADDRESS my_address;
    BACNET_NPDU_DATA npdu_data;
    unsigned max_apdu = 0;
    uint8_t invoke_id = 0;
    bool status = false;
    int len = 0;
    int pdu_len = 0;
    int bytes_sent = 0;
    BACNET_ATOMIC_WRITE_FILE_DATA data;

    /* if we are forbidden to send, don't send! */
    if (!dcc_communication_enabled()) {
        return 0;
    }
    /* is the device bound? */
    status = address_get_by_device(device_id, &max_apdu, &dest);
    /* is there a tsm available? */
    if (status) {
        invoke_id = tsm_next_free_invokeID();
    }
    if (invoke_id) {
        /* load the data for the encoding */
        data.object_type = OBJECT_FILE;
        data.object_instance = file_instance;
        data.access = FILE_STREAM_ACCESS;
        data.type.stream.fileStartPosition = fileStartPosition;
        status = octetstring_copy(&data.fileData[0], fileData);
        if (status) {
            /* encode the NPDU portion of the packet */
            datalink_get_my_address(&my_address);
            npdu_encode_npdu_data(&npdu_data, true, MESSAGE_PRIORITY_NORMAL);
            pdu_len = npdu_encode_pdu(
                &Handler_Transmit_Buffer[0], &dest, &my_address, &npdu_data);
            /* encode the APDU portion of the packet */
            len = awf_encode_apdu(
                &Handler_Transmit_Buffer[pdu_len], invoke_id, &data);
            pdu_len += len;
            /* will the APDU fit the target device?
               note: if there is a bottleneck router in between
               us and the destination, we won't know unless
               we have a way to check for that and update the
               max_apdu in the address binding table. */
            if ((unsigned)pdu_len <= max_apdu) {
                tsm_set_confirmed_unsegmented_transaction(
                    invoke_id, &dest, &npdu_data, &Handler_Transmit_Buffer[0],
                    (uint16_t)pdu_len);
                bytes_sent = datalink_send_pdu(
                    &dest, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
                if (bytes_sent <= 0) {
                    debug_perror("Failed to Send AtomicWriteFile Request");
                }
            } else {
                tsm_free_invoke_id(invoke_id);
                invoke_id = 0;
                debug_fprintf(
                    stderr,
                    "Failed to Send AtomicWriteFile Request "
                    "(payload [%d] exceeds destination maximum APDU [%u])!\n",
                    pdu_len, max_apdu);
            }
        } else {
            tsm_free_invoke_id(invoke_id);
            invoke_id = 0;
            debug_fprintf(
                stderr,
                "Failed to Send AtomicWriteFile Request "
                "(payload [%d] exceeds octet string capacity)!\n",
                pdu_len);
        }
    }

    return invoke_id;
}
