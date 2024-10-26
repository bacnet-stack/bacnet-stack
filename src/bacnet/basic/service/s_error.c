/**************************************************************************
 *
 * Copyright (C) 2016 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacerror.h"
#include "bacnet/dcc.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
/* basic services, TSM, binding, and datalink */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/services.h"

/** Encodes an Error message
 * @param buffer The buffer to build the message for sending.
 * @param dest - Destination address to send the message
 * @param src - Source address from which the message originates
 * @param npdu_data - buffer to hold NPDU data encoded
 * @param invoke_id - use to match up a reply
 * @param reason - #BACNET_ABORT_REASON enumeration
 * @param server - true or false
 *
 * @return Size of the message sent (bytes), or a negative value on error.
 */
int error_encode_pdu(
    uint8_t *buffer,
    BACNET_ADDRESS *dest,
    BACNET_ADDRESS *src,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t invoke_id,
    BACNET_CONFIRMED_SERVICE service,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    int len = 0;
    int pdu_len = 0;

    /* encode the NPDU portion of the packet */
    npdu_encode_npdu_data(npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(&buffer[0], dest, src, npdu_data);

    /* encode the APDU portion of the packet */
    len = bacerror_encode_apdu(
        &buffer[pdu_len], invoke_id, service, error_class, error_code);
    pdu_len += len;

    return pdu_len;
}

/** Sends an Abort message
 * @param buffer The buffer to build the message for sending.
 * @param dest - Destination address to send the message
 * @param invoke_id - use to match up a reply
 * @param reason - #BACNET_ABORT_REASON enumeration
 * @param server - true or false
 *
 * @return Size of the message sent (bytes), or a negative value on error.
 */
int Send_Error_To_Network(
    uint8_t *buffer,
    BACNET_ADDRESS *dest,
    uint8_t invoke_id,
    BACNET_CONFIRMED_SERVICE service,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    int pdu_len = 0;
    BACNET_ADDRESS src;
    int bytes_sent = 0;
    BACNET_NPDU_DATA npdu_data;

    datalink_get_my_address(&src);
    pdu_len = error_encode_pdu(
        buffer, dest, &src, &npdu_data, invoke_id, service, error_class,
        error_code);
    bytes_sent = datalink_send_pdu(dest, &npdu_data, &buffer[0], pdu_len);

    return bytes_sent;
}
