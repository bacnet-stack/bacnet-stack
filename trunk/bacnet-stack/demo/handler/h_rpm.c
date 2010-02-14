/**************************************************************************
*
* Copyright (C) 2007 Steve Karg <skarg@users.sourceforge.net>
* Inspired by John Stachler <John.Stachler@lennoxind.com>
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
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "txbuf.h"
#include "memcopy.h"
#include "bacdef.h"
#include "bacdcode.h"
#include "apdu.h"
#include "npdu.h"
#include "abort.h"
#include "rpm.h"
#include "handlers.h"

/** @file h_rpm.c  Handles Read Property Multiple requests. */

/* function that handles the reading of properties from objects */
static read_property_function Read_Property_Function;
static rpm_object_property_lists_function RPM_Property_List;

void handler_rpm_function_set(
    read_property_function pFunction)
{
    Read_Property_Function = pFunction;
}

void handler_rpm_list_set(
    rpm_object_property_lists_function pFunction)
{
    RPM_Property_List = pFunction;
}

static uint8_t Temp_Buf[MAX_APDU] = { 0 };

static BACNET_PROPERTY_ID RPM_Object_Property(
    struct special_property_list_t *pPropertyList,
    BACNET_PROPERTY_ID special_property,
    unsigned index)
{
    int property = -1;  /* return value */
    unsigned required, optional, proprietary;

    required = pPropertyList->Required.count;
    optional = pPropertyList->Optional.count;
    proprietary = pPropertyList->Proprietary.count;
    if (special_property == PROP_ALL) {
        if (index < required) {
            property = pPropertyList->Required.pList[index];
        } else if (index < (required + optional)) {
            index -= required;
            property = pPropertyList->Optional.pList[index];
        } else if (index < (required + optional + proprietary)) {
            index -= (required + optional);
            property = pPropertyList->Proprietary.pList[index];
        }
    } else if (special_property == PROP_REQUIRED) {
        if (index < required) {
            property = pPropertyList->Required.pList[index];
        }
    } else if (special_property == PROP_OPTIONAL) {
        if (index < optional) {
            property = pPropertyList->Optional.pList[index];
        }
    }

    return (BACNET_PROPERTY_ID)property;
}

static unsigned RPM_Object_Property_Count(
    struct special_property_list_t *pPropertyList,
    BACNET_PROPERTY_ID special_property)
{
    unsigned count = 0; /* return value */

    if (special_property == PROP_ALL) {
        count =
            pPropertyList->Required.count + pPropertyList->Optional.count +
            pPropertyList->Proprietary.count;
    } else if (special_property == PROP_REQUIRED) {
        count = pPropertyList->Required.count;
    } else if (special_property == PROP_OPTIONAL) {
        count = pPropertyList->Optional.count;
    }

    return count;
}

/* Encode the RPM property returning the length of the encoding,
   or 0 if there is no room to fit the encoding.  */
static int RPM_Encode_Property(
    uint8_t * apdu,
    uint16_t offset,
    uint16_t max_apdu,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    int32_t array_index)
{
    int len = 0;
    size_t copy_len = 0;
    int apdu_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata;

    len =
        rpm_ack_encode_apdu_object_property(&Temp_Buf[0], object_property,
        array_index);
    copy_len = memcopy(&apdu[0], &Temp_Buf[0], offset, len, max_apdu);
    if (copy_len == 0) {
        return 0;
    }
    apdu_len += len;
    len = 0;
    if (Read_Property_Function) {
        rpdata.error_class = ERROR_CLASS_OBJECT;
        rpdata.error_code = ERROR_CODE_UNKNOWN_OBJECT;
        rpdata.object_type = object_type;
        rpdata.object_instance = object_instance;
        rpdata.object_property = object_property;
        rpdata.array_index = array_index;
        rpdata.application_data = &Temp_Buf[0];
        rpdata.application_data_len = sizeof(Temp_Buf);
        len = Read_Property_Function(&rpdata);
    }
    if (len < 0) {
        /* error was returned - encode that for the response */
        len =
            rpm_ack_encode_apdu_object_property_error(&Temp_Buf[0],
            rpdata.error_class, rpdata.error_code);
        copy_len =
            memcopy(&apdu[0], &Temp_Buf[0], offset + apdu_len, len, max_apdu);
        if (copy_len == 0) {
            return 0;
        }
    } else if ((offset + apdu_len + 1 + len + 1) < max_apdu) {
        /* enough room to fit the property value and tags */
        len =
            rpm_ack_encode_apdu_object_property_value(&apdu[offset + apdu_len],
            &Temp_Buf[0], len);
    } else {
        /* not enough room - abort! */
        return 0;
    }
    apdu_len += len;

    return apdu_len;
}

void handler_read_property_multiple(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src,
    BACNET_CONFIRMED_SERVICE_DATA * service_data)
{
    int len = 0;
    int copy_len = 0;
    int decode_len = 0;
    int pdu_len = 0;
    BACNET_NPDU_DATA npdu_data;
    int bytes_sent;
    BACNET_ADDRESS my_address;
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance = 0;
    int apdu_len = 0;
    int npdu_len = 0;
    BACNET_PROPERTY_ID object_property;
    int32_t array_index = 0;

    /* jps_debug - see if we are utilizing all the buffer */
    /* memset(&Handler_Transmit_Buffer[0], 0xff, sizeof(Handler_Transmit_Buffer)); */
    /* encode the NPDU portion of the packet */
    datalink_get_my_address(&my_address);
    npdu_encode_npdu_data(&npdu_data, false, MESSAGE_PRIORITY_NORMAL);
    npdu_len =
        npdu_encode_pdu(&Handler_Transmit_Buffer[0], src, &my_address,
        &npdu_data);
    if (service_data->segmented_message) {
        apdu_len =
            abort_encode_apdu(&Handler_Transmit_Buffer[npdu_len],
            service_data->invoke_id, ABORT_REASON_SEGMENTATION_NOT_SUPPORTED,
            true);
#if PRINT_ENABLED
        printf("RPM: Segmented message. Sending Abort!\r\n");
#endif
        goto RPM_ABORT;
    }
    /* decode apdu request & encode apdu reply
       encode complex ack, invoke id, service choice */
    apdu_len =
        rpm_ack_encode_apdu_init(&Handler_Transmit_Buffer[npdu_len],
        service_data->invoke_id);
    do {
        len =
            rpm_decode_object_id(&service_request[decode_len],
            service_len - decode_len, &object_type, &object_instance);
        /* end of object? */
        if (len > 0) {
            decode_len += len;
        } else {
            len =
                rpm_decode_object_end(&service_request[decode_len],
                service_len - decode_len);
            if (len == 1) {
                decode_len++;
                len = rpm_ack_encode_apdu_object_end(&Temp_Buf[0]);
                copy_len =
                    memcopy(&Handler_Transmit_Buffer[npdu_len], &Temp_Buf[0],
                    apdu_len, len, sizeof(Handler_Transmit_Buffer));
                if (!copy_len) {
                    apdu_len =
                        abort_encode_apdu(&Handler_Transmit_Buffer[npdu_len],
                        service_data->invoke_id,
                        ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
                    goto RPM_ABORT;
                } else {
                    apdu_len += copy_len;
                }
            } else {
                apdu_len =
                    abort_encode_apdu(&Handler_Transmit_Buffer[npdu_len],
                    service_data->invoke_id, ABORT_REASON_OTHER, true);
                goto RPM_ABORT;
            }
            break;
        }
        len =
            rpm_ack_encode_apdu_object_begin(&Temp_Buf[0], object_type,
            object_instance);
        copy_len =
            memcopy(&Handler_Transmit_Buffer[npdu_len], &Temp_Buf[0], apdu_len,
            len, sizeof(Handler_Transmit_Buffer));
        if (!copy_len) {
            apdu_len =
                abort_encode_apdu(&Handler_Transmit_Buffer[npdu_len],
                service_data->invoke_id,
                ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
            goto RPM_ABORT;
        } else {
            apdu_len += copy_len;
        }
        /* do each property of this object of the RPM request */
        do {
            len =
                rpm_decode_object_property(&service_request[decode_len],
                service_len - decode_len, &object_property, &array_index);
            /* end of property list? */
            if (len > 0) {
                decode_len += len;
            } else {
                len =
                    rpm_decode_object_end(&service_request[decode_len],
                    service_len - decode_len);
                if (len == 1) {
                    decode_len++;
                    len = rpm_ack_encode_apdu_object_end(&Temp_Buf[0]);
                    copy_len =
                        memcopy(&Handler_Transmit_Buffer[npdu_len],
                        &Temp_Buf[0], apdu_len, len,
                        sizeof(Handler_Transmit_Buffer));
                    if (!copy_len) {
                        apdu_len =
                            abort_encode_apdu(&Handler_Transmit_Buffer
                            [npdu_len], service_data->invoke_id,
                            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
                        goto RPM_ABORT;
                    } else {
                        apdu_len += copy_len;
                    }
                } else {
                    apdu_len =
                        abort_encode_apdu(&Handler_Transmit_Buffer[npdu_len],
                        service_data->invoke_id, ABORT_REASON_OTHER, true);
                    goto RPM_ABORT;
                }
                /* stop decoding properties */
                break;
            }
            /* handle the special properties */
            if ((object_property == PROP_ALL) ||
                (object_property == PROP_REQUIRED) ||
                (object_property == PROP_OPTIONAL)) {
                struct special_property_list_t property_list;
                unsigned property_count = 0;
                unsigned index = 0;
                BACNET_PROPERTY_ID special_object_property;

                special_object_property = object_property;
                RPM_Property_List(object_type, &property_list);
                property_count =
                    RPM_Object_Property_Count(&property_list,
                    special_object_property);
                if (property_count == 0) {
                    /* handle the error code - but use the special property */
                    len =
                        RPM_Encode_Property(&Handler_Transmit_Buffer[0],
                        npdu_len + apdu_len, MAX_APDU, object_type,
                        object_instance, object_property, array_index);
                    if (len > 0) {
                        apdu_len += len;
                    } else {
                        apdu_len =
                            abort_encode_apdu(&Handler_Transmit_Buffer
                            [npdu_len], service_data->invoke_id,
                            ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
                        goto RPM_ABORT;
                    }
                } else {
                    for (index = 0; index < property_count; index++) {
                        object_property =
                            RPM_Object_Property(&property_list,
                            special_object_property, index);
                        len =
                            RPM_Encode_Property(&Handler_Transmit_Buffer[0],
                            npdu_len + apdu_len, MAX_APDU, object_type,
                            object_instance, object_property, array_index);
                        if (len > 0) {
                            apdu_len += len;
                        } else {
                            apdu_len =
                                abort_encode_apdu(&Handler_Transmit_Buffer
                                [npdu_len], service_data->invoke_id,
                                ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
                            goto RPM_ABORT;
                        }
                    }
                }
            } else {
                /* handle an individual property */
                len =
                    RPM_Encode_Property(&Handler_Transmit_Buffer[0],
                    npdu_len + apdu_len, sizeof(Handler_Transmit_Buffer),
                    object_type, object_instance, object_property,
                    array_index);
                if (len > 0) {
                    apdu_len += len;
                } else {
                    apdu_len =
                        abort_encode_apdu(&Handler_Transmit_Buffer[npdu_len],
                        service_data->invoke_id,
                        ABORT_REASON_SEGMENTATION_NOT_SUPPORTED, true);
                    goto RPM_ABORT;
                }
            }
        } while (1);
        if (decode_len >= service_len) {
            break;
        }
    } while (1);
    if (apdu_len > service_data->max_resp) {
        /* too big for the sender - send an abort */
        apdu_len =
            abort_encode_apdu(&Handler_Transmit_Buffer[npdu_len],
            service_data->invoke_id, ABORT_REASON_SEGMENTATION_NOT_SUPPORTED,
            true);
#if PRINT_ENABLED
        fprintf(stderr, "RPM: Message too large.  Sending Abort!\n");
#endif
        goto RPM_ABORT;
    }
  RPM_ABORT:
    pdu_len = apdu_len + npdu_len;
    bytes_sent =
        datalink_send_pdu(src, &npdu_data, &Handler_Transmit_Buffer[0],
        pdu_len);
}
