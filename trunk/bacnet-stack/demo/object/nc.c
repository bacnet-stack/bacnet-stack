/**************************************************************************
*
* Copyright (C) 2011 Krzysztof Malorny <malornykrzysztof@gmail.com>
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
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bacapp.h"
#include "config.h"
#include "device.h"
#include "handlers.h"
#include "wp.h"
#include "nc.h"

#include "datalink.h"       //for send who is
#include "whois.h"


#ifndef MAX_NOTIFICATION_CLASSES
#define MAX_NOTIFICATION_CLASSES 2
#endif


static NOTIFICATION_CLASS_INFO NC_Info[MAX_NOTIFICATION_CLASSES];

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Notification_Properties_Required[] =
{
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_NOTIFICATION_CLASS,
    PROP_PRIORITY,
    PROP_ACK_REQUIRED,
    PROP_RECIPIENT_LIST,
    -1
};

static const int Notification_Properties_Optional[] =
{
    PROP_DESCRIPTION,
    -1
};

static const int Notification_Properties_Proprietary[] =
{
    -1
};

void Notification_Class_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired)
        *pRequired = Notification_Properties_Required;
    if (pOptional)
        *pOptional = Notification_Properties_Optional;
    if (pProprietary)
        *pProprietary = Notification_Properties_Proprietary;
    return;
}

void Notification_Class_Init(void)
{
    uint8_t NotifyIdx = 0;

    for(NotifyIdx = 0; NotifyIdx < MAX_NOTIFICATION_CLASSES; NotifyIdx++)
    {
        /* init with zeros */
        memset(&NC_Info[NotifyIdx], 0x00, sizeof(NOTIFICATION_CLASS_INFO));
        /* set the basic parameters*/
        NC_Info[NotifyIdx].Ack_Required = 0;
        NC_Info[NotifyIdx].Priority[0] = 255;   // The lowest priority for Normal message.
        NC_Info[NotifyIdx].Priority[1] = 255;   // The lowest priority for Normal message.
        NC_Info[NotifyIdx].Priority[2] = 255;   // The lowest priority for Normal message.
    }

    /* init with special values for tests */
//    NC_Info[0].Ack_Required = TRANSITION_TO_NORMAL_MASKED;
//    NC_Info[0].Priority[0] = 248;
//    NC_Info[0].Priority[1] = 192;
//    NC_Info[0].Priority[2] = 200;
//    NC_Info[0].Recipient_List[0].ValidDays = 0x1F;  /* from monday to friday */
//    NC_Info[0].Recipient_List[0].FromTime.hour = 6;
//    NC_Info[0].Recipient_List[0].FromTime.min  = 15;
//    NC_Info[0].Recipient_List[0].FromTime.sec  = 10;
//    NC_Info[0].Recipient_List[0].FromTime.hundredths = 0;
//    NC_Info[0].Recipient_List[0].ToTime.hour = 23;
//    NC_Info[0].Recipient_List[0].ToTime.min  = 0;
//    NC_Info[0].Recipient_List[0].ToTime.sec  = 59;
//    NC_Info[0].Recipient_List[0].ToTime.hundredths = 0;
//    NC_Info[0].Recipient_List[0].Recipient.RecipientType = RECIPIENT_TYPE_ADDRESS;
//    NC_Info[0].Recipient_List[0].Recipient._.Address.mac[0] = 0xC0;
//    NC_Info[0].Recipient_List[0].Recipient._.Address.mac[1] = 0xA8;
//    NC_Info[0].Recipient_List[0].Recipient._.Address.mac[2] = 0x01;
//    NC_Info[0].Recipient_List[0].Recipient._.Address.mac[3] = 0x65;
//    NC_Info[0].Recipient_List[0].Recipient._.Address.mac[4] = 0xBA;
//    NC_Info[0].Recipient_List[0].Recipient._.Address.mac[5] = 0xC0;
//    NC_Info[0].Recipient_List[0].Recipient._.Address.mac_len = 6;
//    NC_Info[0].Recipient_List[0].ConfirmedNotify = true;
//    NC_Info[0].Recipient_List[0].Transitions =
//            TRANSITION_TO_OFFNORMAL_MASKED | TRANSITION_TO_NORMAL_MASKED;


    return;
}


/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Notification_Class_Valid_Instance(
        uint32_t object_instance)
{
    unsigned int index;

    index = Notification_Class_Instance_To_Index(object_instance);
    if (index < MAX_NOTIFICATION_CLASSES)
        return true;

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
unsigned Notification_Class_Instance_To_Index(
        uint32_t object_instance)
{
    unsigned index = MAX_NOTIFICATION_CLASSES;

    if (object_instance < MAX_NOTIFICATION_CLASSES)
        index = object_instance;

    return index;
}

bool Notification_Class_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING *object_name)
{
    static char text_string[32] = "";   /* okay for single thread */
    unsigned int index;
    bool status = false;

    index = Notification_Class_Instance_To_Index(object_instance);
    if (index < MAX_NOTIFICATION_CLASSES) {
        sprintf(text_string, "NOTIFICATION CLASS %lu", (unsigned long) index);
        status = characterstring_init_ansi(object_name, text_string);
    }

    return status;
}



int Notification_Class_Read_Property(
            BACNET_READ_PROPERTY_DATA * rpdata)
{
    NOTIFICATION_CLASS_INFO *CurrentNotify;
    BACNET_CHARACTER_STRING char_string;
    BACNET_OCTET_STRING octet_string;
    BACNET_BIT_STRING bit_string;
    uint8_t *apdu = NULL;
    uint8_t u8Val;
    int idx;
    int apdu_len = 0;           /* return value */


    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }

    apdu = rpdata->application_data;
    CurrentNotify = &NC_Info[Notification_Class_Instance_To_Index(rpdata->object_instance)];

    switch (rpdata->object_property)
    {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len =
                encode_application_object_id(&apdu[0], OBJECT_NOTIFICATION_CLASS,
                rpdata->object_instance);
            break;

        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
            Notification_Class_Object_Name(rpdata->object_instance, &char_string);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;

        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_NOTIFICATION_CLASS);
            break;

        case PROP_NOTIFICATION_CLASS:
            apdu_len += encode_application_unsigned(&apdu[0], rpdata->object_instance);
            break;

        case PROP_PRIORITY:
            if (rpdata->array_index == 0)
                apdu_len += encode_application_unsigned(&apdu[0], 3);
            else
            {
                if(rpdata->array_index == BACNET_ARRAY_ALL)
                {
                    /* TO-OFFNORMAL */
                    apdu_len += encode_application_unsigned(&apdu[apdu_len],
                                    CurrentNotify->Priority[0]);
                    /* TO-FAULT */
                    apdu_len += encode_application_unsigned(&apdu[apdu_len],
                                    CurrentNotify->Priority[1]);
                    /* TO-NORMAL */
                    apdu_len += encode_application_unsigned(&apdu[apdu_len],
                                    CurrentNotify->Priority[2]);
                }
                else if(rpdata->array_index <= 3)
                {
                    apdu_len += encode_application_unsigned(&apdu[apdu_len],
                                    CurrentNotify->Priority[rpdata->array_index - 1]);
                }
                else
                {
                    rpdata->error_class = ERROR_CLASS_PROPERTY;
                    rpdata->error_code  = ERROR_CODE_INVALID_ARRAY_INDEX;
                    apdu_len = -1;
                }
            }
            break;

        case PROP_ACK_REQUIRED:
            u8Val =  CurrentNotify->Ack_Required;

            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, TRANSITION_TO_OFFNORMAL,
                        (u8Val & TRANSITION_TO_OFFNORMAL_MASKED) ? true : false );
            bitstring_set_bit(&bit_string, TRANSITION_TO_FAULT,
                        (u8Val & TRANSITION_TO_FAULT_MASKED    ) ? true : false );
            bitstring_set_bit(&bit_string, TRANSITION_TO_NORMAL,
                        (u8Val & TRANSITION_TO_NORMAL_MASKED   ) ? true : false );
            /* encode bitstring */
            apdu_len += encode_application_bitstring(&apdu[apdu_len], &bit_string);
            break;

        case PROP_RECIPIENT_LIST:
            /* encode all entry of Recipient_List */
            for(idx = 0; idx < NC_MAX_RECIPIENTS; idx++)
            {
                BACNET_DESTINATION *RecipientEntry;
                int i = 0;

                /* get pointer of current element for Recipient_List  - easier for use */
                RecipientEntry = &CurrentNotify->Recipient_List[idx];
                if(RecipientEntry->Recipient.RecipientType != RECIPIENT_TYPE_NOTINITIALIZED)
                {
                    /* Valid Days - BACnetDaysOfWeek - [bitstring] monday-sunday */
                    u8Val = 0x01;
                    bitstring_init(&bit_string);

                    for(i = 0; i < MAX_BACNET_DAYS_OF_WEEK; i++)
                    {
                        if(RecipientEntry->ValidDays & u8Val)
                            bitstring_set_bit(&bit_string, i, true);
                        else
                            bitstring_set_bit(&bit_string, i, false);
                        u8Val <<= 1;    /* next day */
                    }
                    apdu_len += encode_application_bitstring(&apdu[apdu_len],&bit_string);

                    /* From Time */
                    apdu_len += encode_application_time(&apdu[apdu_len],
                                    &RecipientEntry->FromTime);

                    /* To Time */
                    apdu_len += encode_application_time(&apdu[apdu_len],
                                    &RecipientEntry->ToTime);

                    /*
                    BACnetRecipient ::= CHOICE {
                        device [0] BACnetObjectIdentifier,
                        address [1] BACnetAddress
                    } */

                    /* CHOICE - device [0] BACnetObjectIdentifier */
                    if(RecipientEntry->Recipient.RecipientType == RECIPIENT_TYPE_DEVICE)
                    {
                        apdu_len += encode_context_object_id(&apdu[apdu_len], 0, OBJECT_DEVICE,
                                        RecipientEntry->Recipient._.DeviceIdentifier);
                    }
                    /* CHOICE - address [1] BACnetAddress */
                    else if(RecipientEntry->Recipient.RecipientType == RECIPIENT_TYPE_ADDRESS)
                    {
                        /* opening tag 1 */
                        apdu_len += encode_opening_tag(&apdu[apdu_len], 1);
                        /* network-number Unsigned16, */
                        apdu_len += encode_application_unsigned(&apdu[apdu_len],
                                            RecipientEntry->Recipient._.Address.net);

                        /* mac-address OCTET STRING */
                        if (RecipientEntry->Recipient._.Address.net)
                        {
                            octetstring_init(&octet_string,
                                    RecipientEntry->Recipient._.Address.adr,
                                    RecipientEntry->Recipient._.Address.len);
                        }
                        else
                        {
                            octetstring_init(&octet_string,
                                    RecipientEntry->Recipient._.Address.mac,
                                    RecipientEntry->Recipient._.Address.mac_len);
                        }
                        apdu_len += encode_application_octet_string(&apdu[apdu_len], &octet_string);

                        /* closing tag 1 */
                        apdu_len += encode_closing_tag(&apdu[apdu_len], 1);

                    }
                    else {;}    /* shouldn't happen */

                    /* Process Identifier - Unsigned32 */
                    apdu_len += encode_application_unsigned(&apdu[apdu_len],
                                    RecipientEntry->ProcessIdentifier);

                    /* Issue Confirmed Notifications - boolean */
                    apdu_len += encode_application_boolean(&apdu[apdu_len],
                                    RecipientEntry->ConfirmedNotify);

                    /* Transitions - BACnet Event Transition Bits [bitstring] */
                    u8Val = RecipientEntry->Transitions;

                    bitstring_init(&bit_string);
                    bitstring_set_bit(&bit_string, TRANSITION_TO_OFFNORMAL,
                                (u8Val & TRANSITION_TO_OFFNORMAL_MASKED) ? true : false );
                    bitstring_set_bit(&bit_string, TRANSITION_TO_FAULT,
                                (u8Val & TRANSITION_TO_FAULT_MASKED    ) ? true : false );
                    bitstring_set_bit(&bit_string, TRANSITION_TO_NORMAL,
                                (u8Val & TRANSITION_TO_NORMAL_MASKED   ) ? true : false );

                    apdu_len += encode_application_bitstring(&apdu[apdu_len], &bit_string);
                }
            }
            break;

        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code  = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = -1;
            break;
    }

    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (rpdata->object_property != PROP_PRIORITY) &&
        (rpdata->array_index != BACNET_ARRAY_ALL))
    {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code  = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}



bool Notification_Class_Write_Property(
            BACNET_WRITE_PROPERTY_DATA * wp_data)
{
    NOTIFICATION_CLASS_INFO *CurrentNotify;
    NOTIFICATION_CLASS_INFO TmpNotify;
    BACNET_APPLICATION_DATA_VALUE value;
    bool status = false;
    int iOffset = 0;
    uint8_t idx = 0;
    int len = 0;



    CurrentNotify = &NC_Info[Notification_Class_Instance_To_Index(wp_data->object_instance)];

    // decode the some of the request
    len = bacapp_decode_application_data(wp_data->application_data,
                                         wp_data->application_data_len, &value);

    switch (wp_data->object_property)
    {
        case PROP_PRIORITY:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_UNSIGNED_INT,
                &wp_data->error_class, &wp_data->error_code);

            if(status) {
                if (wp_data->array_index == 0) {
                   wp_data->error_class = ERROR_CLASS_PROPERTY;
                   wp_data->error_code  = ERROR_CODE_INVALID_ARRAY_INDEX;
                }
                else if(wp_data->array_index == BACNET_ARRAY_ALL) {
                    /// FIXME: wite all array
                }
                else if(wp_data->array_index <= 3) {
                    CurrentNotify->Priority[wp_data->array_index - 1] =
                            value.type.Unsigned_Int;
                }
                else {
                   wp_data->error_class = ERROR_CLASS_PROPERTY;
                   wp_data->error_code  = ERROR_CODE_INVALID_ARRAY_INDEX;
                }
            }
            break;

        case PROP_ACK_REQUIRED:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_BIT_STRING,
                &wp_data->error_class, &wp_data->error_code);

            if (status) {
                if(value.type.Bit_String.bits_used == 3) {
                   CurrentNotify->Ack_Required =
                            value.type.Bit_String.value[0];
                }
                else {
                   wp_data->error_class = ERROR_CLASS_PROPERTY;
                   wp_data->error_code  = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;

        case PROP_RECIPIENT_LIST:

            memset(&TmpNotify, 0x00, sizeof(NOTIFICATION_CLASS_INFO));

            /* decode all packed */
            while (iOffset < wp_data->application_data_len)
            {
                /* Decode Valid Days */
                len = bacapp_decode_application_data(
                            &wp_data->application_data[iOffset],
                            wp_data->application_data_len, &value);

                if ((len == 0) || (value.tag != BACNET_APPLICATION_TAG_BIT_STRING)) {
                    /* Bad decode, wrong tag or following required parameter missing */
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code  = ERROR_CODE_INVALID_DATA_TYPE;
                    return false;
                }

                if (value.type.Bit_String.bits_used == MAX_BACNET_DAYS_OF_WEEK)
                    /* store value */
                    TmpNotify.Recipient_List[idx].ValidDays = value.type.Bit_String.value[0];
                else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code  = ERROR_CODE_OTHER;
                    return false;
                }

                iOffset += len;
                /* Decode From Time */
                len = bacapp_decode_application_data(
                            &wp_data->application_data[iOffset],
                            wp_data->application_data_len, &value);

                if ((len == 0) || (value.tag != BACNET_APPLICATION_TAG_TIME)) {
                    /* Bad decode, wrong tag or following required parameter missing */
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code  = ERROR_CODE_INVALID_DATA_TYPE;
                    return false;
                }
                /* store value */
                TmpNotify.Recipient_List[idx].FromTime = value.type.Time;

                iOffset += len;
                /* Decode To Time */
                len = bacapp_decode_application_data(
                            &wp_data->application_data[iOffset],
                            wp_data->application_data_len, &value);

                if ((len == 0) || (value.tag != BACNET_APPLICATION_TAG_TIME)) {
                    /* Bad decode, wrong tag or following required parameter missing */
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code  = ERROR_CODE_INVALID_DATA_TYPE;
                    return false;
                }
                /* store value */
                TmpNotify.Recipient_List[idx].ToTime = value.type.Time;

                iOffset += len;
                /* context tag [0] - Device */
                if(decode_is_context_tag(&wp_data->application_data[iOffset], 0))
                {
                    TmpNotify.Recipient_List[idx].Recipient.RecipientType = RECIPIENT_TYPE_DEVICE;
                    /* Decode Network Number */
                    len = bacapp_decode_context_data(
                                &wp_data->application_data[iOffset],
                                wp_data->application_data_len, &value,
                                PROP_RECIPIENT_LIST);

                    if ((len == 0) || (value.tag != BACNET_APPLICATION_TAG_OBJECT_ID)) {
                        /* Bad decode, wrong tag or following required parameter missing */
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code  = ERROR_CODE_INVALID_DATA_TYPE;
                        return false;
                    }
                    /* store value */
                    TmpNotify.Recipient_List[idx].Recipient._.DeviceIdentifier =
                            value.type.Object_Id.instance;

                    iOffset += len;
                }
                /* opening tag [1] - Recipient */
                else if(decode_is_opening_tag_number(&wp_data->application_data[iOffset], 1))
                {
                    iOffset++;
                    TmpNotify.Recipient_List[idx].Recipient.RecipientType = RECIPIENT_TYPE_ADDRESS;
                    /* Decode Network Number */
                    len = bacapp_decode_application_data(
                                &wp_data->application_data[iOffset],
                                wp_data->application_data_len, &value);

                    if ((len == 0) || (value.tag != BACNET_APPLICATION_TAG_UNSIGNED_INT)) {
                        /* Bad decode, wrong tag or following required parameter missing */
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code  = ERROR_CODE_INVALID_DATA_TYPE;
                        return false;
                    }
                    /* store value */
                    TmpNotify.Recipient_List[idx].Recipient._.Address.net = value.type.Unsigned_Int;

                    iOffset += len;
                    /* Decode Address */
                    len = bacapp_decode_application_data(
                                &wp_data->application_data[iOffset],
                                wp_data->application_data_len, &value);

                    if ((len == 0) || (value.tag != BACNET_APPLICATION_TAG_OCTET_STRING)) {
                        /* Bad decode, wrong tag or following required parameter missing */
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code  = ERROR_CODE_INVALID_DATA_TYPE;
                        return false;
                    }
                    /* store value */
                    if (TmpNotify.Recipient_List[idx].Recipient._.Address.net == 0) {
                        memcpy(TmpNotify.Recipient_List[idx].Recipient._.Address.mac,
                                    value.type.Octet_String.value,
                                    value.type.Octet_String.length);
                        TmpNotify.Recipient_List[idx].Recipient._.Address.mac_len =
                                value.type.Octet_String.length;
                    }
                    else {
                        memcpy(TmpNotify.Recipient_List[idx].Recipient._.Address.adr,
                                    value.type.Octet_String.value,
                                    value.type.Octet_String.length);
                        TmpNotify.Recipient_List[idx].Recipient._.Address.len =
                                value.type.Octet_String.length;
                    }

                    iOffset += len;
                    /* closing tag [1] - Recipient */
                    if(decode_is_closing_tag_number(&wp_data->application_data[iOffset], 1))
                        iOffset++;
                    else {
                        /* Bad decode, wrong tag or following required parameter missing */
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code  = ERROR_CODE_INVALID_DATA_TYPE;
                        return false;
                    }
                }
                else {
                    /* Bad decode, wrong tag or following required parameter missing */
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code  = ERROR_CODE_INVALID_DATA_TYPE;
                    return false;
                }

                /* Process Identifier */
                len = bacapp_decode_application_data(
                            &wp_data->application_data[iOffset],
                            wp_data->application_data_len, &value);

                if ((len == 0) || (value.tag != BACNET_APPLICATION_TAG_UNSIGNED_INT)) {
                    /* Bad decode, wrong tag or following required parameter missing */
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code  = ERROR_CODE_INVALID_DATA_TYPE;
                    return false;
                }
                /* store value */
                TmpNotify.Recipient_List[idx].ProcessIdentifier = value.type.Unsigned_Int;

                iOffset += len;
                /* Issue Confirmed Notifications */
                len = bacapp_decode_application_data(
                            &wp_data->application_data[iOffset],
                            wp_data->application_data_len, &value);

                if ((len == 0) || (value.tag != BACNET_APPLICATION_TAG_BOOLEAN)) {
                    /* Bad decode, wrong tag or following required parameter missing */
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code  = ERROR_CODE_INVALID_DATA_TYPE;
                    return false;
                }
                /* store value */
                TmpNotify.Recipient_List[idx].ConfirmedNotify = value.type.Boolean;

                iOffset += len;
                /* Transitions */
                len = bacapp_decode_application_data(
                            &wp_data->application_data[iOffset],
                            wp_data->application_data_len, &value);

                if ((len == 0) || (value.tag != BACNET_APPLICATION_TAG_BIT_STRING)) {
                    /* Bad decode, wrong tag or following required parameter missing */
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code  = ERROR_CODE_INVALID_DATA_TYPE;
                    return false;
                }

                if (value.type.Bit_String.bits_used == MAX_BACNET_EVENT_TRANSITION)
                    /* store value */
                    TmpNotify.Recipient_List[idx].Transitions = value.type.Bit_String.value[0];
                else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code  = ERROR_CODE_OTHER;
                    return false;
                }
                iOffset += len;

                /* Increasing element of list */
                if(++idx >= NC_MAX_RECIPIENTS) {
                    wp_data->error_class = ERROR_CLASS_RESOURCES;
                    wp_data->error_code  = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
                    return false;
                }
            }

            /* Decoded all recipient list */
            /* copy elements from temporary object */
            for(idx = 0; idx < NC_MAX_RECIPIENTS; idx++)
                CurrentNotify->Recipient_List[idx] = TmpNotify.Recipient_List[idx];

            status = true;

        break;

        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
    }

    return status;
}
