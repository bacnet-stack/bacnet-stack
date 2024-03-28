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

/* Binary Output Objects - customize for your use */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacenum.h"
#include "bacnet/bactext.h"
#include "bacnet/bacapp.h"
#include "bacnet/config.h" /* the custom stuff */
#include "bacnet/wp.h"
#include "bacnet/rp.h"
#include "bacnet/basic/object/bv.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/object/device.h"

#include "bacnet/basic/sys/debug.h"

#ifndef MAX_BINARY_VALUES
#define MAX_BINARY_VALUES 10
#endif

#define PRINTF printf

/* When all the priorities are level null, the present value returns */
/* the Relinquish Default value */
#define RELINQUISH_DEFAULT BINARY_INACTIVE
/* Here is our Priority Array.*/
static BACNET_BINARY_PV Binary_Value_Level[MAX_BINARY_VALUES]
                                          [BACNET_MAX_PRIORITY];

typedef struct binary_value_descr {
  unsigned Event_State:3;
  uint32_t Instance;
  BACNET_CHARACTER_STRING Name;
  BACNET_CHARACTER_STRING Description;
  bool Change_Of_Value;
  /* Writable out-of-service allows others to play with our Present Value */
  /* without changing the physical output */
  bool Out_Of_Service;

#if (BINARY_VALUE_INTRINSIC_REPORTING)
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
} BINARY_VALUE_DESCR;

static BINARY_VALUE_DESCR BV_Descr[MAX_BINARY_VALUES];
static int BV_Max_Index = MAX_BINARY_VALUES;

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Binary_Value_Properties_Required[] = { PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME, PROP_OBJECT_TYPE, PROP_PRESENT_VALUE, PROP_STATUS_FLAGS,
    PROP_EVENT_STATE, PROP_OUT_OF_SERVICE, -1 };

static const int Binary_Value_Properties_Optional[] = {
    PROP_DESCRIPTION,
#if (BINARY_VALUE_COMMANDABLE_PV)
    PROP_PRIORITY_ARRAY, PROP_RELINQUISH_DEFAULT,
#endif
#if (BINARY_VALUE_INTRINSIC_REPORTING)
    PROP_TIME_DELAY, PROP_NOTIFICATION_CLASS,
    PROP_ALARM_VALUE,
    PROP_EVENT_ENABLE, PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE, PROP_EVENT_TIME_STAMPS,
    PROP_EVENT_DETECTION_ENABLE,
#endif
-1 };

static const int Binary_Value_Properties_Proprietary[] = { -1 };

/**
 * Initialize the pointers for the required, the optional and the properitary
 * value properties.
 *
 * @param pRequired - Pointer to the pointer of required values.
 * @param pOptional - Pointer to the pointer of optional values.
 * @param pProprietary - Pointer to the pointer of properitary values.
 */
void Binary_Value_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary)
{
    if (pRequired) {
        *pRequired = Binary_Value_Properties_Required;
    }
    if (pOptional) {
        *pOptional = Binary_Value_Properties_Optional;
    }
    if (pProprietary) {
        *pProprietary = Binary_Value_Properties_Proprietary;
    }

    return;
}

/**
 * Initialize the binary values.
 */
void Binary_Value_Init(void)
{
    unsigned i, j;
    static bool initialized = false;

    if (!initialized) {
        initialized = true;

        /* initialize all the binary value priority arrays to NULL */
        for (i = 0; i < MAX_BINARY_VALUES; i++) {
          memset(&BV_Descr[i], 0x00, sizeof(BINARY_VALUE_DESCR));
          BV_Descr[i].Instance = BACNET_INSTANCE(BACNET_ID_VALUE(i, OBJECT_BINARY_VALUE));
          BV_Descr[i].Change_Of_Value = false;

          for (j = 0; j < BACNET_MAX_PRIORITY; j++) {
            Binary_Value_Level[i][j] = BINARY_NULL;
          }

#if (BINARY_VALUE_INTRINSIC_REPORTING)
          BV_Descr[i].Event_State = EVENT_STATE_NORMAL;
          BV_Descr[i].Event_Detection_Enable = true;
          /* notification class not connected */
          BV_Descr[i].Notification_Class = BACNET_MAX_INSTANCE;
          /* initialize Event time stamps using wildcards and set Acked_transitions */
          for (j = 0; j < MAX_BACNET_EVENT_TRANSITION; j++) {
            datetime_wildcard_set(&BV_Descr[i].Event_Time_Stamps[j]);
            BV_Descr[i].Acked_Transitions[j].bIsAcked = true;
          }

          /* Set handler for GetEventInformation function */
          handler_get_event_information_set(
              OBJECT_BINARY_VALUE, Binary_Value_Event_Information);
          /* Set handler for AcknowledgeAlarm function */
          handler_alarm_ack_set(OBJECT_BINARY_VALUE, Binary_Value_Alarm_Ack);
          /* Set handler for GetAlarmSummary Service */
          handler_get_alarm_summary_set(
              OBJECT_BINARY_VALUE, Binary_Value_Alarm_Summary);
#endif
        }
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
bool Binary_Value_Valid_Instance(uint32_t object_instance)
{
    unsigned int index;

    for (index = 0; index < BV_Max_Index; index++) {
        if (BV_Descr[index].Instance == object_instance) {
            return true;
        }
    }

    return false;
}


/**
 * Return the count of analog values.
 *
 * @return Count of binary values.
 */
unsigned Binary_Value_Count(void)
{
    return BV_Max_Index;
}

/**
 * @brief Return the instance of an object indexed by index.
 *
 * @param index Object index
 *
 * @return Object instance
 */
uint32_t Binary_Value_Index_To_Instance(unsigned index)
{
    if (index < BV_Max_Index) {
        return BV_Descr[index].Instance;
    } else {
        PRINT("index %u out of bounds", index);
    }

    return 0;
}

/**
 * Initialize the analog inputs. Returns false if there are errors.
 *
 * @param pInit_data pointer to initialisation values
 *
 * @return true/false
 */
bool Binary_Value_Set(BACNET_OBJECT_LIST_INIT_T *pInit_data)
{
  unsigned i;

  if (!pInit_data) {
    return false;
  }

  if ((int) pInit_data->length > MAX_BINARY_VALUES) {
    PRINTF("pInit_data->length = %d > %d", (int) pInit_data->length, MAX_BINARY_VALUES);
    return false;
  }

  for (i = 0; i < pInit_data->length; i++) {
    if (pInit_data->Object_Init_Values[i].Object_Instance < BACNET_MAX_INSTANCE) {
      BV_Descr[i].Instance = pInit_data->Object_Init_Values[i].Object_Instance;
    } else {
      PRINTF("Object instance %u is too big", pInit_data->Object_Init_Values[i].Object_Instance);
      return false;
    }

    if (!characterstring_init_ansi(&BV_Descr[i].Name, pInit_data->Object_Init_Values[i].Object_Name)) {
      PRINTF("Fail to set Object name to \"%128s\"", pInit_data->Object_Init_Values[i].Object_Name);
      return false;
    }

    if (!characterstring_init_ansi(&BV_Descr[i].Description, pInit_data->Object_Init_Values[i].Description)) {
      PRINTF("Fail to set Object description to \"%128s\"", pInit_data->Object_Init_Values[i].Description);
      return false;
    }
  }

  BV_Max_Index = (int) pInit_data->length;

  return true;
}

/**
 * Return the index that corresponds to the object instance.
 *
 * @param instance Object Instance
 *
 * @return Object index
 */
unsigned Binary_Value_Instance_To_Index(uint32_t object_instance)
{
    unsigned index = 0;

    for (; index < BV_Max_Index && BV_Descr[index].Instance != object_instance; index++) ;

    return index;
}

/**
 * For a given object instance-number, return the present value.
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  Present value
 */
BACNET_BINARY_PV Binary_Value_Present_Value(uint32_t object_instance)
{
    BACNET_BINARY_PV value = RELINQUISH_DEFAULT;
    unsigned index = 0;
    unsigned i = 0;

    index = Binary_Value_Instance_To_Index(object_instance);
    if (index < MAX_BINARY_VALUES) {
        for (i = 0; i < BACNET_MAX_PRIORITY; i++) {
          if (Binary_Value_Level[index][i] != BINARY_NULL) {
            value = Binary_Value_Level[index][i];
            break;
          }
        }
    }

    return value;
}

/**
 * For a given object instance-number, sets the present-value at a given
 * priority 1..16.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value - BACNET_BINARY_PV binary value translated to either 0 or 1
 * @param  priority - priority 1..16
 *
 * @return  true if values are within range and present-value is set.
 */
bool Binary_Value_Present_Value_Set(
    uint32_t object_instance, BACNET_BINARY_PV value, uint8_t priority)
{
    bool status = false;

    if((priority > 0) && (priority <= BACNET_MAX_PRIORITY)) {
      unsigned index = Binary_Value_Instance_To_Index(object_instance);

      if (index < BV_Max_Index) {
        if (Binary_Value_Level[index][priority-1] != value) {
          BV_Descr[index].Change_Of_Value = true;
        }
        Binary_Value_Level[index][priority-1] = value;
        status = true;
      }
    }

    return status;
}

bool Binary_Value_Change_Of_Value(uint32_t object_instance)
{
    bool status = false;
    unsigned index = Binary_Value_Instance_To_Index(object_instance);

    if (index < BV_Max_Index) {
        status = BV_Descr[index].Change_Of_Value;
    }

    return status;
}

void Binary_Value_Change_Of_Value_Clear(uint32_t object_instance)
{
    unsigned index = Binary_Value_Instance_To_Index(object_instance);

    if (index < BV_Max_Index) {
        BV_Descr[index].Change_Of_Value = false;
    }
}

/**
 * @brief Encode a BACnetARRAY property element
 * @param object_instance [in] BACnet network port object instance number
 * @param priority [in] array index requested:
 *    0 to N for individual array members
 * @param apdu [out] Buffer in which the APDU contents are built, or NULL to
 * return the length of buffer if it had been built
 * @return The length of the apdu encoded or
 *   BACNET_STATUS_ERROR for ERROR_CODE_INVALID_ARRAY_INDEX
 */
static int Binary_Value_Priority_Array_Encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX priority, uint8_t *apdu)
{
    int apdu_len = BACNET_STATUS_ERROR;
    BACNET_BINARY_PV value = RELINQUISH_DEFAULT;
    unsigned index = 0;

    index = Binary_Value_Instance_To_Index(object_instance);
    if ((index < BV_Max_Index) && (priority < BACNET_MAX_PRIORITY)) {
        if (Binary_Value_Level[index][priority] != BINARY_NULL) {
            apdu_len = encode_application_null(apdu);
        } else {
            value = Binary_Value_Level[index][priority];
            apdu_len = encode_application_enumerated(apdu, value);
        }
    }

    return apdu_len;
}

/**
 * For a given object instance-number, loads the value_list with the COV data.
 *
 * @param  object_instance - object-instance number of the object
 * @param  value_list - list of COV data
 *
 * @return  true if the value list is encoded
 */
bool Binary_Value_Encode_Value_List(
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
            Binary_Value_Present_Value(object_instance);
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
        if (Binary_Value_Out_Of_Service(object_instance)) {
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
bool Binary_Value_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name)
{
    bool status = false;
    unsigned index = 0;

    if (!object_name) {
        return false;
    }

    index = Binary_Value_Instance_To_Index(object_instance);
    if (index < BV_Max_Index) {
        *object_name = BV_Descr[index].Name;
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
bool Binary_Value_Description(
    uint32_t object_instance, BACNET_CHARACTER_STRING *description)
{
    bool status = false;
    unsigned index = 0;

    if (!description) {
        return false;
    }

    index = Binary_Value_Instance_To_Index(object_instance);
    if (index < BV_Max_Index) {
        *description = BV_Descr[index].Description;
        status = true;
    }

    return status;
}

/**
 * Return the OOO value, if any.
 *
 * @param instance Object instance.
 *
 * @return true/false
 */
bool Binary_Value_Out_Of_Service(uint32_t instance)
{
    unsigned index = 0;
    bool oos_flag = false;

    index = Binary_Value_Instance_To_Index(instance);
    if (index < BV_Max_Index) {
        oos_flag = BV_Descr[index].Out_Of_Service;
    }

    return oos_flag;
}

/**
 * Set the OOO value, if any.
 *
 * @param instance Object instance.
 * @param oos_flag New OOO value.
 */
void Binary_Value_Out_Of_Service_Set(uint32_t instance, bool oos_flag)
{
    unsigned index = 0;

    index = Binary_Value_Instance_To_Index(instance);
    if (index < BV_Max_Index) {
        BV_Descr[index].Out_Of_Service = oos_flag;
    }
}

/**
 * Return the requested property of the binary value.
 *
 * @param rpdata  Property requested, see for BACNET_READ_PROPERTY_DATA details.
 *
 * @return apdu len, or BACNET_STATUS_ERROR on error
 */
int Binary_Value_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    int apdu_len = 0; /* return value */
    int apdu_size = 0;
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    BACNET_BINARY_PV present_value = BINARY_INACTIVE;
    unsigned object_index = 0;
    bool state = false;
    uint8_t *apdu = NULL;
#if (BINARY_VALUE_INTRINSIC_REPORTING)
    BINARY_VALUE_DESCR *CurrentBV = NULL;
#endif

    /* Valid data? */
    if (rpdata == NULL) {
        return 0;
    }
    if ((rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }

    /* Valid object index? */
    object_index = Binary_Value_Instance_To_Index(rpdata->object_instance);
    if (object_index >= BV_Max_Index) {
        rpdata->error_class = ERROR_CLASS_OBJECT;
        rpdata->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return BACNET_STATUS_ERROR;
    }

#if (BINARY_VALUE_INTRINSIC_REPORTING)
        CurrentBV = &BV_Descr[object_index];
#endif

    apdu = rpdata->application_data;
    apdu_size = rpdata->application_data_len;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = encode_application_object_id(
                &apdu[0], OBJECT_BINARY_VALUE, rpdata->object_instance);
            break;
            /* note: Name and Description don't have to be the same.
               You could make Description writable and different */
        case PROP_OBJECT_NAME:
            if (Binary_Value_Object_Name(
                    rpdata->object_instance, &char_string)) {
                apdu_len =
                    encode_application_character_string(&apdu[0], &char_string);
            }
            break;
        case PROP_DESCRIPTION:
            if (Binary_Value_Description(
                    rpdata->object_instance, &char_string)) {
                apdu_len =
                    encode_application_character_string(&apdu[0], &char_string);
            }
            break;
        case PROP_OBJECT_TYPE:
            apdu_len =
                encode_application_enumerated(&apdu[0], OBJECT_BINARY_VALUE);
            break;
        case PROP_PRESENT_VALUE:
            present_value = Binary_Value_Present_Value(rpdata->object_instance);
            apdu_len = encode_application_enumerated(&apdu[0], present_value);
            break;
        case PROP_STATUS_FLAGS:
            /* note: see the details in the standard on how to use these */
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            state = Binary_Value_Out_Of_Service(rpdata->object_instance);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE, state);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;
        case PROP_EVENT_STATE:
            /* note: see the details in the standard on how to use this */
            apdu_len =
                encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;
        case PROP_OUT_OF_SERVICE:
            state = Binary_Value_Out_Of_Service(rpdata->object_instance);
            apdu_len = encode_application_boolean(&apdu[0], state);
            break;
        case PROP_PRIORITY_ARRAY:
            apdu_len = bacnet_array_encode(rpdata->object_instance,
                rpdata->array_index, Binary_Value_Priority_Array_Encode,
                BACNET_MAX_PRIORITY, apdu, apdu_size);
            if (apdu_len == BACNET_STATUS_ABORT) {
                rpdata->error_code =
                    ERROR_CODE_ABORT_SEGMENTATION_NOT_SUPPORTED;
            } else if (apdu_len == BACNET_STATUS_ERROR) {
                rpdata->error_class = ERROR_CLASS_PROPERTY;
                rpdata->error_code = ERROR_CODE_INVALID_ARRAY_INDEX;
            }
            break;
        case PROP_RELINQUISH_DEFAULT:
            present_value = RELINQUISH_DEFAULT;
            apdu_len = encode_application_enumerated(&apdu[0], present_value);
            break;
#if (BINARY_VALUE_INTRINSIC_REPORTING)
        case PROP_ALARM_VALUE:
            /* note: you need to look up the actual value */
            apdu_len = encode_application_enumerated(
                &apdu[0], CurrentBV->Alarm_Value);
            break;
        case PROP_TIME_DELAY:
            apdu_len =
                encode_application_unsigned(&apdu[0], CurrentBV->Time_Delay);
            break;

        case PROP_NOTIFICATION_CLASS:
            apdu_len = encode_application_unsigned(
                &apdu[0], CurrentBV->Notification_Class);
            break;

        case PROP_EVENT_ENABLE:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, TRANSITION_TO_OFFNORMAL,
                (CurrentBV->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ? true
                                                                      : false);
            bitstring_set_bit(&bit_string, TRANSITION_TO_FAULT,
                (CurrentBV->Event_Enable & EVENT_ENABLE_TO_FAULT) ? true
                                                                  : false);
            bitstring_set_bit(&bit_string, TRANSITION_TO_NORMAL,
                (CurrentBV->Event_Enable & EVENT_ENABLE_TO_NORMAL) ? true
                                                                   : false);

            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;

        case PROP_EVENT_DETECTION_ENABLE:
            apdu_len =
                encode_application_boolean(&apdu[0], CurrentBV->Event_Detection_Enable);
            break;

        case PROP_ACKED_TRANSITIONS:
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, TRANSITION_TO_OFFNORMAL,
                CurrentBV->Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked);
            bitstring_set_bit(&bit_string, TRANSITION_TO_FAULT,
                CurrentBV->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked);
            bitstring_set_bit(&bit_string, TRANSITION_TO_NORMAL,
                CurrentBV->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked);

            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;

        case PROP_NOTIFY_TYPE:
            apdu_len = encode_application_enumerated(
                &apdu[0], CurrentBV->Notify_Type ? NOTIFY_EVENT : NOTIFY_ALARM);
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
                    &CurrentBV->Event_Time_Stamps[i].date);
                len += encode_application_time(&apdu[apdu_len + len],
                    &CurrentBV->Event_Time_Stamps[i].time);
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
                    &CurrentBV->Event_Time_Stamps[rpdata->array_index].date);
                apdu_len += encode_application_time(&apdu[apdu_len],
                    &CurrentBV->Event_Time_Stamps[rpdata->array_index].time);
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

    /* Only array properties can have array options. */
    if ((apdu_len >= 0) && (rpdata->object_property != PROP_PRIORITY_ARRAY) &&
        (rpdata->array_index != BACNET_ARRAY_ALL)) {
        rpdata->error_class = ERROR_CLASS_PROPERTY;
        rpdata->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = BACNET_STATUS_ERROR;
    }

    return apdu_len;
}

/**
 * Set the requested property of the binary value.
 *
 * @param wp_data  Property requested, see for BACNET_WRITE_PROPERTY_DATA
 * details.
 *
 * @return true if successful
 */
bool Binary_Value_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    bool status = false; /* return value */
    unsigned int object_index = 0;
    unsigned int priority = 0;
    BACNET_BINARY_PV level = BINARY_NULL;
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;
#if (BINARY_VALUE_INTRINSIC_REPORTING)
    BINARY_VALUE_DESCR *CurrentBV = NULL;
#endif

    /* Valid data? */
    if (wp_data == NULL) {
        return false;
    }
    if (wp_data->application_data_len == 0) {
        return false;
    }

    /* Decode the some of the request. */
    len = bacapp_decode_application_data(
        wp_data->application_data, wp_data->application_data_len, &value);
    /* FIXME: len < application_data_len: more data? */
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    /* Only array properties can have array options. */
    if ((wp_data->object_property != PROP_PRIORITY_ARRAY) &&
        (wp_data->array_index != BACNET_ARRAY_ALL)) {
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        return false;
    }

    /* Valid object index? */
    object_index = Binary_Value_Instance_To_Index(wp_data->object_instance);
    if (object_index >= BV_Max_Index) {
        wp_data->error_class = ERROR_CLASS_OBJECT;
        wp_data->error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }

#if (BINARY_VALUE_INTRINSIC_REPORTING)
    CurrentBV = &BV_Descr[object_index];
#endif

    switch (wp_data->object_property) {
        case PROP_PRESENT_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                priority = wp_data->priority;
                /* Command priority 6 is reserved for use by Minimum On/Off
                   algorithm and may not be used for other purposes in any
                   object. */
                if (priority && (priority <= BACNET_MAX_PRIORITY) &&
                    (priority != 6 /* reserved */) &&
                    (value.type.Enumerated <= MAX_BINARY_PV)) {
                    level = (BACNET_BINARY_PV)value.type.Enumerated;
                    priority--;
                    Binary_Value_Level[object_index][priority] = level;
                    /* Note: you could set the physical output here if we
                       are the highest priority.
                       However, if Out of Service is TRUE, then don't set the
                       physical output.  This comment may apply to the
                       main loop (i.e. check out of service before changing
                       output) */
                    status = true;
                } else if (priority == 6) {
                    /* Command priority 6 is reserved for use by Minimum On/Off
                       algorithm and may not be used for other purposes in any
                       object. */
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
                } else {
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            } else {
                status = write_property_type_valid(
                    wp_data, &value, BACNET_APPLICATION_TAG_NULL);
                if (status) {
                    level = BINARY_NULL;
                    priority = wp_data->priority;
                    if (priority && (priority <= BACNET_MAX_PRIORITY)) {
                        priority--;
                        Binary_Value_Level[object_index][priority] = level;
                        /* Note: you could set the physical output here to the
                           next highest priority, or to the relinquish default
                           if no priorities are set. However, if Out of Service
                           is TRUE, then don't set the physical output.  This
                           comment may apply to the
                           main loop (i.e. check out of service before changing
                           output) */
                    } else {
                        status = false;
                        wp_data->error_class = ERROR_CLASS_PROPERTY;
                        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                    }
                }
            }
            break;
        case PROP_OUT_OF_SERVICE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_BOOLEAN);
            if (status) {
                Binary_Value_Out_Of_Service_Set(
                    wp_data->object_instance, value.type.Boolean);
            }
            break;
#if (BINARY_VALUE_INTRINSIC_REPORTING)
        case PROP_TIME_DELAY:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                CurrentBV->Time_Delay = value.type.Unsigned_Int;
                CurrentBV->Remaining_Time_Delay = CurrentBV->Time_Delay;
            }
            break;

        case PROP_NOTIFICATION_CLASS:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_UNSIGNED_INT);
            if (status) {
                CurrentBV->Notification_Class = value.type.Unsigned_Int;
            }
            break;

        case PROP_ALARM_VALUE:
            status = write_property_type_valid(
                wp_data, &value, BACNET_APPLICATION_TAG_ENUMERATED);
            if (status) {
                if (value.type.Enumerated <= MAX_BINARY_PV) {
                    Binary_Value_Alarm_Value_Set(wp_data->object_instance,
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
                    CurrentBV->Event_Enable = value.type.Bit_String.value[0];
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
                        CurrentBV->Notify_Type = 1;
                        break;
                    case NOTIFY_ALARM:
                        CurrentBV->Notify_Type = 0;
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
        case PROP_PRIORITY_ARRAY:
        case PROP_RELINQUISH_DEFAULT:
#if (BINARY_VALUE_INTRINSIC_REPORTING)
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
unsigned Binary_Value_Event_State(uint32_t object_instance)
{
    unsigned state = EVENT_STATE_NORMAL;
#if !(BINARY_VALUE_INTRINSIC_REPORTING)
  (void) object_instance;
#endif
#if (BINARY_VALUE_INTRINSIC_REPORTING)
    unsigned index = Binary_Value_Instance_To_Index(object_instance);

    if (index < BV_Max_Index) {
        state = BV_Descr[index].Event_State;
    }
#endif

    return state;
}

#if (BINARY_VALUE_INTRINSIC_REPORTING)
/**
 * For a given object instance-number, gets the event-detection-enable property value
 *
 * @param  object_instance - object-instance number of the object
 *
 * @return  event-detection-enable property value
 */
bool Binary_Value_Event_Detection_Enable(uint32_t object_instance)
{
    bool retval = false;
#if !(BINARY_VALUE_INTRINSIC_REPORTING)
    (void) object_instance;
#endif
#if (BINARY_VALUE_INTRINSIC_REPORTING)
    unsigned index = Binary_Value_Instance_To_Index(object_instance);

    if (index < BV_Max_Index) {
        retval = BV_Descr[index].Event_Detection_Enable;
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
bool Binary_Value_Event_Detection_Enable_Set(uint32_t object_instance, bool value)
{
    bool retval = false;
#if !(BINARY_VALUE_INTRINSIC_REPORTING)
    (void) object_instance;
    (void) value;
#endif
#if (BINARY_VALUE_INTRINSIC_REPORTING)
    unsigned index = Binary_Value_Instance_To_Index(object_instance);

    if (index < BV_Max_Index) {
        BV_Descr[index].Event_Detection_Enable = value;
        retval = true;
    }
#endif

    return retval;
}

int Binary_Value_Event_Information(
    unsigned index, BACNET_GET_EVENT_INFORMATION_DATA *getevent_data)
{
    bool IsNotAckedTransitions;
    bool IsActiveEvent;
    int i;

    /* check index */
    if (index < BV_Max_Index) {
        /* Event_State not equal to NORMAL */
        IsActiveEvent = (BV_Descr[index].Event_State != EVENT_STATE_NORMAL);

        /* Acked_Transitions property, which has at least one of the bits
           (TO-OFFNORMAL, TO-FAULT, TONORMAL) set to FALSE. */
        IsNotAckedTransitions =
            (BV_Descr[index]
                    .Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                    .bIsAcked == false) |
            (BV_Descr[index].Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked ==
                false) |
            (BV_Descr[index].Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked ==
                false);
    } else
        return -1; /* end of list  */

    if ((IsActiveEvent) || (IsNotAckedTransitions)) {
        /* Object Identifier */
        getevent_data->objectIdentifier.type = OBJECT_BINARY_VALUE;
        getevent_data->objectIdentifier.instance =
            Binary_Value_Index_To_Instance(index);
        /* Event State */
        getevent_data->eventState = BV_Descr[index].Event_State;
        /* Acknowledged Transitions */
        bitstring_init(&getevent_data->acknowledgedTransitions);
        bitstring_set_bit(&getevent_data->acknowledgedTransitions,
            TRANSITION_TO_OFFNORMAL,
            BV_Descr[index]
                .Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                .bIsAcked);
        bitstring_set_bit(&getevent_data->acknowledgedTransitions,
            TRANSITION_TO_FAULT,
            BV_Descr[index].Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked);
        bitstring_set_bit(&getevent_data->acknowledgedTransitions,
            TRANSITION_TO_NORMAL,
            BV_Descr[index].Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked);
        /* Event Time Stamps */
        for (i = 0; i < 3; i++) {
            getevent_data->eventTimeStamps[i].tag = TIME_STAMP_DATETIME;
            getevent_data->eventTimeStamps[i].value.dateTime =
                BV_Descr[index].Event_Time_Stamps[i];
        }
        /* Notify Type */
        getevent_data->notifyType = BV_Descr[index].Notify_Type;
        /* Event Enable */
        bitstring_init(&getevent_data->eventEnable);
        bitstring_set_bit(&getevent_data->eventEnable, TRANSITION_TO_OFFNORMAL,
            (BV_Descr[index].Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ? true
                                                                       : false);
        bitstring_set_bit(&getevent_data->eventEnable, TRANSITION_TO_FAULT,
            (BV_Descr[index].Event_Enable & EVENT_ENABLE_TO_FAULT) ? true
                                                                   : false);
        bitstring_set_bit(&getevent_data->eventEnable, TRANSITION_TO_NORMAL,
            (BV_Descr[index].Event_Enable & EVENT_ENABLE_TO_NORMAL) ? true
                                                                    : false);
        /* Event Priorities */
        Notification_Class_Get_Priorities(
            BV_Descr[index].Notification_Class, getevent_data->eventPriorities);

        return 1; /* active event */
    } else
        return 0; /* no active event at this index */
}

int Binary_Value_Alarm_Ack(
    BACNET_ALARM_ACK_DATA *alarmack_data, BACNET_ERROR_CODE *error_code)
{
    BINARY_VALUE_DESCR *CurrentBV;
    unsigned int object_index;

    if((error_code == NULL) || (alarmack_data == NULL)) {
      PRINT("[%s %d]: NULL pointer parameter! error_code = %p; alarmack_data = %p\r\n", __FILE__, __LINE__, error_code, alarmack_data);
      return -2;
    }

    object_index = Binary_Value_Instance_To_Index(
        alarmack_data->eventObjectIdentifier.instance);

    if (object_index < BV_Max_Index)
        CurrentBV = &BV_Descr[object_index];
    else {
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return -1;
    }

    switch (alarmack_data->eventStateAcked) {
        case EVENT_STATE_OFFNORMAL:
            if (CurrentBV->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                    .bIsAcked == false) {
                if (alarmack_data->eventTimeStamp.tag != TIME_STAMP_DATETIME) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                if (datetime_compare(
                        &CurrentBV->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                             .Time_Stamp,
                        &alarmack_data->eventTimeStamp.value.dateTime) > 0) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                /* Send ack notification */
                CurrentBV->Acked_Transitions[TRANSITION_TO_OFFNORMAL].bIsAcked =
                    true;
            } else if (alarmack_data->eventStateAcked ==
                CurrentBV->Event_State) {
                /* Send ack notification */
            } else {
                *error_code = ERROR_CODE_INVALID_EVENT_STATE;
                return -1;
            }
            break;

        case EVENT_STATE_FAULT:
            if (CurrentBV->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked ==
                false) {
                if (alarmack_data->eventTimeStamp.tag != TIME_STAMP_DATETIME) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                if (datetime_compare(
                        &CurrentBV->Acked_Transitions[TRANSITION_TO_FAULT]
                             .Time_Stamp,
                        &alarmack_data->eventTimeStamp.value.dateTime) > 0) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                /* Send ack notification */
                CurrentBV->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked =
                    true;
            } else if (alarmack_data->eventStateAcked ==
                CurrentBV->Event_State) {
                /* Send ack notification */
            } else {
                *error_code = ERROR_CODE_INVALID_EVENT_STATE;
                return -1;
            }
            break;

        case EVENT_STATE_NORMAL:
            if (CurrentBV->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked ==
                false) {
                if (alarmack_data->eventTimeStamp.tag != TIME_STAMP_DATETIME) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                if (datetime_compare(
                        &CurrentBV->Acked_Transitions[TRANSITION_TO_NORMAL]
                             .Time_Stamp,
                        &alarmack_data->eventTimeStamp.value.dateTime) > 0) {
                    *error_code = ERROR_CODE_INVALID_TIME_STAMP;
                    return -1;
                }
                /* Send ack notification */
                CurrentBV->Acked_Transitions[TRANSITION_TO_NORMAL].bIsAcked =
                    true;
            } else if (alarmack_data->eventStateAcked ==
                CurrentBV->Event_State) {
                /* Send ack notification */
            } else {
                *error_code = ERROR_CODE_INVALID_EVENT_STATE;
                return -1;
            }
            break;

        default:
            return -2;
    }
    CurrentBV->Ack_notify_data.bSendAckNotify = true;
    CurrentBV->Ack_notify_data.EventState = alarmack_data->eventStateAcked;

    return 1;
}

int Binary_Value_Alarm_Summary(
    unsigned index, BACNET_GET_ALARM_SUMMARY_DATA *getalarm_data)
{
    if(getalarm_data == NULL) {
      PRINT("[%s %d]: NULL pointer parameter! getalarm_data = %p\r\n", __FILE__, __LINE__, getalarm_data);
      return -2;
    }
    /* check index */
    if (index < BV_Max_Index) {
        /* Event_State is not equal to NORMAL  and
           Notify_Type property value is ALARM */
        if ((BV_Descr[index].Event_State != EVENT_STATE_NORMAL) &&
            (BV_Descr[index].Notify_Type == NOTIFY_ALARM)) {
            /* Object Identifier */
            getalarm_data->objectIdentifier.type = OBJECT_BINARY_VALUE;
            getalarm_data->objectIdentifier.instance =
                Binary_Value_Index_To_Instance(index);
            /* Alarm State */
            getalarm_data->alarmState = BV_Descr[index].Event_State;
            /* Acknowledged Transitions */
            bitstring_init(&getalarm_data->acknowledgedTransitions);
            bitstring_set_bit(&getalarm_data->acknowledgedTransitions,
                TRANSITION_TO_OFFNORMAL,
                BV_Descr[index]
                    .Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                    .bIsAcked);
            bitstring_set_bit(&getalarm_data->acknowledgedTransitions,
                TRANSITION_TO_FAULT,
                BV_Descr[index]
                    .Acked_Transitions[TRANSITION_TO_FAULT]
                    .bIsAcked);
            bitstring_set_bit(&getalarm_data->acknowledgedTransitions,
                TRANSITION_TO_NORMAL,
                BV_Descr[index]
                    .Acked_Transitions[TRANSITION_TO_NORMAL]
                    .bIsAcked);

            return 1; /* active alarm */
        } else
            return 0; /* no active alarm at this index */
    } else
        return -1; /* end of list  */
}

bool Binary_Value_Alarm_Value_Set(
    uint32_t object_instance, BACNET_BINARY_PV value)
{
    bool status = false;
    unsigned index = Binary_Value_Instance_To_Index(object_instance);

    if (index < BV_Max_Index) {
        BV_Descr[index].Alarm_Value = value;
        status = true;
    }

    return status;
}
#endif /* (INTRINSIC_REPORTING) */

void Binary_Value_Intrinsic_Reporting(uint32_t object_instance)
{
#if !(BINARY_VALUE_INTRINSIC_REPORTING)
  (void) object_instance;
#endif
#if (BINARY_VALUE_INTRINSIC_REPORTING)
    BACNET_EVENT_NOTIFICATION_DATA event_data = { 0 };
    BACNET_CHARACTER_STRING msgText = { 0 };
    BINARY_VALUE_DESCR *CurrentBV = NULL;
    unsigned int object_index = 0;
    uint8_t FromState = 0;
    uint8_t ToState = 0;
    BACNET_BINARY_PV Alarm_Value = BINARY_INACTIVE;
    BACNET_BINARY_PV PresentVal  = BINARY_INACTIVE;
    bool SendNotify = false;

    object_index = Binary_Value_Instance_To_Index(object_instance);
    if (object_index < BV_Max_Index) {
        CurrentBV = &BV_Descr[object_index];
    } else {
        return;
    }
    /* check whether Intrinsic reporting is enabled*/
    if (!CurrentBV->Event_Detection_Enable) {
        return; /* limits are not configured */
    }

    if (CurrentBV->Ack_notify_data.bSendAckNotify) {
        /* clean bSendAckNotify flag */
        CurrentBV->Ack_notify_data.bSendAckNotify = false;
        /* copy toState */
        ToState = CurrentBV->Ack_notify_data.EventState;
        PRINT("Binary-Value[%d]: Send AckNotification.\n", object_instance);
        characterstring_init_ansi(&msgText, "AckNotification");

        /* Notify Type */
        event_data.notifyType = NOTIFY_ACK_NOTIFICATION;

        /* Send EventNotification. */
        SendNotify = true;
    } else {
        /* actual Present_Value */
        PresentVal = Binary_Value_Present_Value(object_instance);
        FromState = CurrentBV->Event_State;
        switch (CurrentBV->Event_State) {
            case EVENT_STATE_NORMAL:
                /* (a) If pCurrentState is NORMAL, and pMonitoredValue is equal to any of the values contained in pAlarmValues for
                       pTimeDelay, then indicate a transition to the OFFNORMAL event state.
                */
                if ((PresentVal == CurrentBV->Alarm_Value) &&
                    ((CurrentBV->Event_Enable & EVENT_ENABLE_TO_OFFNORMAL) ==
                        EVENT_ENABLE_TO_OFFNORMAL)) {
                    if (!CurrentBV->Remaining_Time_Delay)
                        CurrentBV->Event_State = EVENT_STATE_OFFNORMAL;
                    else
                        CurrentBV->Remaining_Time_Delay--;
                    break;
                }

                /* value of the object is still in the same event state */
                CurrentBV->Remaining_Time_Delay = CurrentBV->Time_Delay;
                break;

            case EVENT_STATE_OFFNORMAL:
                /* (b) If pCurrentState is OFFNORMAL, and pMonitoredValue is not equal to any of the values contained in pAlarmValues
                       for pTimeDelayNormal, then indicate a transition to the NORMAL event state.
                */
                if ((PresentVal != CurrentBV->Alarm_Value) &&
                    ((CurrentBV->Event_Enable & EVENT_ENABLE_TO_NORMAL) ==
                        EVENT_ENABLE_TO_NORMAL)) {
                    if (!CurrentBV->Remaining_Time_Delay)
                        CurrentBV->Event_State = EVENT_STATE_NORMAL;
                    else
                        CurrentBV->Remaining_Time_Delay--;
                    break;
                }

                /* value of the object is still in the same event state */
                CurrentBV->Remaining_Time_Delay = CurrentBV->Time_Delay;
                break;

            default:
                return; /* shouldn't happen */
        } /* switch (FromState) */

        ToState = CurrentBV->Event_State;

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
            PRINT("Binary-Value[%d]: Event_State goes from %.128s to %.128s.\n",
                object_instance, bactext_event_state_name(FromState),
                bactext_event_state_name(ToState));
            /* Notify Type */
            event_data.notifyType = CurrentBV->Notify_Type;

            /* Send EventNotification. */
            SendNotify = true;
        }
    }

    if (SendNotify) {
        /* Event Object Identifier */
        event_data.eventObjectIdentifier.type = OBJECT_BINARY_VALUE;
        event_data.eventObjectIdentifier.instance = object_instance;

        /* Time Stamp */
        event_data.timeStamp.tag = TIME_STAMP_DATETIME;
        if (event_data.notifyType != NOTIFY_ACK_NOTIFICATION) {
            Device_getCurrentDateTime(&event_data.timeStamp.value.dateTime);
            /* fill Event_Time_Stamps */
            switch (ToState) {
                case EVENT_STATE_OFFNORMAL:
                    datetime_copy(
                        &CurrentBV->Event_Time_Stamps[TRANSITION_TO_OFFNORMAL],
                        &event_data.timeStamp.value.dateTime);
                    break;
                case EVENT_STATE_FAULT:
                    datetime_copy(
                        &CurrentBV->Event_Time_Stamps[TRANSITION_TO_FAULT],
                        &event_data.timeStamp.value.dateTime);
                    break;
                case EVENT_STATE_NORMAL:
                    datetime_copy(
                        &CurrentBV->Event_Time_Stamps[TRANSITION_TO_NORMAL],
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
                        &CurrentBV->Event_Time_Stamps[TRANSITION_TO_FAULT]);
                    break;
                case EVENT_STATE_NORMAL:
                    datetime_copy(&event_data.timeStamp.value.dateTime,
                        &CurrentBV->Event_Time_Stamps[TRANSITION_TO_NORMAL]);
                    break;
                case EVENT_STATE_OFFNORMAL:
                    datetime_copy(&event_data.timeStamp.value.dateTime,
                        &CurrentBV->Event_Time_Stamps[TRANSITION_TO_OFFNORMAL]);
                    break;
                default:
                    break;
            }
        }

        /* Notification Class */
        event_data.notificationClass = CurrentBV->Notification_Class;

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
        event_data.toState = CurrentBV->Event_State;

        /* Event Values */
        if (event_data.notifyType != NOTIFY_ACK_NOTIFICATION) {
            /* Value that exceeded a limit. */
            event_data.notificationParams.changeOfState.newState =
                (BACNET_PROPERTY_STATE) { .tag = BINARY_VALUE, .state = { .binaryValue = PresentVal } };
            /* Status_Flags of the referenced object. */
            bitstring_init(
                &event_data.notificationParams.changeOfState.statusFlags);
            bitstring_set_bit(
                &event_data.notificationParams.changeOfState.statusFlags,
                STATUS_FLAG_IN_ALARM,
                CurrentBV->Event_State != EVENT_STATE_NORMAL);
            bitstring_set_bit(
                &event_data.notificationParams.changeOfState.statusFlags,
                STATUS_FLAG_FAULT, false);
            bitstring_set_bit(
                &event_data.notificationParams.changeOfState.statusFlags,
                STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(
                &event_data.notificationParams.changeOfState.statusFlags,
                STATUS_FLAG_OUT_OF_SERVICE, CurrentBV->Out_Of_Service);
        }

        /* add data from notification class */
        PRINT("Binary-Value[%d]: Notification Class[%d]-%s "
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
            PRINT("Binary-Value[%d]: Ack Required!\n", object_instance);
            switch (event_data.toState) {
                case EVENT_STATE_OFFNORMAL:
                    CurrentBV->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                        .bIsAcked = false;
                    CurrentBV->Acked_Transitions[TRANSITION_TO_OFFNORMAL]
                        .Time_Stamp = event_data.timeStamp.value.dateTime;
                    break;

                case EVENT_STATE_FAULT:
                    CurrentBV->Acked_Transitions[TRANSITION_TO_FAULT].bIsAcked =
                        false;
                    CurrentBV->Acked_Transitions[TRANSITION_TO_FAULT]
                        .Time_Stamp = event_data.timeStamp.value.dateTime;
                    break;

                case EVENT_STATE_NORMAL:
                    CurrentBV->Acked_Transitions[TRANSITION_TO_NORMAL]
                        .bIsAcked = false;
                    CurrentBV->Acked_Transitions[TRANSITION_TO_NORMAL]
                        .Time_Stamp = event_data.timeStamp.value.dateTime;
                    break;

                default: /* shouldn't happen */
                    break;
            }
        }
    }
#endif /* (BINARY_VALUE_INTRINSIC_REPORTING) */
}

