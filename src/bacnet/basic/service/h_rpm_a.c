/**************************************************************************
 *
 * Copyright (C) 2008 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/bactext.h"
#include "bacnet/rpm.h"
/* some demo stuff needed */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"

#define PRINTF debug_aprintf
#define PERROR debug_perror

/** @file h_rpm_a.c  Handles Read Property Multiple Acknowledgments. */

/** Decode the received RPM data and make a linked list of the results.
 * @ingroup DSRPM
 *
 * @param apdu [in] The received apdu data.
 * @param apdu_len [in] Total length of the apdu.
 * @param read_access_data [out] Pointer to the head of the linked list
 *          where the RPM data is to be stored.
 * @return The number of bytes decoded, or -1 on error
 */
int rpm_ack_decode_service_request(
    const uint8_t *apdu,
    int apdu_len,
    BACNET_READ_ACCESS_DATA *read_access_data)
{
    int decoded_len = 0; /* return value */
    uint32_t error_value = 0; /* decoded error value */
    int len = 0; /* number of bytes returned from decoding */
    uint8_t tag_number = 0; /* decoded tag number */
    uint32_t len_value = 0; /* decoded length value */
    int data_len = 0; /* data blob length */
    BACNET_READ_ACCESS_DATA *rpm_object;
    BACNET_READ_ACCESS_DATA *old_rpm_object;
    BACNET_PROPERTY_REFERENCE *rpm_property;
    BACNET_PROPERTY_REFERENCE *old_rpm_property;
    BACNET_APPLICATION_DATA_VALUE *value;
    BACNET_APPLICATION_DATA_VALUE *old_value;

    if (!read_access_data) {
        return 0;
    }
    rpm_object = read_access_data;
    old_rpm_object = rpm_object;
    while (rpm_object && apdu_len) {
        len = rpm_ack_decode_object_id(
            apdu, apdu_len, &rpm_object->object_type,
            &rpm_object->object_instance);
        if (len <= 0) {
            old_rpm_object->next = NULL;
            if (rpm_object != read_access_data) {
                /* don't free original */
                free(rpm_object);
                rpm_object = NULL;
            }
            break;
        }
        decoded_len += len;
        apdu_len -= len;
        apdu += len;
        rpm_property = calloc(1, sizeof(BACNET_PROPERTY_REFERENCE));
        rpm_object->listOfProperties = rpm_property;
        old_rpm_property = rpm_property;
        while (rpm_property && apdu_len) {
            len = rpm_ack_decode_object_property(
                apdu, apdu_len, &rpm_property->propertyIdentifier,
                &rpm_property->propertyArrayIndex);
            if (len <= 0) {
                old_rpm_property->next = NULL;
                if (rpm_object->listOfProperties == rpm_property) {
                    /* was this the only property in the list? */
                    rpm_object->listOfProperties = NULL;
                }
                free(rpm_property);
                rpm_property = NULL;
                break;
            }
            decoded_len += len;
            apdu_len -= len;
            apdu += len;
            if (apdu_len && decode_is_opening_tag_number(apdu, 4)) {
                data_len = bacnet_enclosed_data_length(apdu, apdu_len);
                /* propertyValue */
                decoded_len++;
                apdu_len--;
                apdu++;
                /* note: if this is an array, there will be
                   more than one element to decode */
                value = calloc(1, sizeof(BACNET_APPLICATION_DATA_VALUE));
                rpm_property->value = value;

                /* Special case for an empty array - we decode it as null */
                if (apdu_len && decode_is_closing_tag_number(apdu, 4)) {
                    bacapp_value_list_init(value, 1);
                    decoded_len++;
                    apdu_len--;
                    apdu++;
                } else {
                    while (value && (apdu_len > 0)) {
                        len = bacapp_decode_known_property(
                            apdu, (unsigned)apdu_len, value,
                            rpm_object->object_type,
                            rpm_property->propertyIdentifier);
                        /* If len == 0 then it's an empty structure, which is
                         * OK. */
                        if (len < 0) {
                            /* problem decoding */
                            if (data_len >= 0) {
                                /* valid data that we'll skip over */
                                len = data_len;
                                bacapp_value_list_init(value, 1);
                            } else {
                                PERROR(
                                    "RPM Ack: unable to decode! %s:%s\n",
                                    bactext_object_type_name(
                                        rpm_object->object_type),
                                    bactext_property_name(
                                        rpm_property->propertyIdentifier));
                                /* note: caller will free the memory */
                                return BACNET_STATUS_ERROR;
                            }
                        }
                        decoded_len += len;
                        apdu_len -= len;
                        apdu += len;
                        if (apdu_len && decode_is_closing_tag_number(apdu, 4)) {
                            decoded_len++;
                            apdu_len--;
                            apdu++;
                            break;
                        } else if (len > 0) {
                            old_value = value;
                            value = calloc(
                                1, sizeof(BACNET_APPLICATION_DATA_VALUE));
                            old_value->next = value;
                        } else {
                            PERROR(
                                "RPM Ack: decoded %s:%s len=%d\n",
                                bactext_object_type_name(
                                    rpm_object->object_type),
                                bactext_property_name(
                                    rpm_property->propertyIdentifier),
                                len);
                            break;
                        }
                    }
                }
            } else if (apdu_len && decode_is_opening_tag_number(apdu, 5)) {
                /* propertyAccessError */
                decoded_len++;
                apdu_len--;
                apdu++;
                /* decode the class and code sequence */
                len =
                    decode_tag_number_and_value(apdu, &tag_number, &len_value);
                decoded_len += len;
                apdu_len -= len;
                apdu += len;
                /* FIXME: we could validate that the tag is enumerated... */
                len = decode_enumerated(apdu, len_value, &error_value);
                rpm_property->error.error_class =
                    (BACNET_ERROR_CLASS)error_value;
                decoded_len += len;
                apdu_len -= len;
                apdu += len;
                len =
                    decode_tag_number_and_value(apdu, &tag_number, &len_value);
                decoded_len += len;
                apdu_len -= len;
                apdu += len;
                /* FIXME: we could validate that the tag is enumerated... */
                len = decode_enumerated(apdu, len_value, &error_value);
                rpm_property->error.error_code = (BACNET_ERROR_CODE)error_value;
                decoded_len += len;
                apdu_len -= len;
                apdu += len;
                if (apdu_len && decode_is_closing_tag_number(apdu, 5)) {
                    decoded_len++;
                    apdu_len--;
                    apdu++;
                }
            }
            old_rpm_property = rpm_property;
            rpm_property = calloc(1, sizeof(BACNET_PROPERTY_REFERENCE));
            old_rpm_property->next = rpm_property;
        }
        len = rpm_decode_object_end(apdu, apdu_len);
        if (len) {
            decoded_len += len;
            apdu_len -= len;
            apdu += len;
        }
        if (apdu_len) {
            old_rpm_object = rpm_object;
            rpm_object = calloc(1, sizeof(BACNET_READ_ACCESS_DATA));
            old_rpm_object->next = rpm_object;
        }
    }

    return decoded_len;
}

/* for debugging... */
void rpm_ack_print_data(BACNET_READ_ACCESS_DATA *rpm_data)
{
#ifdef BACAPP_PRINT_ENABLED
    BACNET_OBJECT_PROPERTY_VALUE object_value; /* for bacapp printing */
#endif
    BACNET_PROPERTY_REFERENCE *listOfProperties = NULL;
    BACNET_APPLICATION_DATA_VALUE *value = NULL;
    bool array_value = false;

    if (rpm_data) {
        PRINTF(
            "%s #%lu\r\n", bactext_object_type_name(rpm_data->object_type),
            (unsigned long)rpm_data->object_instance);
        PRINTF("{\r\n");
        listOfProperties = rpm_data->listOfProperties;
        while (listOfProperties) {
            if ((listOfProperties->propertyIdentifier < 512) ||
                (listOfProperties->propertyIdentifier > 4194303)) {
                /* Enumerated values 0-511 and 4194304+ are reserved
                   for definition by ASHRAE.*/
                PRINTF(
                    "    %s: ",
                    bactext_property_name(
                        listOfProperties->propertyIdentifier));
            } else {
                /* Enumerated values 512-4194303 may be used
                    by others subject to the procedures and
                    constraints described in Clause 23. */
                PRINTF(
                    "    proprietary %u: ",
                    (unsigned)listOfProperties->propertyIdentifier);
            }
            if (listOfProperties->propertyArrayIndex != BACNET_ARRAY_ALL) {
                PRINTF("[%d]", listOfProperties->propertyArrayIndex);
            }
            value = listOfProperties->value;
            if (value) {
                if (value->next) {
                    PRINTF("{");
                    array_value = true;
                } else {
                    array_value = false;
                }
#ifdef BACAPP_PRINT_ENABLED
                object_value.object_type = rpm_data->object_type;
                object_value.object_instance = rpm_data->object_instance;
#endif
                while (value) {
#ifdef BACAPP_PRINT_ENABLED
                    object_value.object_property =
                        listOfProperties->propertyIdentifier;
                    object_value.array_index =
                        listOfProperties->propertyArrayIndex;
                    object_value.value = value;
                    bacapp_print_value(stdout, &object_value);
#endif
                    if (value->next) {
                        PRINTF(",\r\n        ");
                    } else {
                        if (array_value) {
                            PRINTF("}\r\n");
                        } else {
                            PRINTF("\r\n");
                        }
                    }
                    value = value->next;
                }
            } else {
                /* AccessError */
                PRINTF(
                    "BACnet Error: %s: %s\r\n",
                    bactext_error_class_name(
                        (int)listOfProperties->error.error_class),
                    bactext_error_code_name(
                        (int)listOfProperties->error.error_code));
            }
            listOfProperties = listOfProperties->next;
        }
        PRINTF("}\r\n");
    }
}

/**
 * Free the allocated memory from a ReadPropertyMultiple ACK.
 * @param rpm_data - #BACNET_READ_ACCESS_DATA
 * @return RPM data from the next element in the linked list
 */
BACNET_READ_ACCESS_DATA *rpm_data_free(BACNET_READ_ACCESS_DATA *rpm_data)
{
    BACNET_READ_ACCESS_DATA *old_rpm_data = NULL;
    BACNET_PROPERTY_REFERENCE *rpm_property = NULL;
    BACNET_PROPERTY_REFERENCE *old_rpm_property = NULL;
    BACNET_APPLICATION_DATA_VALUE *value = NULL;
    BACNET_APPLICATION_DATA_VALUE *old_value = NULL;

    if (rpm_data) {
        rpm_property = rpm_data->listOfProperties;
        while (rpm_property) {
            value = rpm_property->value;
            while (value) {
                old_value = value;
                value = value->next;
                free(old_value);
                old_value = NULL;
            }
            old_rpm_property = rpm_property;
            rpm_property = rpm_property->next;
            free(old_rpm_property);
            old_rpm_property = NULL;
        }
        old_rpm_data = rpm_data;
        rpm_data = rpm_data->next;
        free(old_rpm_data);
        old_rpm_data = NULL;
    }

    return rpm_data;
}

/** Handler for a ReadPropertyMultiple ACK.
 * @ingroup DSRPM
 * For each read property, print out the ACK'd data for debugging,
 * and free the request data items from linked property list.
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information
 *                          decoded from the APDU header of this message.
 */
void handler_read_property_multiple_ack(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data)
{
    int len = 0;
    BACNET_READ_ACCESS_DATA *rpm_data;

    (void)src;
    (void)service_data; /* we could use these... */

    rpm_data = calloc(1, sizeof(BACNET_READ_ACCESS_DATA));
    if (rpm_data) {
        len = rpm_ack_decode_service_request(
            service_request, service_len, rpm_data);
        if (len > 0) {
            while (rpm_data) {
                rpm_ack_print_data(rpm_data);
                rpm_data = rpm_data_free(rpm_data);
            }
        } else {
            PERROR("RPM Ack Malformed! Freeing memory...\n");
            while (rpm_data) {
                rpm_data = rpm_data_free(rpm_data);
            }
        }
    }
}
