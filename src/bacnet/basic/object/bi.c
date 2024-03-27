 /**************************************************************************
 *
 * Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
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

/* Binary Input Objects customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacenum.h"
#include "bacnet/bactext.h"
#include "bacnet/bacapp.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#include "bacnet/cov.h"
#include "bacnet/config.h" /* the custom stuff */
#include "bacnet/basic/object/bi.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/object/device.h"

#ifndef MAX_BINARY_INPUTS
#define MAX_BINARY_INPUTS 5
#endif

/* stores the current value */
static BACNET_BINARY_PV Present_Value[MAX_BINARY_INPUTS];
/* Change of Value flag */
static bool Change_Of_Value[MAX_BINARY_INPUTS];
/* Polarity of Input */
static BACNET_POLARITY Polarity[MAX_BINARY_INPUTS];

typedef BACNET_CHARACTER_STRING BINARY_INPUT_CHARACTER_STRING;

typedef struct binary_input_descr {
  unsigned Event_State:3;
  uint32_t Instance;
  BINARY_INPUT_CHARACTER_STRING Name;
  BINARY_INPUT_CHARACTER_STRING Description;
  /* out of service decouples physical input from Present_Value */
  bool Out_Of_Service;

#if (BINARY_INPUT_INTRINSIC_REPORTING)
  uint32_t Time_Delay;
  uint32_t Notification_Class;
  unsigned Event_Enable:3;
  unsigned Event_Detection_Enable:1;
  unsigned Notify_Type:1;
  ACKED_INFO Acked_Transitions[MAX_BACNET_EVENT_TRANSITION];
  BACNET_DATE_TIME Event_Time_Stamps[MAX_BACNET_EVENT_TRANSITION];
  /* time to generate event notification */
  uint32_t Remaining_Time_Delay;
  /* AckNotification information */
  ACK_NOTIFICATION Ack_notify_data;
  BACNET_BINARY_PV Alarm_Value;
#endif
} BINARY_INPUT_DESCR;

static BINARY_INPUT_DESCR BI_Descr[MAX_BINARY_INPUTS];
static int BI_Max_Index = MAX_BINARY_INPUTS;

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Binary_Input_Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_PRESENT_VALUE, PROP_STATUS_FLAGS,
    PROP_EVENT_STATE, PROP_OUT_OF_SERVICE, PROP_POLARITY, -1 };

static const int Binary_Input_Properties_Optional[] = { PROP_DESCRIPTION,
#if (BINARY_INPUT_INTRINSIC_REPORTING)
    PROP_TIME_DELAY, PROP_NOTIFICATION_CLASS,
    PROP_ALARM_VALUE,
    PROP_EVENT_ENABLE, PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE, PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_DETECTION_ENABLE,
#endif
  -1 };

static const int Binary_Input_Properties_Proprietary[] = { -1 };

void Binary_Input_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired) {
        *pRequired = Binary_Input_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Binary_Input_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Binary_Input_Properties_Proprietary;
    }

    return;
}

/**
 * Validate whether the given instance exists in our table.
 *
 * @param object_instance Object instance
 *
 * @return true/false
 */
bool Binary_Input_Valid_Instance(uint32_t object_instance)
{
    unsigned int index;

    for (index = 0; index < BI_Max_Index; index++) {
        if (BI_Descr[index].Instance == object_instance) {
            return true;
        }
    }

    return false;
}


/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Binary_Input_Count(void)
{
    return BI_Max_Index;
}

/**
 * @brief Return the instance of an object indexed by index.
 *
 * @param index Object index
 *
 * @return Object instance
 */
uint32_t Binary_Input_Index_To_Instance(unsigned index)
{
    if (index < BI_Max_Index) {
        return BI_Descr[index].Instance;
    } else {
        PRINT("index out of bounds");
    }

    return 0;
}

void Binary_Input_Init(void)
{
  static bool initialized = false;
  unsigned i;

  if (!initialized) {
    initialized = true;

    /* initialize all the values */
    for (i = 0; i < BI_Max_Index; i++) {
#if (BINARY_INPUT_INTRINSIC_REPORTING)
      unsigned j;
#endif

      memset(&BI_Descr[i], 0x00, sizeof(BINARY_INPUT_DESCR));
      BI_Descr[i].Instance = BACNET_INSTANCE(BACNET_ID_VALUE(i, OBJECT_BINARY_INPUT));
      Present_Value[i] = BINARY_INACTIVE;
#if (BINARY_INPUT_INTRINSIC_REPORTING)
      BI_Descr[i].Event_State = EVENT_STATE_NORMAL;
      BI_Descr[i].Event_Detection_Enable = true;
      /* notification class not connected */
      BI_Descr[i].Notification_Class = BACNET_MAX_INSTANCE;
      /* initialize Event time stamps using wildcards and set Acked_transitions */
      for (j = 0; j < MAX_BACNET_EVENT_TRANSITION; j++) {
        datetime_wildcard_set(&BI_Descr[i].Event_Time_Stamps[j]);
        BI_Descr[i].Acked_Transitions[j].bIsAcked = true;
      }

      /* Set handler for GetEventInformation function */
      handler_get_event_information_set(
          OBJECT_BINARY_INPUT, Binary_Input_Event_Information);
      /* Set handler for AcknowledgeAlarm function */
      handler_alarm_ack_set(OBJECT_BINARY_INPUT, Binary_Input_Alarm_Ack);
      /* Set handler for GetAlarmSummary Service */
      handler_get_alarm_summary_set(
          OBJECT_BINARY_INPUT, Binary_Input_Alarm_Summary);
#endif
    }
  }

  return;
}

/**
 * Initialize the analog inputs. Returns false if there are errors.
 *
 * @param pInit_data pointer to initialisation values
 *
 * @return true/false
 */
bool Binary_Input_Set(BACNET_OBJECT_LIST_INIT_T *pInit_data)
{
  unsigned i;

  if (!pInit_data) {
    return false;
  }

  if ((int) pInit_data->length > MAX_BINARY_INPUTS) {
    PRINT("pInit_data->length = %d >= %d", (int) pInit_data->length, MAX_BINARY_INPUTS);
    return false;
  }

  for (i = 0; i < pInit_data->length; i++) {
    if (pInit_data->Object_Init_Values[i].Object_Instance < BACNET_MAX_INSTANCE) {
      BI_Descr[i].Instance = pInit_data->Object_Init_Values[i].Object_Instance;
    } else {
      PRINT("Object instance %u is too big", pInit_data->Object_Init_Values[i].Object_Instance);
      return false;
    }

    if (!characterstring_init_ansi(&BI_Descr[i].Name, pInit_data->Object_Init_Values[i].Object_Name)) {
      PRINT("Fail to set Object name to \".%128s\"", pInit_data->Object_Init_Values[i].Object_Name);
      return false;
    }

    if (!characterstring_init_ansi(&BI_Descr[i].Description, pInit_data->Object_Init_Values[i].Description)) {
      PRINT("Fail to set Object description to \".%128s\"", pInit_data->Object_Init_Values[i].Description);
      return false;
    }
  }

  BI_Max_Index = (int) pInit_data->length;

  return true;
}

/**
 * Return the index that corresponds to the object instance.
 *
 * @param instance Object Instance
 *
 * @return Object index
 */
unsigned Binary_Input_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = 0;

    for (; index < BI_Max_Index && BI_Descr[index].Instance != object_instance; index++) ;

    return index;
}

BACNET_BINARY_PV Binary_Input_Present_Value(uint32_t object_instance)
{
    BACNET_BINARY_PV value = BINARY_INACTIVE;
    unsigned index = 0;

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < BI_Max_Index) {
        value = Present_Value[index];
        if (Polarity[index] != POLARITY_NORMAL) {
            if (value == BINARY_INACTIVE) {
                value = BINARY_ACTIVE;
            } else {
                value = BINARY_INACTIVE;
            }
        }
    }

    return value;
}

bool Binary_Input_Out_Of_Service(uint32_t object_instance)
{
    bool value = false;
    unsigned index = 0;

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < BI_Max_Index) {
        value = BI_Descr[index].Out_Of_Service;
    }

    return value;
}

bool Binary_Input_Change_Of_Value(uint32_t object_instance)
{
    bool status = false;
    unsigned index;

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < BI_Max_Index) {
        status = Change_Of_Value[index];
    }

    return status;
}

void Binary_Input_Change_Of_Value_Clear(uint32_t object_instance)
{
    unsigned index;

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < BI_Max_Index) {
        Change_Of_Value[index] = false;
    }

    return;
}

/**
 * For a given object instance-number, loads the value_list with the COV data.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value_list - list of COV data
 *
 * @return  true if the value list is encoded
 */
bool Binary_Input_Encode_Value_List(
    uint32_t object_instance, BACNET_PROPERTY_VALUE *value_list)
{
    bool status = false;

    if (value_list) {
        value_list->propertyIdentifier = PROP_PRESENT_VALUE;
        value_list->propertyArrayIndex = BACNET_ARRAY_ALL;
        value_list->value.context_specific = false;
        value_list->value.tag = BACNET_APPLICATION_TAG_ENUMERATED;
        value_list->value.next = NULL;
        value_list->value.type.Enumerated =
            Binary_Input_Present_Value(object_instance);
        value_list->priority = BACNET_NO_PRIORITY;
        value_list = value_list->next;
    }
    if (value_list) {
        value_list->propertyIdentifier = PROP_STATUS_FLAGS;
        value_list->propertyArrayIndex = BACNET_ARRAY_ALL;
        value_list->value.context_specific = false;
        value_list->value.tag = BACNET_APPLICATION_TAG_BIT_STRING;
        value_list->value.next = NULL;
        bitstring_init(&value_list->value.type.Bit_String);
        bitstring_set_bit(
            &value_list->value.type.Bit_String, STATUS_FLAG_IN_ALARM, false);
        bitstring_set_bit(
            &value_list->value.type.Bit_String, STATUS_FLAG_FAULT, false);
        bitstring_set_bit(
            &value_list->value.type.Bit_String, STATUS_FLAG_OVERRIDDEN, false);
        if (Binary_Input_Out_Of_Service(object_instance)) {
            bitstring_set_bit(&value_list->value.type.Bit_String,
                STATUS_FLAG_OUT_OF_SERVICE, true);
        } else {
            bitstring_set_bit(&value_list->value.type.Bit_String,
                STATUS_FLAG_OUT_OF_SERVICE, false);
        }
        value_list->priority = BACNET_NO_PRIORITY;
        value_list->next = NULL;
        status = true;
    }

    return status;
}

bool Binary_Input_Present_Value_Set(
    uint32_t object_instance, BACNET_BINARY_PV value)
{
    unsigned index = 0;
    bool status = false;

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < BI_Max_Index) {
        if (Polarity[index] != POLARITY_NORMAL) {
            if (value == BINARY_INACTIVE) {
                value = BINARY_ACTIVE;
            } else {
                value = BINARY_INACTIVE;
            }
        }
        if (Present_Value[index] != value) {
            Change_Of_Value[index] = true;
        }
        Present_Value[index] = value;
        status = true;
    }

    return status;
}

void Binary_Input_Out_Of_Service_Set(uint32_t object_instance, bool value)
{
    unsigned index = 0;

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < BI_Max_Index) {
        if (BI_Descr[index].Out_Of_Service != value) {
            Change_Of_Value[index] = true;
        }
        BI_Descr[index].Out_Of_Service = value;
    }

    return;
}

/**
 * For a given object instance-number, return the name.
 *
 * Note: the object name must be unique within this device.
 *
 * @param  object_instance - object-instance number of the object
 * @param  object_name - object name pointer
 *
 * @return  true/false
 */
bool Binary_Input_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;
    unsigned index = 0;

    if (!object_name) {
        return false;
    }

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < BI_Max_Index) {
        *object_name = BI_Descr[index].Name;
        status = true;
    }


    return status;
}

/**
 * For a given object instance-number, return the description.
 *
 * Note: the object name must be unique within this device.
 *
 * @param  object_instance - object-instance number of the object
 * @param  description - description pointer
 *
 * @return  true/false
 */
bool Binary_Input_Description(
    uint32_t object_instance, BACNET_CHARACTER_STRING *description)
{
    bool status = false;
    unsigned index = 0;

    if (!description) {
        return false;
    }

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < BI_Max_Index) {
        *description = BI_Descr[index].Description;
        status = true;
    }

    return status;
}


BACNET_POLARITY Binary_Input_Polarity(uint32_t object_instance)
{
    BACNET_POLARITY polarity = POLARITY_NORMAL;
    unsigned index = 0;

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < BI_Max_Index) {
        polarity = Polarity[index];
    }

    return polarity;
}

bool Binary_Input_Polarity_Set(
    uint32_t object_instance, BACNET_POLARITY polarity)
{
    bool status = false;
    unsigned index = 0;

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < BI_Max_Index) {
        Polarity[index] = polarity;
    }

    return status;
}

/* return apdu length, or BACNET_STATUS_ERROR on error */
/* assumption - object already exists, and has been bounds checked */
int Binary_Input_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    unsigned object_index = 0;
    uint8_t *apdu = NULL;
    bool state = false;
#if (BINARY_INPUT_INTRINSIC_REPORTING)
    BINARY_INPUT_DESCR *CurrentBI = NULL;
#endif

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }

    object_index = Binary_Input_Instance_To_Index(rpdata->object_instance);
    if (object_index < BI_Max_Index) {
#if (BINARY_INPUT_INTRINSIC_REPORTING)
        CurrentBI = &BI_Descr[object_index];
#endif
    } else {
        return BACNET_STATUS_ERROR;
    }


    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_BINARY_INPUT, rpdata->object_instance);
            break;
        case PROP_OBJECT_NAME:
            if (Binary_Input_Object_Name(
                    rpdata->object_instance, &char_string)) {
                apdu_len =
                    encode_application_character_string(&apdu[0], &char_string);
            }
            break;

        case PROP_DESCRIPTION:
            if (Binary_Input_Description(
                    rpdata->object_instance, &char_string)) {
                apdu_len =
                    encode_application_character_string(&apdu[0], &char_string);
            }
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_BINARY_INPUT);
            break;
        case PROP_PRESENT_VALUE:
            /* note: you need to look up the actual value */
            apdu_len = encode_application_enumerated(
                &apdu[0], Binary_Input_Present_Value(rpdata->object_instance));
            break;
        case PROP_STATUS_FLAGS:
            /* note: see the details in the standard on how to use these */
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            state = Binary_Input_Out_Of_Service(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, state);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            apdu_len = encode_application_enumerated(
                &apdu[0], Binary_Input_Event_State(rpdata->object_instance));
            break;

        case PROP_OUT_OF_SERVICE:
            state = Binary_Input_Out_Of_Service(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_POLARITY:
            apdu_len = encode_application_enumerated(
                &apdu[0], Binary_Input_Polarity(rpdata->object_instance));
            break;
#if (BINARY_INPUT_INTRINSIC_REPORTING)
        case PROP_ALARM_VALUE:
            /* note: you need to look up the actual value */
            apdu_len = encode_application_enumerated(
                &apdu[0], CurrentBI->Alarm_Value);
            break;
        case PROP_TIME_DELAY:
            apdu_len =
                encode_application_unsigned(&apdu[0], CurrentBI->Time_Delay);
            break;

        case PROP_NOTIFICATION_CLASS:
            apdu_len = encode_application_unsigned(
                &apdu[0], CurrentBI->Notification_Class);
            break;

        case PROP_EVENT_ENABLE:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, TRANSITION_TO_OFFNORMAL,
                (CurrentBI->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ? true
                                                                      : false);
            bitstring_set_bit(&bit_string, TRANSITION_TO_FAULT,
                (CurrentBI->Event_Enable & EVENT_ENABLE_TO_FAULT) ? true
                                                                  : false);
            bitstring_set_bit(&bit_string, TRANSITION_TO_NORMAL,
                (CurrentBI->Event_Enable & EVENT_ENABLE_TO_NORMAL) ? true
                                                                   : false);

            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;

        case PROP_EVENT_DETECTION_ENABLE:
            apdu_len =
                encode_application_boolean(&apdu[0], CurrentBI->Event_Detection_Enable);
            break;

        case PROP_ACKED_TRANSITIONS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, TRANSITION_TO_OFFNORMAL,
                CurrentBI->Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked);
            bitstring_set_bit(&bit_string, TRANSITION_TO_FAULT,
                CurrentBI->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked);
            bitstring_set_bit(&bit_string, TRANSITION_TO_NORMAL,
                CurrentBI->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked);

            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;

        case PROP_NOTIFY_TYPE:
            apdu_len = encode_application_enumerated(
                &apdu[0], CurrentBI->Notify_Type ? NOTIFY_EVENT : NOTIFY_ALARM);
            break;

        case PROP_EVENT_TIME_STAMPS:
            /* Array element zero is the number of elements in the array */
            if (rpdata->array_index == 0)
                apdu_len = encode_application_unsigned(
                    &apdu[0], MAX_BACNET_EVENT_TRANSITION);
            /* if no index was specified, then try to encode the entire list */
            /* into one packet. */
            else if (rpdata->array_index == BACNET_ARRAY_ALL) {
              unsigned i = 0;
              int len = 0;

              for (i = 0; i < MAX_BACNET_EVENT_TRANSITION; i++) {
                len = encode_opening_tag(
                    &apdu[apdu_len], TIME_STAMP_DATETIME);
                len += encode_application_date(&apdu[apdu_len + len],
                    &CurrentBI->Event_Time_Stamps[i].date);
                len += encode_application_time(&apdu[apdu_len + len],
                    &CurrentBI->Event_Time_Stamps[i].time);
                len += encode_closing_tag(
                    &apdu[apdu_len + len], TIME_STAMP_DATETIME);

                /* add it if we have room */
                if ((apdu_len + len) < MAX_APDU)
                  apdu_len += len;
                else {
                  rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
                  apdu_len = BACNET_STATUS_ABORT;
                  break;
                }
              }
            } else if (rpdata->array_index <= MAX_BACNET_EVENT_TRANSITION) {
                apdu_len =
                    encode_opening_tag(&apdu[apdu_len], TIME_STAMP_DATETIME);
                apdu_len += encode_application_date(&apdu[apdu_len],
                    &CurrentBI->Event_Time_Stamps[rpdata->array_index].date);
                apdu_len += encode_application_time(&apdu[apdu_len],
                    &CurrentBI->Event_Time_Stamps[rpdata->array_index].time);
                apdu_len +=
                    encode_closing_tag(&apdu[apdu_len], TIME_STAMP_DATETIME);
            } else {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
                apdu_len = BACNET_STATUS_ERROR;
            }
            break;
#endif
        default:
            rpdata->error_class = ERROR_CLASS_PROPERTY;
            rpdata->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = BACNET_STATUS_ERROR;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) && (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/* returns true if successful */
bool Binary_Input_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    unsigned int object_index = 0;
    BACNET_APPLICATION_DATA_VALUE value;
#if (BINARY_INPUT_INTRINSIC_REPORTING)
    BINARY_INPUT_DESCR *CurrentBI = NULL;
#endif

    /* decode the some of the request */
    int len = bacapp_decode_application_data(
        wp_data->application_data, wp_data->application_data_len, &value);
    /* FIXME: len < application_data_len: more data? */
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    /*  only array properties can have array options */
    /*  only array properties can have array options */
    if ((wp_data->object_property != PROP_EVENT_TIME_STAMPS) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }
    object_index = Binary_Input_Instance_To_Index(wp_data->object_instance);
    if (object_index < BI_Max_Index) {
#if (BINARY_INPUT_INTRINSIC_REPORTING)
        CurrentBI = &BI_Descr[object_index];
#endif
    } else {
        return false;
    }

    switch (wp_data->object_property) {
#if (BINARY_INPUT_OUT_OF_SERVICE)
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Enumerated <= MAX_BINARY_PV) {
                    Binary_Input_Present_Value_Set(wp_data->object_instance,
                        (BACNET_BINARY_PV)value.type.Enumerated);
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Binary_Input_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
        case PROP_POLARITY:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Enumerated < MAX_POLARITY) {
                    Binary_Input_Polarity_Set(wp_data->object_instance,
                        (BACNET_POLARITY)value.type.Enumerated);
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
#endif
#if (BINARY_INPUT_INTRINSIC_REPORTING)
        case PROP_TIME_DELAY:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                CurrentBI->Time_Delay = value.type.Unsigned_Int;
                CurrentBI->Remaining_Time_Delay = CurrentBI->Time_Delay;
            }
            break;

        case PROP_NOTIFICATION_CLASS:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                CurrentBI->Notification_Class = value.type.Unsigned_Int;
            }
            break;

        case PROP_ALARM_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Enumerated <= MAX_BINARY_PV) {
                    Binary_Input_Alarm_Value_Set(wp_data->object_instance,
                        (BACNET_BINARY_PV) value.type.Enumerated);
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;

        case PROP_EVENT_ENABLE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BIT_STRING);
            if (status) {
                if (value.type.Bit_String.bits_used == 3) {
                    CurrentBI->Event_Enable = value.type.Bit_String.value[0];
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    status = false;
                }
            }
            break;

        case PROP_NOTIFY_TYPE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                switch ((BACNET_NOTIFY_TYPE)value.type.Enumerated) {
                    case NOTIFY_EVENT:
                        CurrentBI->Notify_Type = 1;
                        break;
                    case NOTIFY_ALARM:
                        CurrentBI->Notify_Type = 0;
                        break;
                    default:
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                        status = false;
                        break;
                }
            }
            break;
#endif
        case PROP_OBJECT_IDENTIFIER:
        case PROP_OBJECT_NAME:
        case PROP_DESCRIPTION:
        case PROP_OBJECT_TYPE:
        case PROP_STATUS_FLAGS:
        case PROP_EVENT_STATE:
#if !(BINARY_INPUT_OUT_OF_SERVICE)
        case PROP_PRESENT_VALUE:
        case PROP_OUT_OF_SERVICE:
        case PROP_POLARITY:
#endif
#if (BINARY_INPUT_INTRINSIC_REPORTING)
        case PROP_EVENT_DETECTION_ENABLE:
        case PROP_ACKED_TRANSITIONS:
        case PROP_EVENT_TIME_STAMPS:
#endif
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
        default:
            wp_data->error_class = ERROR_CLASS_PROPERTY;
            wp_data->error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            break;
    }

    return status;
}

/**
 * For a given object instance-number, gets the event-state property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  event-state property value
 */
unsigned Binary_Input_Event_State(uint32_t object_instance)
{
    unsigned state = EVENT_STATE_NORMAL;
#if !(BINARY_INPUT_INTRINSIC_REPORTING)
  (void) object_instance;
#endif
#if (BINARY_INPUT_INTRINSIC_REPORTING)
    unsigned index = Binary_Input_Instance_To_Index(object_instance);

    if (index < BI_Max_Index) {
        state = BI_Descr[index].Event_State;
    }
#endif

    return state;
}

#if (BINARY_INPUT_INTRINSIC_REPORTING)
/**
 * For a given object instance-number, gets the event-detection-enable property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  event-detection-enable property value
 */
bool Binary_Input_Event_Detection_Enable(uint32_t object_instance)
{
    bool retval = false;
#if !(BINARY_INPUT_INTRINSIC_REPORTING)
    (void) object_instance;
#endif
#if (BINARY_INPUT_INTRINSIC_REPORTING)
    unsigned index = Binary_Input_Instance_To_Index(object_instance);

    if (index < BI_Max_Index) {
        retval = BI_Descr[index].Event_Detection_Enable;
    }
#endif

    return retval;
}

/**
 * For a given object instance-number, sets the event-detection-enable property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  event-detection-enable property value
 */
bool Binary_Input_Event_Detection_Enable_Set(uint32_t object_instance, bool value)
{
    bool retval = false;
#if !(BINARY_INPUT_INTRINSIC_REPORTING)
    (void) object_instance;
    (void) value;
#endif
#if (BINARY_INPUT_INTRINSIC_REPORTING)
    unsigned index = Binary_Input_Instance_To_Index(object_instance);

    if (index < BI_Max_Index) {
        BI_Descr[index].Event_Detection_Enable = value;
        retval = true;
    }
#endif

    return retval;
}

int Binary_Input_Event_Information(
    unsigned index, BACNET_GET_EVENT_INFORMATION_DATA *getevent_data)
{
    bool IsNotAckedTransitions;
    bool IsActiveEvent;
    int i;

    /* check index */
    if (index < BI_Max_Index) {
        /* Event_State not equal to NORMAL */
        IsActiveEvent = (BI_Descr[index].Event_State != EVENT_STATE_NORMAL);

        /* Acked_Transitions property, which has at least one of the bits
           (TO-OFFNORMAL, TO-FAULT, TONORMAL) set to FALSE. */
        IsNotAckedTransitions =
            (BI_Descr[index]
                    .Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                    .bIsAcked == false) |
            (BI_Descr[index].Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked ==
                false) |
            (BI_Descr[index].Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked ==
                false);
    } else
        return -1; /* end of list  */

    if ((IsActiveEvent) || (IsNotAckedTransitions)) {
        /* Object Identifier */
        getevent_data->objectIdentifier.type = OBJECT_BINARY_INPUT;
        getevent_data->objectIdentifier.instance =
            Binary_Input_Index_To_Instance(index);
        /* Event State */
        getevent_data->eventState = BI_Descr[index].Event_State;
        /* Acknowledged Transitions */
        bitstring_init(&getevent_data->acknowledgedTransitions);
        bitstring_set_bit(&getevent_data->acknowledgedTransitions,
            TRANSITION_TO_OFFNORMAL,
            BI_Descr[index]
                .Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                .bIsAcked);
        bitstring_set_bit(&getevent_data->acknowledgedTransitions,
            TRANSITION_TO_FAULT,
            BI_Descr[index].Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked);
        bitstring_set_bit(&getevent_data->acknowledgedTransitions,
            TRANSITION_TO_NORMAL,
            BI_Descr[index].Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked);
        /* Event Time Stamps */
        for (i = 0; i < 3; i++) {
            getevent_data->eventTimeStamps[i].tag = TIME_STAMP_DATETIME;
            getevent_data->eventTimeStamps[i].value.dateTime =
                BI_Descr[index].Event_Time_Stamps[i];
        }
        /* Notify Type */
        getevent_data->notifyType = BI_Descr[index].Notify_Type;
        /* Event Enable */
        bitstring_init(&getevent_data->eventEnable);
        bitstring_set_bit(&getevent_data->eventEnable, TRANSITION_TO_OFFNORMAL,
            (BI_Descr[index].Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ? true
                                                                       : false);
        bitstring_set_bit(&getevent_data->eventEnable, TRANSITION_TO_FAULT,
            (BI_Descr[index].Event_Enable & EVENT_ENABLE_TO_FAULT) ? true
                                                                   : false);
        bitstring_set_bit(&getevent_data->eventEnable, TRANSITION_TO_NORMAL,
            (BI_Descr[index].Event_Enable & EVENT_ENABLE_TO_NORMAL) ? true
                                                                    : false);
        /* Event Priorities */
        Notification_Class_Get_Priorities(
            BI_Descr[index].Notification_Class, getevent_data->eventPriorities);

        return 1; /* active event */
    } else
        return 0; /* no active event at this index */
}


bool Binary_Input_Alarm_Value_Set(
    uint32_t object_instance, BACNET_BINARY_PV value)
{
    unsigned index = 0;
    bool status = false;

    index = Binary_Input_Instance_To_Index(object_instance);
    if (index < BI_Max_Index) {
        if (Polarity[index] != POLARITY_NORMAL) {
          value = (value == BINARY_INACTIVE) ? BINARY_ACTIVE : BINARY_INACTIVE;
        }
        BI_Descr[index].Alarm_Value = value;
        status = true;
    }

    return status;
}
#endif /* (INTRINSIC_REPORTING) */

void Binary_Input_Intrinsic_Reporting(uint32_t object_instance)
{
#if !(BINARY_INPUT_INTRINSIC_REPORTING)
  (void) object_instance;
#endif
#if (BINARY_INPUT_INTRINSIC_REPORTING)
    BACNET_EVENT_NOTIFICATION_DATA event_data = { 0 };
    BACNET_CHARACTER_STRING msgText = { 0 };
    BINARY_INPUT_DESCR *CurrentBI = NULL;
    unsigned int object_index = 0;
    uint8_t FromState = 0;
    uint8_t ToState = 0;
    BACNET_BINARY_PV Alarm_Value = BINARY_INACTIVE;
    BACNET_BINARY_PV PresentVal  = BINARY_INACTIVE;
    bool SendNotify = false;

    object_index = Binary_Input_Instance_To_Index(object_instance);
    if (object_index < BI_Max_Index) {
        CurrentBI = &BI_Descr[object_index];
    } else {
        return;
    }
    /* check whether Intrinsic reporting is enabled*/
    if (!CurrentBI->Event_Detection_Enable) {
        return; /* limits are not configured */
    }

    if (CurrentBI->Ack_notify_data.bSendAckNotify) {
        /* clean bSendAckNotify flag */
        CurrentBI->Ack_notify_data.bSendAckNotify = false;
        /* copy toState */
        ToState = CurrentBI->Ack_notify_data.EventState;
        PRINT("Binary-Input[%d]: Send AckNotification.\n", object_instance);
        characterstring_init_ansi(&msgText, "AckNotification");

        /* Notify Type */
        event_data.notifyType = NOTIFY_ACK_NOTIFICATION;

        /* Send EventNotification. */
        SendNotify = true;
    } else {
        /* actual Present_Value */
        PresentVal = Binary_Input_Present_Value(object_instance);
        FromState = CurrentBI->Event_State;
        switch (CurrentBI->Event_State) {
            case EVENT_STATE_NORMAL:
                /* (a) If pCurrentState is NORMAL, and pMonitoredValue is equal to any of the values contained in pAlarmValues for
                       pTimeDelay, then indicate a transition to the OFFNORMAL event state.
                */
                if ((PresentVal == CurrentBI->Alarm_Value) &&
                    ((CurrentBI->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ==
                        EVENT_ENABLE_TO_OFFNORMAL)) {
                    if (!CurrentBI->Remaining_Time_Delay)
                        CurrentBI->Event_State = EVENT_STATE_OFFNORMAL;
                    else
                        CurrentBI->Remaining_Time_Delay--;
                    break;
                }

                /* value of the object is still in the same event state */
                CurrentBI->Remaining_Time_Delay = CurrentBI->Time_Delay;
                break;

            case EVENT_STATE_OFFNORMAL:
                /* (b) If pCurrentState is OFFNORMAL, and pMonitoredValue is not equal to any of the values contained in pAlarmValues
                       for pTimeDelayNormal, then indicate a transition to the NORMAL event state.
                */
                if ((PresentVal != CurrentBI->Alarm_Value) &&
                    ((CurrentBI->Event_Enable & EVENT_ENABLE_TO_NORMAL) ==
                        EVENT_ENABLE_TO_NORMAL)) {
                    if (!CurrentBI->Remaining_Time_Delay)
                        CurrentBI->Event_State = EVENT_STATE_NORMAL;
                    else
                        CurrentBI->Remaining_Time_Delay--;
                    break;
                }

                /* value of the object is still in the same event state */
                CurrentBI->Remaining_Time_Delay = CurrentBI->Time_Delay;
                break;

            default:
                return; /* shouldn't happen */
        } /* switch (FromState) */

        ToState = CurrentBI->Event_State;

        if (FromState != ToState) {
            /* Event_State has changed.
               Need to fill only the basic parameters of this type of event.
               Other parameters will be filled in common function. */

            switch (ToState) {
                case EVENT_STATE_NORMAL:
                    characterstring_init_ansi( &msgText, "Back to normal state from off-normal");
                    break;

                case EVENT_STATE_OFFNORMAL:
                    characterstring_init_ansi( &msgText, "Back to off-normal state from normal");
                    break;

                default:
                    break;
            } /* switch (ToState) */
            PRINT("Binary-Input[%d]: Event_State goes from %.128s to %.128s.\n",
                object_instance, bactext_event_state_name(FromState),
                bactext_event_state_name(ToState));
            /* Notify Type */
            event_data.notifyType = CurrentBI->Notify_Type;

            /* Send EventNotification. */
            SendNotify = true;
        }
    }

    if (SendNotify) {
        /* Event Object Identifier */
        event_data.eventObjectIdentifier.type = OBJECT_BINARY_INPUT;
        event_data.eventObjectIdentifier.instance = object_instance;

        /* Time Stamp */
        event_data.timeStamp.tag = TIME_STAMP_DATETIME;
        if (event_data.notifyType != NOTIFY_ACK_NOTIFICATION) {
            Device_getCurrentDateTime(&event_data.timeStamp.value.dateTime);
            /* fill Event_Time_Stamps */
            switch (ToState) {
                case EVENT_STATE_OFFNORMAL:
                    datetime_copy(
                        &CurrentBI->Event_Time_Stamps[TRANSITION_TO_OFFNORMAL],
                        &event_data.timeStamp.value.dateTime);
                    break;
                case EVENT_STATE_FAULT:
                    datetime_copy(
                        &CurrentBI->Event_Time_Stamps[TRANSITION_TO_FAULT],
                        &event_data.timeStamp.value.dateTime);
                    break;
                case EVENT_STATE_NORMAL:
                    datetime_copy(
                        &CurrentBI->Event_Time_Stamps[TRANSITION_TO_NORMAL],
                        &event_data.timeStamp.value.dateTime);
                    break;
                default:
                    break;
            }
        } else {
            /* fill event_data timeStamp */
            switch (ToState) {
                case EVENT_STATE_FAULT:
                    datetime_copy(&event_data.timeStamp.value.dateTime,
                        &CurrentBI->Event_Time_Stamps[TRANSITION_TO_FAULT]);
                    break;
                case EVENT_STATE_NORMAL:
                    datetime_copy(&event_data.timeStamp.value.dateTime,
                        &CurrentBI->Event_Time_Stamps[TRANSITION_TO_NORMAL]);
                    break;
                case EVENT_STATE_OFFNORMAL:
                    datetime_copy(&event_data.timeStamp.value.dateTime,
                        &CurrentBI->Event_Time_Stamps[TRANSITION_TO_OFFNORMAL]);
                    break;
                default:
                    break;
            }
        }

        /* Notification Class */
        event_data.notificationClass = CurrentBI->Notification_Class;

        /* Event Type */
        event_data.eventType = EVENT_CHANGE_OF_STATE;

        /* Message Text */
        event_data.messageText = &msgText;

        /* Notify Type */
        /* filled before */

        /* From State */
        if (event_data.notifyType != NOTIFY_ACK_NOTIFICATION)
            event_data.fromState = FromState;

        /* To State */
        event_data.toState = CurrentBI->Event_State;

        /* Event Values */
        if (event_data.notifyType != NOTIFY_ACK_NOTIFICATION) {
            /* Value that exceeded a limit. */
            event_data.notificationParams.changeOfState.newState =
                (BACNET_PROPERTY_STATE) { .tag = BINARY_VALUE, .state = { .binaryValue = Present_Value[object_index] } };
            /* Status_Flags of the referenced object. */
            bitstring_init(
                &event_data.notificationParams.changeOfState.statusFlags);
            bitstring_set_bit(
                &event_data.notificationParams.changeOfState.statusFlags,
                STATUS_FLAG_IN_ALARM,
                CurrentBI->Event_State != EVENT_STATE_NORMAL);
            bitstring_set_bit(
                &event_data.notificationParams.changeOfState.statusFlags,
                STATUS_FLAG_FAULT, false);
            bitstring_set_bit(
                &event_data.notificationParams.changeOfState.statusFlags,
                STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(
                &event_data.notificationParams.changeOfState.statusFlags,
                STATUS_FLAG_OUT_OF_SERVICE, CurrentBI->Out_Of_Service);
        }

        /* add data from notification class */
        PRINT("Binary-Input[%d]: Notification Class[%d]-%s "
               "%u/%u/%u-%u:%u:%u.%u!\n",
            object_instance, event_data.notificationClass,
            bactext_event_type_name(event_data.eventType),
            (unsigned)event_data.timeStamp.value.dateTime.date.year,
            (unsigned)event_data.timeStamp.value.dateTime.date.month,
            (unsigned)event_data.timeStamp.value.dateTime.date.day,
            (unsigned)event_data.timeStamp.value.dateTime.time.hour,
            (unsigned)event_data.timeStamp.value.dateTime.time.min,
            (unsigned)event_data.timeStamp.value.dateTime.time.sec,
            (unsigned)event_data.timeStamp.value.dateTime.time.hundredths);
        Notification_Class_common_reporting_function(&event_data);

        /* Ack required */
        if ((event_data.notifyType != NOTIFY_ACK_NOTIFICATION) &&
            (event_data.ackRequired == true)) {
            PRINT("Binary-Input[%d]: Ack Required!\n", object_instance);
            switch (event_data.toState) {
                case EVENT_STATE_OFFNORMAL:
                    CurrentBI->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                        .bIsAcked = false;
                    CurrentBI->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                        .Time_Stamp = event_data.timeStamp.value.dateTime;
                    break;

                case EVENT_STATE_FAULT:
                    CurrentBI->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked =
                        false;
                    CurrentBI->Acked_Transitions[TRANSITION_TO_FAULT]
                        .Time_Stamp = event_data.timeStamp.value.dateTime;
                    break;

                case EVENT_STATE_NORMAL:
                    CurrentBI->Acked_Transitions[TRANSITION_TO_NORMAL]
                        .bIsAcked = false;
                    CurrentBI->Acked_Transitions[TRANSITION_TO_NORMAL]
                        .Time_Stamp = event_data.timeStamp.value.dateTime;
                    break;

                default: /* shouldn't happen */
                    break;
            }
        }
    }
#endif /* (BINARY_INPUT_INTRINSIC_REPORTING) */
}

