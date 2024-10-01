/**
 * @file
 * @brief AtomicWriteFile-Request service handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2006
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacstr.h"
#include "bacnet/bacerror.h"
#include "bacnet/bacdcode.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/awf.h"
/* basic objects, services, TSM, and datalink */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/services.h"
#include "bacnet/datalink/datalink.h"
#if defined(BACFILE)
#include "bacnet/basic/object/bacfile.h"
#endif

/*
from BACnet SSPC-135-2004

14. FILE ACCESS SERVICES

This clause defines the set of services used to access and
manipulate files contained in BACnet devices. The concept of files
is used here as a network-visible representation for a collection
of octets of arbitrary length and meaning. This is an abstract
concept only and does not imply the use of disk, tape or other
mass storage devices in the server devices. These services may
be used to access vendor-defined files as well as specific
files defined in the BACnet protocol standard.
Every file that is accessible by File Access Services shall
have a corresponding File object in the BACnet device. This File
object is used to identify the particular file by name. In addition,
the File object provides access to "header information," such
as the file's total size, creation date, and type. File Access
Services may model files in two ways: as a continuous stream of
octets or as a contiguous sequence of numbered records.
The File Access Services provide atomic read and write operations.
In this context "atomic" means that during the execution
of a read or write operation, no other AtomicReadFile or
AtomicWriteFile operations are allowed for the same file.
Synchronization of these services with internal operations
of the BACnet device is a local matter and is not defined by this
standard.
*/

#if defined(BACFILE)
void handler_atomic_write_file(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    BACNET_ATOMIC_WRITE_FILE_DATA data;
    int len = 0;
    int pdu_len = 0;
    bool error = false;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;
    BACNET_ERROR_CLASS error_class = ERROR_CLASS_OBJECT;
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;

#if PRINT_ENABLED
    fprintf(stderr, "Received AtomicWriteFile Request!\n");
#endif
    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], src, &my_address, &npdu_data);
    if (service_data->segmented_message) {
        len = abort_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
#if PRINT_ENABLED
        fprintf(stderr, "Segmented Message. Sending Abort!\n");
#endif
        goto AWF_ABORT;
    }
    len = awf_decode_service_request(service_request, service_len, &data);
    /* bad decoding - send an abort */
    if (len < 0) {
        len = abort_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            ABORT_REASON_OTHER, true);
#if PRINT_ENABLED
        fprintf(stderr, "Bad Encoding. Sending Abort!\n");
#endif
        goto AWF_ABORT;
    }
    if (data.object_type == OBJECT_FILE) {
        if (!bacfile_valid_instance(data.object_instance)) {
            error = true;
        } else if (data.access == FILE_STREAM_ACCESS) {
            if (bacfile_write_stream_data(&data)) {
#if PRINT_ENABLED
                fprintf(
                    stderr, "AWF: Stream offset %d, %d bytes\n",
                    data.type.stream.fileStartPosition,
                    (int)octetstring_length(&data.fileData[0]));
#endif
                len = awf_ack_encode_apdu(
                    &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
                    &data);
            } else {
                error = true;
                error_class = ERROR_CLASS_OBJECT;
                error_code = ERROR_CODE_FILE_ACCESS_DENIED;
            }
        } else if (data.access == FILE_RECORD_ACCESS) {
            if (bacfile_write_record_data(&data)) {
#if PRINT_ENABLED
                fprintf(
                    stderr, "AWF: StartRecord %d, RecordCount %u\n",
                    data.type.record.fileStartRecord,
                    data.type.record.returnedRecordCount);
#endif
                len = awf_ack_encode_apdu(
                    &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
                    &data);
            } else {
                error = true;
                error_class = ERROR_CLASS_OBJECT;
                error_code = ERROR_CODE_FILE_ACCESS_DENIED;
            }
        } else {
            error = true;
            error_class = ERROR_CLASS_SERVICES;
            error_code = ERROR_CODE_INVALID_FILE_ACCESS_METHOD;
#if PRINT_ENABLED
            fprintf(stderr, "Record Access Requested. Sending Error!\n");
#endif
        }
    } else {
        error = true;
        error_class = ERROR_CLASS_SERVICES;
        error_code = ERROR_CODE_INCONSISTENT_OBJECT_TYPE;
    }
    if (error) {
        len = bacerror_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            SERVICE_CONFIRMED_ATOMIC_WRITE_FILE, error_class, error_code);
    }
AWF_ABORT:
    pdu_len += len;
    bytes_sent = datalink_send_pdu(
        src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
#if PRINT_ENABLED
    if (bytes_sent <= 0) {
        fprintf(stderr, "Failed to send PDU (%s)!\n", strerror(errno));
    }
#endif

    return;
}
#endif
