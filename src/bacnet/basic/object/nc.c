/**
 * @file
 * @author Krzysztof Malorny <malornykrzysztof@gmail.com>
 * @author Ed Hague <edward@bac-test.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2011, 2018
 * @brief A basic BACnet Notification Class object
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/bacapp.h"
#include "bacnet/bacdest.h"
#include "bacnet/datetime.h"
#include "bacnet/event.h"
#include "bacnet/wp.h"
/* basic objects and services */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/nc.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"

#ifndef MAX_NOTIFICATION_CLASSES
#define MAX_NOTIFICATION_CLASSES 2
#endif

#if defined(INTRINSIC_REPORTING)
static NOTIFICATION_CLASS_INFO NC_Info[MAX_NOTIFICATION_CLASSES];
/* buffer for sending event messages */
static uint8_t Event_Buffer[MAX_APDU];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER, PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,       PROP_NOTIFICATION_CLASS,
    PROP_PRIORITY,          PROP_ACK_REQUIRED,
    PROP_RECIPIENT_LIST,    -1
};

static const int Properties_Optional[] = { PROP_DESCRIPTION, -1 };

static const int Properties_Proprietary[] = { -1 };

void Notification_Class_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired) {
        *pRequired = Properties_Required;
    }
    if (pOptional) {
        *pOptional = Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Properties_Proprietary;
    }
    return;
}

void Notification_Class_Init(void)
{
    uint8_t NotifyIdx = 0;
    unsigned i;

    for (NotifyIdx = 0; NotifyIdx < MAX_NOTIFICATION_CLASSES; NotifyIdx++) {
        /* init with zeros */
        memset(&NC_Info[NotifyIdx], 0x00, sizeof(NOTIFICATION_CLASS_INFO));
        /* set the basic parameters */
        NC_Info[NotifyIdx].Ack_Required = 0;
        /* The lowest priority for Normal message = 255 */
        NC_Info[NotifyIdx].Priority[TRANSITION_TO_OFFNORMAL] = 255;
        NC_Info[NotifyIdx].Priority[TRANSITION_TO_FAULT] = 255;
        NC_Info[NotifyIdx].Priority[TRANSITION_TO_NORMAL] = 255;
        /* note: default uses wildcard device destination */
        for (i = 0; i < NC_MAX_RECIPIENTS; i++) {
            BACNET_DESTINATION *destination;
            destination = &NC_Info[NotifyIdx].Recipient_List[i];
            bacnet_destination_default_init(destination);
        }
    }

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Notification_Class_Valid_Instance(uint32_t object_instance)
{
    unsigned int index;

    index = Notification_Class_Instance_To_Index(object_instance);
    if (index < MAX_NOTIFICATION_CLASSES) {
        return true;
    }

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Notification_Class_Count(void)
{
    return MAX_NOTIFICATION_CLASSES;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Notification_Class_Index_To_Instance(unsigned index)
{
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Notification_Class_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = MAX_NOTIFICATION_CLASSES;

    if (object_instance < MAX_NOTIFICATION_CLASSES) {
        index = object_instance;
    }

    return index;
}

bool Notification_Class_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    char text[32] = "";
    unsigned int index;
    bool status = false;

    index = Notification_Class_Instance_To_Index(object_instance);
    if (index < MAX_NOTIFICATION_CLASSES) {
        snprintf(
            text, sizeof(text), "NOTIFICATION CLASS %lu",
            (unsigned long)object_instance);
        status = characterstring_init_ansi(object_name, text);
    }

    return status;
}

int Notification_Class_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    NOTIFICATION_CLASS_INFO *CurrentNotify;
    BACNET_CHARACTER_STRING char_string;
    BACNET_BIT_STRING bit_string;
    uint8_t *apdu = NULL;
    uint8_t u8Val;
    int idx;
    int apdu_len = 0; /* return value */
    uint16_t apdu_max = 0;

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }

    apdu = rpdata->application_data;
    apdu_max = rpdata->application_data_len;
    CurrentNotify =
        &NC_Info[Notification_Class_Instance_To_Index(rpdata->object_instance)];

    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_NOTIFICATION_CLASS, rpdata->object_instance);
            break;

        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            Notification_Class_Object_Name(
                rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;

        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(
                &apdu[0], OBJECT_NOTIFICATION_CLASS);
            break;

        case PROP_NOTIFICATION_CLASS:
            apdu_len +=
                encode_application_unsigned(&apdu[0], rpdata->object_instance);
            break;

        case PROP_PRIORITY:
            if (rpdata->array_index == 0) {
                apdu_len += encode_application_unsigned(&apdu[0], 3);
            } else {
                if (rpdata->array_index == BACNET_ARRAY_ALL) {
                    apdu_len += encode_application_unsigned(
                        &apdu[apdu_len],
                        CurrentNotify->Priority[TRANSITION_TO_OFFNORMAL]);
                    apdu_len += encode_application_unsigned(
                        &apdu[apdu_len],
                        CurrentNotify->Priority[TRANSITION_TO_FAULT]);
                    apdu_len += encode_application_unsigned(
                        &apdu[apdu_len],
                        CurrentNotify->Priority[TRANSITION_TO_NORMAL]);
                } else if (rpdata->array_index <= MAX_BACNET_EVENT_TRANSITION) {
                    apdu_len += encode_application_unsigned(
                        &apdu[apdu_len],
                        CurrentNotify->Priority[rpdata->array_index - 1]);
                } else {
                    rpdata->error_class = ERROR_CLASS_PROPERTY;
                    rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                    apdu_len = -1;
                }
            }
            break;

        case PROP_ACK_REQUIRED:
            u8Val = CurrentNotify->Ack_Required;

            bitstring_init(&bit_string);
            bitstring_set_bit(
                &bit_string, TRANSITION_TO_OFFNORMAL,
                (u8Val & TRANSITION_TO_OFFNORMAL_MASKED) ? true : false);
            bitstring_set_bit(
                &bit_string, TRANSITION_TO_FAULT,
                (u8Val & TRANSITION_TO_FAULT_MASKED) ? true : false);
            bitstring_set_bit(
                &bit_string, TRANSITION_TO_NORMAL,
                (u8Val & TRANSITION_TO_NORMAL_MASKED) ? true : false);
            /* encode bitstring */
            apdu_len +=
                encode_application_bitstring(&apdu[apdu_len], &bit_string);
            break;

        case PROP_RECIPIENT_LIST:
            /* get the size of all entry of Recipient_List */
            for (idx = 0; idx < NC_MAX_RECIPIENTS; idx++) {
                BACNET_DESTINATION *Destination;
                BACNET_RECIPIENT *Recipient;
                Destination = &CurrentNotify->Recipient_List[idx];
                Recipient = &Destination->Recipient;
                if (!bacnet_recipient_device_wildcard(Recipient)) {
                    /* unused slot denoted by wildcard */
                    apdu_len += bacnet_destination_encode(NULL, Destination);
                }
            }
            if (apdu_len > apdu_max) {
                /* Abort response */
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                apdu_len = BACNET_STATUS_ABORT;
                break;
            }
            /* size fits, therefore, encode all entry of Recipient_List */
            apdu_len = 0;
            for (idx = 0; idx < NC_MAX_RECIPIENTS; idx++) {
                BACNET_DESTINATION *Destination;
                BACNET_RECIPIENT *Recipient;
                Destination = &CurrentNotify->Recipient_List[idx];
                Recipient = &Destination->Recipient;
                if (!bacnet_recipient_device_wildcard(Recipient)) {
                    /* unused slot denoted by wildcard */
                    apdu_len +=
                        bacnet_destination_encode(&apdu[apdu_len], Destination);
                }
            }
            break;

        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = -1;
            break;
    }

    return apdu_len;
}

bool Notification_Class_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    NOTIFICATION_CLASS_INFO *CurrentNotify;
    NOTIFICATION_CLASS_INFO TmpNotify;
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    uint8_t TmpPriority[MAX_BACNET_EVENT_TRANSITION]; /* BACnetARRAY[3] of
                                                         Unsigned */
    bool status = false;
    int iOffset;
    uint8_t idx;
    int len = 0;

    CurrentNotify = &NC_Info[Notification_Class_Instance_To_Index(
        wp_data->object_instance)];

    /* decode some of the request */
    len = bacapp_decode_application_data(
        wp_data->application_data, wp_data->application_data_len, &value);
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    switch (wp_data->object_property) {
        case PROP_PRIORITY:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                if (wp_data->array_index == 0) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                    status = false;
                } else if (wp_data->array_index == BACNET_ARRAY_ALL) {
                    iOffset = 0;
                    for (idx = 0; idx < MAX_BACNET_EVENT_TRANSITION; idx++) {
                        len = bacapp_decode_application_data(
                            &wp_data->application_data[iOffset],
                            wp_data->application_data_len, &value);
                        if ((len == 0) ||
                            (value.tag !=
                             BACNET_APPLICATION_TAG_UNSIGNED_INT)) {
                            /* Bad decode, wrong tag or following required
                             * parameter missing */
                            wp_data->error_class = ERROR_CLASS_PROPERTY;
                            wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
                            status = false;
                            break;
                        }
                        if (value.type.Unsigned_Int > 255) {
                            wp_data->error_class = ERROR_CLASS_PROPERTY;
                            wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                            status = false;
                            break;
                        }
                        TmpPriority[idx] = (uint8_t)value.type.Unsigned_Int;
                        iOffset += len;
                    }
                    if (status == true) {
                        for (idx = 0; idx < MAX_BACNET_EVENT_TRANSITION;
                             idx++) {
                            CurrentNotify->Priority[idx] = TmpPriority[idx];
                        }
                    }
                } else if (wp_data->array_index <= 3) {
                    if (value.type.Unsigned_Int > 255) {
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                        status = false;
                    } else {
                        CurrentNotify->Priority[wp_data->array_index - 1] =
                            value.type.Unsigned_Int;
                    }
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                    status = false;
                }
            }
            break;

        case PROP_ACK_REQUIRED:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BIT_STRING);
            if (status) {
                if (value.type.Bit_String.bits_used == 3) {
                    CurrentNotify->Ack_Required =
                        value.type.Bit_String.value[0];
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;

        case PROP_RECIPIENT_LIST:
            for (idx = 0; idx < NC_MAX_RECIPIENTS; idx++) {
                BACNET_DESTINATION *destination;
                destination = &TmpNotify.Recipient_List[idx];
                bacnet_destination_default_init(destination);
            }
            idx = 0;
            iOffset = 0;
            /* decode all packed */
            while (iOffset < wp_data->application_data_len) {
                BACNET_DESTINATION *destination;
                destination = &TmpNotify.Recipient_List[idx];
                len = bacnet_destination_decode(
                    &wp_data->application_data[iOffset],
                    wp_data->application_data_len, destination);
                if (len == BACNET_STATUS_REJECT) {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_INVALID_DATA_TYPE;
                    return false;
                }
                iOffset += len;
                /* Increasing element of list */
                if ((++idx >= NC_MAX_RECIPIENTS) &&
                    (iOffset < wp_data->application_data_len)) {
                    wp_data->error_class = ERROR_CLASS_RESOURCES;
                    wp_data->error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
                    return false;
                }
            }
            /* Decoded all recipient list */
            /* copy elements from temporary object */
            for (idx = 0; idx < NC_MAX_RECIPIENTS; idx++) {
                BACNET_ADDRESS src = { 0 };
                unsigned max_apdu = 0;
                uint32_t device_id;
                BACNET_DESTINATION *destination;
                BACNET_RECIPIENT *recipient;

                destination = &CurrentNotify->Recipient_List[idx];
                bacnet_destination_copy(
                    destination, &TmpNotify.Recipient_List[idx]);
                recipient = &destination->Recipient;
                if (bacnet_recipient_device_valid(recipient)) {
                    device_id = recipient->type.device.instance;
                    address_bind_request(device_id, &max_apdu, &src);
                } else if (recipient->tag == BACNET_RECIPIENT_TAG_ADDRESS) {
                    /* nothing to do - we have the address */
                }
            }
            status = true;
            break;
        default:
            if (property_lists_member(
                    Properties_Required, Properties_Optional,
                    Properties_Proprietary, wp_data->object_property)) {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            } else {
                wp_data->error_class = ERROR_CLASS_PROPERTY;
                wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            }
            break;
    }

    return status;
}

void Notification_Class_Get_Priorities(
    uint32_t Object_Instance, uint32_t *pPriorityArray)
{
    NOTIFICATION_CLASS_INFO *CurrentNotify;
    uint32_t object_index;
    int i;

    object_index = Notification_Class_Instance_To_Index(Object_Instance);

    if (object_index < MAX_NOTIFICATION_CLASSES) {
        CurrentNotify = &NC_Info[object_index];
    } else {
        for (i = 0; i < 3; i++) {
            pPriorityArray[i] = 255;
        }
        return; /* unknown object */
    }

    for (i = 0; i < 3; i++) {
        pPriorityArray[i] = CurrentNotify->Priority[i];
    }
}

bool Notification_Class_Get_Recipient_List(
    uint32_t Object_Instance, BACNET_DESTINATION *pRecipientList)
{
    uint32_t object_index =
        Notification_Class_Instance_To_Index(Object_Instance);

    if (object_index < MAX_NOTIFICATION_CLASSES) {
        NOTIFICATION_CLASS_INFO *CurrentNotify = &NC_Info[object_index];
        int i;

        for (i = 0; i < NC_MAX_RECIPIENTS; i++) {
            pRecipientList[i] = CurrentNotify->Recipient_List[i];
        }
    } else {
        return false; /* unknown object */
    }

    return true;
}

bool Notification_Class_Set_Recipient_List(
    uint32_t Object_Instance, BACNET_DESTINATION *pRecipientList)
{
    uint32_t object_index =
        Notification_Class_Instance_To_Index(Object_Instance);

    if (object_index < MAX_NOTIFICATION_CLASSES) {
        NOTIFICATION_CLASS_INFO *CurrentNotify = &NC_Info[object_index];
        int i;

        for (i = 0; i < NC_MAX_RECIPIENTS; i++) {
            CurrentNotify->Recipient_List[i] = pRecipientList[i];
        }
    } else {
        return false; /* unknown object */
    }

    return true;
}

void Notification_Class_Set_Priorities(
    uint32_t Object_Instance, uint32_t *pPriorityArray)
{
    uint32_t object_index =
        Notification_Class_Instance_To_Index(Object_Instance);

    if (object_index < MAX_NOTIFICATION_CLASSES) {
        NOTIFICATION_CLASS_INFO *CurrentNotify = &NC_Info[object_index];
        int i;

        for (i = 0; i < 3; i++) {
            if (pPriorityArray[i] <= 255) {
                CurrentNotify->Priority[i] = pPriorityArray[i];
            }
        }
    }
}

void Notification_Class_Get_Ack_Required(
    uint32_t Object_Instance, uint8_t *pAckRequired)
{
    uint32_t object_index =
        Notification_Class_Instance_To_Index(Object_Instance);

    if (object_index < MAX_NOTIFICATION_CLASSES) {
        NOTIFICATION_CLASS_INFO *CurrentNotify = &NC_Info[object_index];
        *pAckRequired = CurrentNotify->Ack_Required;
    } else {
        *pAckRequired = 0;
        return; /* unknown object */
    }
}

void Notification_Class_Set_Ack_Required(
    uint32_t Object_Instance, uint8_t Ack_Required)
{
    uint32_t object_index =
        Notification_Class_Instance_To_Index(Object_Instance);

    if (object_index < MAX_NOTIFICATION_CLASSES) {
        NOTIFICATION_CLASS_INFO *CurrentNotify = &NC_Info[object_index];
        CurrentNotify->Ack_Required = Ack_Required;
    }
}

static bool
IsRecipientActive(BACNET_DESTINATION *pBacDest, uint8_t EventToState)
{
    BACNET_DATE_TIME DateTime;

    /* valid Transitions */
    switch (EventToState) {
        case EVENT_STATE_OFFNORMAL:
        case EVENT_STATE_HIGH_LIMIT:
        case EVENT_STATE_LOW_LIMIT:
            if (!bitstring_bit(
                    &pBacDest->Transitions, TRANSITION_TO_OFFNORMAL)) {
                return false;
            }
            break;

        case EVENT_STATE_FAULT:
            if (!bitstring_bit(&pBacDest->Transitions, TRANSITION_TO_FAULT)) {
                return false;
            }
            break;

        case EVENT_STATE_NORMAL:
            if (!bitstring_bit(&pBacDest->Transitions, TRANSITION_TO_NORMAL)) {
                return false;
            }
            break;

        default:
            return false; /* shouldn't happen */
    }

    /* get actual date and time */
    datetime_local(&DateTime.date, &DateTime.time, NULL, NULL);

    /* valid Days */
    if (!(bitstring_bit(&pBacDest->ValidDays, (DateTime.date.wday - 1)))) {
        return false;
    }
    /* valid FromTime */
    if (datetime_compare_time(&DateTime.time, &pBacDest->FromTime) < 0) {
        return false;
    }

    /* valid ToTime */
    if (datetime_compare_time(&pBacDest->ToTime, &DateTime.time) < 0) {
        return false;
    }

    return true;
}

void Notification_Class_common_reporting_function(
    BACNET_EVENT_NOTIFICATION_DATA *event_data)
{
    /* Fill the parameters common for all types of events. */

    NOTIFICATION_CLASS_INFO *CurrentNotify;
    BACNET_DESTINATION *pBacDest;
    uint32_t notify_index;
    uint8_t index;

    notify_index =
        Notification_Class_Instance_To_Index(event_data->notificationClass);

    if (notify_index < MAX_NOTIFICATION_CLASSES) {
        CurrentNotify = &NC_Info[notify_index];
    } else {
        return;
    }

    /* Initiating Device Identifier */
    event_data->initiatingObjectIdentifier.type = OBJECT_DEVICE;
    event_data->initiatingObjectIdentifier.instance =
        Device_Object_Instance_Number();

    /* Priority and AckRequired */
    switch (event_data->toState) {
        case EVENT_STATE_NORMAL:
            event_data->priority =
                CurrentNotify->Priority[TRANSITION_TO_NORMAL];
            event_data->ackRequired =
                (CurrentNotify->Ack_Required & TRANSITION_TO_NORMAL_MASKED)
                ? true
                : false;
            break;

        case EVENT_STATE_FAULT:
            event_data->priority = CurrentNotify->Priority[TRANSITION_TO_FAULT];
            event_data->ackRequired =
                (CurrentNotify->Ack_Required & TRANSITION_TO_FAULT_MASKED)
                ? true
                : false;
            break;

        case EVENT_STATE_OFFNORMAL:
        case EVENT_STATE_HIGH_LIMIT:
        case EVENT_STATE_LOW_LIMIT:
            event_data->priority =
                CurrentNotify->Priority[TRANSITION_TO_OFFNORMAL];
            event_data->ackRequired =
                (CurrentNotify->Ack_Required & TRANSITION_TO_OFFNORMAL_MASKED)
                ? true
                : false;
            break;

        default: /* shouldn't happen */
            break;
    }

    /* send notifications for active recipients */
    debug_printf_stderr(
        "Notification Class[%u]: send notifications\n",
        event_data->notificationClass);
    /* pointer to first recipient */
    pBacDest = &CurrentNotify->Recipient_List[0];
    for (index = 0; index < NC_MAX_RECIPIENTS; index++, pBacDest++) {
        if (bacnet_recipient_device_wildcard(&pBacDest->Recipient)) {
            /* unused slots denoted by wildcard */
            continue;
        }
        if (IsRecipientActive(pBacDest, event_data->toState)) {
            BACNET_ADDRESS dest;
            uint32_t device_id;
            unsigned max_apdu;

            /* Process Identifier */
            event_data->processIdentifier = pBacDest->ProcessIdentifier;

            /* send notification */
            if (pBacDest->Recipient.tag == BACNET_RECIPIENT_TAG_DEVICE) {
                /* send notification to the specified device */
                device_id = pBacDest->Recipient.type.device.instance;
                debug_printf_stderr(
                    "Notification Class[%u]: send notification to %u\n",
                    event_data->notificationClass, (unsigned)device_id);
                if (pBacDest->ConfirmedNotify == true) {
                    Send_CEvent_Notify(device_id, event_data);
                } else if (address_get_by_device(device_id, &max_apdu, &dest)) {
                    Send_UEvent_Notify(Event_Buffer, event_data, &dest);
                }
            } else if (
                pBacDest->Recipient.tag == BACNET_RECIPIENT_TAG_ADDRESS) {
                debug_printf_stderr(
                    "Notification Class[%u]: send notification to ADDR\n",
                    event_data->notificationClass);
                /* send notification to the address indicated */
                if (pBacDest->ConfirmedNotify == true) {
                    if (address_get_device_id(&dest, &device_id)) {
                        Send_CEvent_Notify(device_id, event_data);
                    }
                } else {
                    dest = pBacDest->Recipient.type.address;
                    Send_UEvent_Notify(Event_Buffer, event_data, &dest);
                }
            }
        }
    }
}

/* This function tries to find the addresses of the defined devices. */
/* It should be called periodically (example once per minute). */
void Notification_Class_find_recipient(void)
{
    NOTIFICATION_CLASS_INFO *notification;
    BACNET_DESTINATION *destination;
    BACNET_RECIPIENT *recipient;
    BACNET_ADDRESS src = { 0 };
    unsigned max_apdu = 0;
    uint32_t device_id;
    unsigned i, j;

    for (i = 0; i < MAX_NOTIFICATION_CLASSES; i++) {
        notification = &NC_Info[i];
        for (j = 0; j < NC_MAX_RECIPIENTS; j++) {
            destination = &notification->Recipient_List[j];
            recipient = &destination->Recipient;
            if (bacnet_recipient_device_valid(recipient)) {
                device_id = recipient->type.device.instance;
                if (!address_bind_request(device_id, &max_apdu, &src)) {
                    /*  Send who_ is request only when
                        address of device is unknown. */
                    Send_WhoIs(device_id, device_id);
                }
            }
        }
    }
}

/**
 * @brief AddListElement from an object list property
 * @ingroup ObjHelpers
 * @param list_element [in] Pointer to the BACnet_List_Element_Data structure,
 * which is packed with the information from the request.
 * @return #BACNET_STATUS_OK or #BACNET_STATUS_ERROR or
 * #BACNET_STATUS_ABORT or #BACNET_STATUS_REJECT.
 * @note 15.1.2 Service Procedure
 * After verifying the validity of the request, the responding BACnet-user
 * shall attempt to modify the object identified in the
 * 'Object Identifier' parameter. If the identified object exists
 * and has the property specified in the 'Property Identifier' parameter,
 * and, if present, has the array element specified in the
 * 'Property Array Index' parameter, an attempt shall be made to add all of
 * the elements specified in the 'List of Elements' parameter to the
 * specified property or array element of the property. If this
 * attempt is successful, a 'Result(+)' primitive shall be issued.
 *
 * When comparing elements in the 'List of Elements' parameter with
 * elements in the specified property or array element of the
 * property, the complete element shall be compared unless the
 * property description specifies otherwise. If one or more of the
 * elements is already present in the BACnetLIST, it shall be updated
 * with the provided element, that is, the existing element is
 * over-written with the provided element. Optionally, if the provided
 * element is exactly the same as the existing element in every
 * way, it can be ignored, that is, not added to the BACnetLIST.
 * Ignoring an element that already exists shall not cause the
 * service to fail.
 *
 * If the specified object does not exist, the specified property
 * does not exist, the specified array element does not exist, or the
 * specified property or array element is not a BACnetLIST, then
 * the service shall fail and a 'Result(-)' response primitive shall
 * be issued. If one or more elements cannot be added to, or updated
 * in, the BACnetLIST, a 'Result(-)' response primitive shall be
 * issued and no elements shall be added to, or updated in, the BACnetLIST.
 *
 * The effect of this service shall be to add to, or update in,
 * the BACnetLIST all of the specified elements, or to neither add nor
 * update any elements at all.
 */
int Notification_Class_Add_List_Element(BACNET_LIST_ELEMENT_DATA *list_element)
{
    NOTIFICATION_CLASS_INFO *notification = NULL;
    BACNET_DESTINATION recipient_list[NC_MAX_RECIPIENTS] = { 0 };
    uint8_t *application_data = NULL;
    int application_data_len = 0, len = 0;
    uint32_t notify_index = 0;
    unsigned index = 0;
    unsigned element_count = 0, new_element_count = 0;
    unsigned added_element_count = 0, same_element_count = 0;
    bool same_element = false;
    unsigned i = 0, j = 0;

    if (!list_element) {
        return BACNET_STATUS_ABORT;
    }
    if (list_element->object_property != PROP_RECIPIENT_LIST) {
        list_element->error_class = ERROR_CLASS_SERVICES;
        list_element->error_code = ERROR_CODE_PROPERTY_IS_NOT_A_LIST;
        return BACNET_STATUS_ERROR;
    }
    if (list_element->array_index != BACNET_ARRAY_ALL) {
        list_element->error_class = ERROR_CLASS_PROPERTY;
        list_element->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return BACNET_STATUS_ERROR;
    }
    notify_index =
        Notification_Class_Instance_To_Index(list_element->object_instance);
    if (notify_index < MAX_NOTIFICATION_CLASSES) {
        notification = &NC_Info[notify_index];
    } else {
        list_element->error_class = ERROR_CLASS_OBJECT;
        list_element->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return BACNET_STATUS_ERROR;
    }
    /* get the current size of all entry of Recipient_List */
    for (index = 0; index < NC_MAX_RECIPIENTS; index++) {
        BACNET_DESTINATION *d1;
        BACNET_RECIPIENT *r1;
        d1 = &notification->Recipient_List[index];
        r1 = &d1->Recipient;
        if (!bacnet_recipient_device_wildcard(r1)) {
            /* unused slots denoted by wildcard */
            element_count++;
        }
    }
    /* decode the elements */
    new_element_count = 0;
    application_data = list_element->application_data;
    application_data_len = list_element->application_data_len;
    while (application_data_len > 0) {
        len = bacnet_destination_decode(
            application_data, application_data_len,
            &recipient_list[new_element_count]);
        if (len > 0) {
            new_element_count++;
            application_data_len -= len;
        } else {
            list_element->first_failed_element_number = new_element_count;
            list_element->error_class = ERROR_CLASS_PROPERTY;
            list_element->error_code = ERROR_CODE_INVALID_DATA_ENCODING;
            return BACNET_STATUS_ERROR;
        }
    }
    if (new_element_count == 0) {
        return BACNET_STATUS_OK;
    }
    /* determine the same and added recipient counts */
    same_element_count = 0;
    added_element_count = 0;
    for (i = 0; i < new_element_count; i++) {
        BACNET_DESTINATION *d1;
        BACNET_RECIPIENT *r1;
        d1 = &recipient_list[i];
        r1 = &d1->Recipient;
        same_element = false;
        for (j = 0; j < NC_MAX_RECIPIENTS; j++) {
            BACNET_DESTINATION *d2;
            BACNET_RECIPIENT *r2;
            d2 = &notification->Recipient_List[j];
            r2 = &d2->Recipient;
            if (bacnet_recipient_same(r1, r2)) {
                same_element = true;
                break;
            }
        }
        if (same_element) {
            same_element_count++;
        } else {
            added_element_count++;
            if ((added_element_count + element_count) > NC_MAX_RECIPIENTS) {
                list_element->first_failed_element_number = 1 + i;
                list_element->error_class = ERROR_CLASS_RESOURCES;
                list_element->error_code =
                    ERROR_CODE_NO_SPACE_TO_ADD_LIST_ELEMENT;
                return BACNET_STATUS_ERROR;
            }
        }
    }
    /* update existing and add new */
    for (i = 0; i < new_element_count; i++) {
        BACNET_DESTINATION *d1;
        BACNET_RECIPIENT *r1;
        d1 = &recipient_list[i];
        r1 = &d1->Recipient;
        same_element = false;
        for (j = 0; j < NC_MAX_RECIPIENTS; j++) {
            BACNET_DESTINATION *d2;
            BACNET_RECIPIENT *r2;
            d2 = &notification->Recipient_List[j];
            r2 = &d2->Recipient;
            if (bacnet_recipient_same(r1, r2)) {
                /* update existing element */
                same_element = true;
                bacnet_destination_copy(d2, d1);
            }
        }
        if (!same_element) {
            /* add new element to next free slot */
            for (j = 0; j < NC_MAX_RECIPIENTS; j++) {
                BACNET_DESTINATION *d2;
                BACNET_RECIPIENT *r2;
                d2 = &notification->Recipient_List[j];
                r2 = &d2->Recipient;
                if (bacnet_recipient_device_wildcard(r2)) {
                    /* unused slot denoted by wildcard */
                    bacnet_destination_copy(d2, d1);
                    break;
                }
            }
        }
    }

    return BACNET_STATUS_OK;
}

/**
 * @brief RemoveListElement from an object list property
 * @ingroup ObjHelpers
 * @param list_element [in] Pointer to the BACnet_List_Element_Data structure,
 * which is packed with the information from the request.
 * @return The length of the apdu encoded or #BACNET_STATUS_ERROR or
 * #BACNET_STATUS_ABORT or #BACNET_STATUS_REJECT.
 * @note After verifying the validity of the request, the responding
 * BACnet-user shall attempt to modify the object identified in the
 * 'Object Identifier' parameter. If the identified object exists and it
 * has the property specified in the 'Property Identifier' parameter,
 * and if present has the array element specified in the 'Property Array Index'
 * parameter, an attempt shall be made to remove the
 * elements in the 'List of Elements' from the property or array element
 * of the property of the object.
 *
 * When comparing elements of the service with entries in the affected
 * BACnetLIST, the complete element shall be compared
 * unless the property description specifies otherwise. If one or more
 * of the elements does not exist or cannot be removed because
 * of insufficient authority, none of the elements shall be removed and
 * a 'Result(-)' response primitive shall be issued.
 */
int Notification_Class_Remove_List_Element(
    BACNET_LIST_ELEMENT_DATA *list_element)
{
    NOTIFICATION_CLASS_INFO *notification = NULL;
    uint32_t notify_index = 0;
    unsigned index = 0;
    BACNET_DESTINATION recipient_list[NC_MAX_RECIPIENTS] = { 0 };
    uint8_t *application_data = NULL;
    int application_data_len = 0, len = 0;
    unsigned element_count = 0;
    unsigned remove_element_count = 0;
    bool same_element = false;
    unsigned i = 0, j = 0;

    if (!list_element) {
        return BACNET_STATUS_ABORT;
    }
    if (list_element->object_property != PROP_RECIPIENT_LIST) {
        list_element->error_class = ERROR_CLASS_SERVICES;
        list_element->error_code = ERROR_CODE_PROPERTY_IS_NOT_A_LIST;
        return BACNET_STATUS_ERROR;
    }
    if (list_element->array_index != BACNET_ARRAY_ALL) {
        list_element->error_class = ERROR_CLASS_PROPERTY;
        list_element->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return BACNET_STATUS_ERROR;
    }
    notify_index =
        Notification_Class_Instance_To_Index(list_element->object_instance);
    if (notify_index < MAX_NOTIFICATION_CLASSES) {
        notification = &NC_Info[notify_index];
    } else {
        list_element->error_class = ERROR_CLASS_OBJECT;
        list_element->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return BACNET_STATUS_ERROR;
    }
    /* get the current size of all entry of Recipient_List */
    for (index = 0; index < NC_MAX_RECIPIENTS; index++) {
        BACNET_DESTINATION *d1;
        BACNET_RECIPIENT *r1;
        d1 = &notification->Recipient_List[index];
        r1 = &d1->Recipient;
        if (!bacnet_recipient_device_wildcard(r1)) {
            /* unused slot denoted by wildcard */
            element_count++;
        }
    }
    /* decode the elements */
    application_data = list_element->application_data;
    application_data_len = list_element->application_data_len;
    while (application_data_len > 0) {
        len = bacnet_destination_decode(
            application_data, application_data_len,
            &recipient_list[remove_element_count]);
        if (len > 0) {
            remove_element_count++;
            application_data_len -= len;
        } else {
            list_element->first_failed_element_number = remove_element_count;
            list_element->error_class = ERROR_CLASS_PROPERTY;
            list_element->error_code = ERROR_CODE_INVALID_DATA_ENCODING;
            return BACNET_STATUS_ERROR;
        }
    }
    /* determine if one or more of the elements does not exist */
    for (i = 0; i < remove_element_count; i++) {
        BACNET_DESTINATION *d1;
        BACNET_RECIPIENT *r1;
        d1 = &recipient_list[i];
        r1 = &d1->Recipient;
        same_element = false;
        for (j = 0; j < NC_MAX_RECIPIENTS; j++) {
            BACNET_DESTINATION *d2;
            BACNET_RECIPIENT *r2;
            d2 = &notification->Recipient_List[j];
            r2 = &d2->Recipient;
            if (bacnet_recipient_same(r1, r2)) {
                same_element = true;
            }
        }
        if (!same_element) {
            list_element->first_failed_element_number = 1 + i;
            list_element->error_class = ERROR_CLASS_SERVICES;
            list_element->error_code = ERROR_CODE_LIST_ELEMENT_NOT_FOUND;
            return BACNET_STATUS_ERROR;
        }
    }
    /* remove any matching elements */
    for (i = 0; i < remove_element_count; i++) {
        BACNET_DESTINATION *d1;
        BACNET_RECIPIENT *r1;
        d1 = &recipient_list[i];
        r1 = &d1->Recipient;
        for (j = 0; j < NC_MAX_RECIPIENTS; j++) {
            BACNET_DESTINATION *d2;
            BACNET_RECIPIENT *r2;
            d2 = &notification->Recipient_List[j];
            r2 = &d2->Recipient;
            if (bacnet_recipient_same(r1, r2)) {
                bacnet_destination_default_init(d2);
            }
        }
    }

    return BACNET_STATUS_OK;
}
#endif /* defined(INTRINSIC_REPORTING) */
