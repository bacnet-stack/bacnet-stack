/**
 * @file
 * @brief Handler for a BACnet ConfirmedPrivateTransfer-Request example
 * @author Peter Mc Shane <petermcs@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/reject.h"
#include "bacnet/ptransfer.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/tsm/tsm.h"

#define MYMAXSTR 32
#define MYMAXBLOCK 8

DATABLOCK MyData[MYMAXBLOCK];

uint8_t IOBufferPT[MAX_APDU]; /* Buffer for building response in  */

static void ProcessPT(BACNET_PRIVATE_TRANSFER_DATA *data)
{
    int iLen; /* Index to current location in data */
    char cBlockNumber;
    uint32_t ulTemp;
    int tag_len;
    uint8_t tag_number;
    uint32_t len_value_type;
    BACNET_CHARACTER_STRING bsTemp;

    iLen = 0;

    /* Decode the block number */
    tag_len = decode_tag_number_and_value(
        &data->serviceParameters[iLen], &tag_number, &len_value_type);
    iLen += tag_len;
    if (tag_number != BACNET_APPLICATION_TAG_UNSIGNED_INT) {
        /* Bail out early if wrong type */
        /* and signal unexpected error */
        data->serviceParametersLen = 0;
        return;
    }

    iLen += decode_unsigned(
        &data->serviceParameters[iLen], len_value_type, &ulTemp);
    cBlockNumber = (char)ulTemp;
    if (cBlockNumber < MY_MAX_BLOCK) {
        if (data->serviceNumber == MY_SVC_READ) {
            /*  Read Response is an unsigned int with
               0 for success or a non 0 error code
               For a successful read the 0 success
               code is followed by the block number
               and then the block contents which
               consist of 2 unsigned ints (in 0 to 255
               range as they are really chars) a single
               precision real and a string which
               will be up to 32 chars + a nul */

            iLen = 0;

            /* Signal success */
            iLen += encode_application_unsigned(&IOBufferPT[iLen], MY_ERR_OK);
            /* Followed by the block number */
            iLen +=
                encode_application_unsigned(&IOBufferPT[iLen], cBlockNumber);
            /* And Then the block contents */
            iLen += encode_application_unsigned(
                &IOBufferPT[iLen], MyData[(int8_t)cBlockNumber].cMyByte1);
            iLen += encode_application_unsigned(
                &IOBufferPT[iLen], MyData[(int8_t)cBlockNumber].cMyByte2);
            iLen += encode_application_real(
                &IOBufferPT[iLen], MyData[(int8_t)cBlockNumber].fMyReal);
            characterstring_init_ansi(
                &bsTemp, (char *)MyData[(int8_t)cBlockNumber].sMyString);
            iLen +=
                encode_application_character_string(&IOBufferPT[iLen], &bsTemp);
        } else {
            /* Write operation */
            /*  Write block consists of the block number
               followed by the block contents as
               described above for the read operation.
               The returned result is an unsigned
               response which is 0 for success and
               a non 0 error code otherwise. */

            tag_len = decode_tag_number_and_value(
                &data->serviceParameters[iLen], &tag_number, &len_value_type);
            iLen += tag_len;
            if (tag_number != BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                data->serviceParametersLen = 0;
                return;
            }
            iLen += decode_unsigned(
                &data->serviceParameters[iLen], len_value_type, &ulTemp);
            MyData[(int8_t)cBlockNumber].cMyByte1 = (char)ulTemp;

            tag_len = decode_tag_number_and_value(
                &data->serviceParameters[iLen], &tag_number, &len_value_type);
            iLen += tag_len;
            if (tag_number != BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                data->serviceParametersLen = 0;
                return;
            }
            iLen += decode_unsigned(
                &data->serviceParameters[iLen], len_value_type, &ulTemp);
            MyData[(int8_t)cBlockNumber].cMyByte2 = (char)ulTemp;

            tag_len = decode_tag_number_and_value(
                &data->serviceParameters[iLen], &tag_number, &len_value_type);
            iLen += tag_len;
            if (tag_number != BACNET_APPLICATION_TAG_REAL) {
                data->serviceParametersLen = 0;
                return;
            }
            iLen += decode_real(
                &data->serviceParameters[iLen],
                &MyData[(int8_t)cBlockNumber].fMyReal);

            tag_len = decode_tag_number_and_value(
                &data->serviceParameters[iLen], &tag_number, &len_value_type);
            iLen += tag_len;
            if (tag_number != BACNET_APPLICATION_TAG_CHARACTER_STRING) {
                data->serviceParametersLen = 0;
                return;
            }
            decode_character_string(
                &data->serviceParameters[iLen], len_value_type, &bsTemp);
            /* Only copy as much as we can accept */
            snprintf(
                (char *)MyData[(int8_t)cBlockNumber].sMyString, MY_MAX_STR,
                "%s", characterstring_value(&bsTemp));
            /* Make sure it is nul terminated */
            MyData[(int8_t)cBlockNumber].sMyString[MY_MAX_STR] = '\0';
            /* Signal success */
            iLen = encode_application_unsigned(&IOBufferPT[0], MY_ERR_OK);
        }
    } else {
        /* Signal bad index */
        iLen = encode_application_unsigned(&IOBufferPT[0], MY_ERR_BAD_INDEX);
    }
    data->serviceParametersLen = iLen;
    data->serviceParameters = IOBufferPT;
}

/*
 * This is called when we receive a private transfer packet.
 * We parse the data, send the private part for processing and then send the
 * response which the application generates.
 * If there are any BACnet level errors we send an error response from here.
 * If there are any application level errors they will be packeged up in the
 * response block which we send back to the originator of the request.
 *
 */

void handler_conf_private_trans(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_DATA *service_data)
{
    BACNET_PRIVATE_TRANSFER_DATA data;
    int len;
    int pdu_len;
    bool error;
    int bytes_sent;
    BACNET_NPDU_DATA npdu_data;
    BACNET_ADDRESS my_address;
    BACNET_ERROR_CLASS error_class;
    BACNET_ERROR_CODE error_code;

    len = 0;
    pdu_len = 0;
    error = false;
    bytes_sent = 0;
    error_class = ERROR_CLASS_OBJECT;
    error_code = ERROR_CODE_UNKNOWN_OBJECT;

    debug_print("Received Confirmed Private Transfer Request!\n");
    /* encode the NPDU portion of the response packet as it will be needed */
    /* no matter what the outcome. */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, service_data->priority);
    pdu_len = npdu_encode_pdu(
        &Handler_Transmit_Buffer[0], src, &my_address, &npdu_data);
    if (service_len == 0) {
        len = reject_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            REJECT_REASON_MISSING_REQUIRED_PARAMETER);
        debug_print("CPT: Missing Required Parameter. Sending Reject!\n");
        goto CPT_ABORT;
    } else if (service_data->segmented_message) {
        len = abort_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
        debug_print("CPT: Segmented Message. Sending Abort!\n");
        goto CPT_ABORT;
    }

    len = ptransfer_decode_service_request(service_request, service_len, &data);
    /* bad decoding - send an abort */
    if (len < 0) {
        len = abort_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            ABORT_REASON_OTHER, true);
        debug_print("CPT: Bad Encoding. Sending Abort!\n");
        goto CPT_ABORT;
    }

    /*  Simple example with service number of 0 for
       read block and 1 for write block
       We also only support our own vendor ID.
       In theory we could support others
       for compatability purposes but these
       interfaces are rarely documented... */
    if ((data.vendorID == BACNET_VENDOR_ID) &&
        (data.serviceNumber <= MY_SVC_WRITE)) {
        /* We only try to understand our own IDs and service numbers */
        /* Will either return a result block or an app level status block */
        ProcessPT(&data);
        if (data.serviceParametersLen == 0) {
            /* No respopnse means fatal error */
            error = true;
            error_class = ERROR_CLASS_SERVICES;
            error_code = ERROR_CODE_OTHER;
            debug_print("CPT: Error servicing request!\n");
        }
        len = ptransfer_ack_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id, &data);
    } else { /* Not our vendor ID or bad service parameter */

        error = true;
        error_class = ERROR_CLASS_SERVICES;
        error_code = ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED;
        debug_print("CPT: Not our Vendor ID or invalid service code!\n");
    }

    if (error) {
        len = ptransfer_error_encode_apdu(
            &Handler_Transmit_Buffer[pdu_len], service_data->invoke_id,
            error_class, error_code, &data);
    }
CPT_ABORT:
    pdu_len += len;
    bytes_sent = datalink_send_pdu(
        src, &npdu_data, &Handler_Transmit_Buffer[0], pdu_len);
    debug_perror("CPT: Failed to send PDU");

    return;
}
