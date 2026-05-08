/**
 * @file
 * @brief Handles AtomicReadFile-Request service.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2006
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/bacdcode.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/reject.h"
#include "bacnet/arf.h"
/* basic objects, services, TSM, and datalink */
#include "bacnet/basic/object/bacfile.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/datalink.h"

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

14.1 AtomicReadFile Service

14.1.5 Service Procedure

The responding BACnet-user shall first verify the validity
of the 'File Identifier' parameter and return a 'Result(-)' response
with the appropriate error class and code if the File object
is unknown, if there is currently another AtomicReadFile or
AtomicWriteFile service in progress, or if the File object is
currently inaccessible for another reason. If the 'File Start
Position' parameter or the 'File Start Record' parameter is
either less than 0 or exceeds the actual file size, then the appropriate
error is returned in a 'Result(-)' response. If not, then the
responding BACnet-user shall read the number of octets specified by
'Requested Octet Count' or the number of records specified by
'Requested Record Count'. If the number of remaining octets or
records is less than the requested amount, then the length of
the 'File Data' returned or 'Returned Record Count' shall indicate
the actual number read. If the returned response contains the
last octet or record of the file, then the 'End Of File' parameter
shall be TRUE, otherwise FALSE.
*/

/**
 * @brief Encode an AtomicReadFile ACK or Error response based on the
 *       provided request and service data.
 * @param service_request The APDU portion of the request, starting with the
 * service choice.
 * @param service_len The length of the service_request buffer.
 * @param apdu The buffer to encode the response APDU into.
 * @param apdu_size The size of the apdu buffer.
 * @param src The source address to send the response to.
 * @param npdu_data The NPDU data to use for encoding the response.
 * @param service_data The confirmed service data from the request, used for
 * encoding the response.
 * @return The number of bytes encoded into the apdu buffer, or a negative value
 * on encoding error.
 */
int handler_atomic_read_file_encode(
    uint8_t *apdu,
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_NPDU_DATA *npdu_data,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    BACNET_ATOMIC_READ_FILE_DATA data = { 0 };
    int len = 0;
    int pdu_len = 0;
    bool error = false;
    BACNET_ADDRESS my_address;
    BACNET_ERROR_CODE error_code = ERROR_CODE_UNKNOWN_OBJECT;
    uint8_t *apdu_start;

    DEBUG_PRINTF("Received Atomic-Read-File Request!\n");
    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(npdu_data, false, service_data->priority);
    len = npdu_encode_pdu(apdu, src, &my_address, npdu_data);
    pdu_len += len;
    if (apdu) {
        apdu += len;
    }
    /* APDU starts after the NPDU portion */
    apdu_start = apdu;
    if (service_len == 0) {
        error_code = ERROR_CODE_REJECT_MISSING_REQUIRED_PARAMETER;
        error = true;
        DEBUG_PRINTF("ARF: Missing Required Parameter. Sending Reject!\n");
    } else if (service_data->segmented_message) {
        error_code = ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
        error = true;
    }
    if (!error) {
        len = arf_decode_service_request(service_request, service_len, &data);
    }
    /* bad decoding - send an abort */
    if (!error) {
        if (len < 0) {
            DEBUG_PRINTF("ARF: Bad Encoding. Sending Abort!\n");
            error_code = ERROR_CODE_ABORT_OTHER;
            error = true;
        } else if (data.object_type == OBJECT_FILE) {
            if (!bacfile_valid_instance(data.object_instance)) {
                error = true;
            } else if (data.access == FILE_STREAM_ACCESS) {
                if (data.type.stream.requestedOctetCount <=
                    octetstring_capacity(&data.fileData[0])) {
                    bacfile_read_stream_data(&data);
                    DEBUG_PRINTF(
                        "ARF: Stream offset %d, %d octets.\n",
                        (int)data.type.stream.fileStartPosition,
                        (int)data.type.stream.requestedOctetCount);
                    len = arf_ack_encode_apdu(
                        apdu, service_data->invoke_id, &data);
                    pdu_len += len;
                } else {
                    error_code = ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                    error = true;
                    DEBUG_PRINTF(
                        "ARF: Too Big To Send (%d >= %d). "
                        "Sending Abort!\n",
                        (int)data.type.stream.requestedOctetCount,
                        (int)octetstring_capacity(&data.fileData[0]));
                }
            } else if (data.access == FILE_RECORD_ACCESS) {
                if (data.type.record.RecordCount > ARRAY_SIZE(data.fileData)) {
                    DEBUG_PRINTF(
                        "ARF: RecordCount %u > %u. Sending Reject!\n",
                        (unsigned)data.type.record.RecordCount,
                        (unsigned)ARRAY_SIZE(data.fileData));
                    error_code = ERROR_CODE_REJECT_PARAMETER_OUT_OF_RANGE;
                    error = true;
                } else if (
                    data.type.record.fileStartRecord >=
                    ARRAY_SIZE(data.fileData)) {
                    DEBUG_PRINTF(
                        "ARF: fileStartRecord %d >= %u. Sending Error!\n",
                        (int)data.type.record.fileStartRecord,
                        (unsigned)ARRAY_SIZE(data.fileData));
                    error_code = ERROR_CODE_INVALID_FILE_START_POSITION;
                    error = true;
                } else if (bacfile_read_record_data(&data)) {
                    DEBUG_PRINTF(
                        "ARF: fileStartRecord %d, %u RecordCount.\n",
                        (int)data.type.record.fileStartRecord,
                        (unsigned)data.type.record.RecordCount);
                    len = arf_ack_encode_apdu(
                        apdu, service_data->invoke_id, &data);
                    pdu_len += len;
                } else {
                    DEBUG_PRINTF("ARF: file_access_denied! Sending Error!");
                    error = true;
                    error_code = ERROR_CODE_FILE_ACCESS_DENIED;
                }
            } else {
                DEBUG_PRINTF(
                    "ARF: Invalid File Access Method. Sending Error!\n");
                error = true;
                error_code = ERROR_CODE_INVALID_FILE_ACCESS_METHOD;
            }
        } else {
            DEBUG_PRINTF("ARF: Inconsistent Object Type. Sending Error!\n");
            error = true;
            error_code = ERROR_CODE_INCONSISTENT_OBJECT_TYPE;
        }
    }
    if (error) {
        len = bacnet_error_encode_apdu(
            apdu_start, service_data->invoke_id,
            SERVICE_CONFIRMED_ATOMIC_READ_FILE, error_code);
        pdu_len += len;
    }

    return pdu_len;
}

/**
 * @brief Handler for the AtomicReadFile service. Encodes and sends an ACK or
 * Error response based on the provided request and service data.
 * @param service_request The APDU portion of the request, starting with the
 * service choice.
 * @param service_len The length of the service_request buffer.
 * @param src The source address to send the response to.
 * @param service_data The confirmed service data from the request, used for
 * encoding the response.
 */
void handler_atomic_read_file(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    int pdu_len = 0, bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data = { 0 };

    pdu_len = handler_atomic_read_file_encode(
        NULL, service_request, service_len, src, &npdu_data, service_data);
    if (pdu_len > sizeof(Handler_Transmit_Buffer)) {
        debug_perror("ARF: Encoded PDU length exceeds transmit buffer size");
    } else {
        pdu_len = handler_atomic_read_file_encode(
            Handler_Transmit_Buffer, service_request, service_len, src,
            &npdu_data, service_data);
        bytes_sent = datalink_send_pdu(
            src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
        if (bytes_sent <= 0) {
            debug_perror("ARF: Failed to send PDU");
        }
    }
}
