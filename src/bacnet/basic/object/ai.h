/**
 * @file
 * @brief API for basic BACnet Analog Input Object implementation.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @author Krzysztof Malorny <malornykrzysztof@gmail.com>
 * @date 2005, 2011
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_ANALOG_INPUT_H
#define BACNET_BASIC_OBJECT_ANALOG_INPUT_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/rp.h"
#include "bacnet/wp.h"
#if defined(INTRINSIC_REPORTING)
#include "bacnet/basic/object/nc.h"
#include "bacnet/getevent.h"
#include "bacnet/alarm_ack.h"
#include "bacnet/get_alarm_sum.h"
#endif

typedef struct analog_input_descr {
    unsigned Event_State:3;
    float Present_Value;
    BACNET_RELIABILITY Reliability;
    bool Out_Of_Service;
    uint8_t Units;
    float Prior_Value;
    float COV_Increment;
    bool Changed;
    char* Object_Name;
    char* Description;
#if defined(INTRINSIC_REPORTING)
    uint32_t Time_Delay;
    uint32_t Notification_Class;
    float High_Limit;
    float Low_Limit;
    float Deadband;
    unsigned Limit_Enable:2;
    unsigned Event_Enable:3;
    unsigned Notify_Type:1;
    ACKED_INFO Acked_Transitions[MAX_BACNET_EVENT_TRANSITION];
    BACNET_DATE_TIME Event_Time_Stamps[MAX_BACNET_EVENT_TRANSITION];
    /* time to generate event notification */
    uint32_t Remaining_Time_Delay;
    /* AckNotification information */
    ACK_NOTIFICATION Ack_notify_data;
#endif
} ANALOG_INPUT_DESCR;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void Analog_Input_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);

    BACNET_STACK_EXPORT
    bool Analog_Input_Valid_Instance(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    unsigned Analog_Input_Count(
        void);
    BACNET_STACK_EXPORT
    uint32_t Analog_Input_Index_To_Instance(
        unsigned index);
    BACNET_STACK_EXPORT
    unsigned Analog_Input_Instance_To_Index(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Analog_Input_Object_Instance_Add(
        uint32_t instance);

    BACNET_STACK_EXPORT
    bool Analog_Input_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    BACNET_STACK_EXPORT
    bool Analog_Input_Name_Set(
        uint32_t object_instance,
        char *new_name);

    BACNET_STACK_EXPORT
    char *Analog_Input_Description(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Analog_Input_Description_Set(
        uint32_t instance,
        char *new_name);

    BACNET_STACK_EXPORT
    BACNET_RELIABILITY Analog_Input_Reliability(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Analog_Input_Reliability_Set(
        uint32_t object_instance,
        BACNET_RELIABILITY value);

    BACNET_STACK_EXPORT
    bool Analog_Input_Units_Set(
        uint32_t instance,
        uint16_t units);
    BACNET_STACK_EXPORT
    uint16_t Analog_Input_Units(
        uint32_t instance);

    BACNET_STACK_EXPORT
    int Analog_Input_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);
    BACNET_STACK_EXPORT
    bool Analog_Input_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    BACNET_STACK_EXPORT
    float Analog_Input_Present_Value(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Analog_Input_Present_Value_Set(
        uint32_t object_instance,
        float value);

    BACNET_STACK_EXPORT
    bool Analog_Input_Out_Of_Service(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Analog_Input_Out_Of_Service_Set(
        uint32_t object_instance,
        bool oos_flag);

    BACNET_STACK_EXPORT
    unsigned Analog_Input_Event_State(uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Analog_Input_Event_State_Set(uint32_t object_instance, unsigned state);

    BACNET_STACK_EXPORT
    bool Analog_Input_Change_Of_Value(
        uint32_t instance);
    BACNET_STACK_EXPORT
    void Analog_Input_Change_Of_Value_Clear(
        uint32_t instance);
    BACNET_STACK_EXPORT
    bool Analog_Input_Encode_Value_List(
        uint32_t object_instance,
        BACNET_PROPERTY_VALUE * value_list);
    float Analog_Input_COV_Increment(
        uint32_t instance);
    BACNET_STACK_EXPORT
    void Analog_Input_COV_Increment_Set(
        uint32_t instance,
        float value);

    /* note: header of Intrinsic_Reporting function is required
       even when INTRINSIC_REPORTING is not defined */
    BACNET_STACK_EXPORT
    void Analog_Input_Intrinsic_Reporting(
        uint32_t object_instance);

#if defined(INTRINSIC_REPORTING)
    BACNET_STACK_EXPORT
    int Analog_Input_Event_Information(
        unsigned index,
        BACNET_GET_EVENT_INFORMATION_DATA * getevent_data);

    BACNET_STACK_EXPORT
    int Analog_Input_Alarm_Ack(
        BACNET_ALARM_ACK_DATA * alarmack_data,
        BACNET_ERROR_CODE * error_code);

    BACNET_STACK_EXPORT
    int Analog_Input_Alarm_Summary(
        unsigned index,
        BACNET_GET_ALARM_SUMMARY_DATA * getalarm_data);
#endif

    BACNET_STACK_EXPORT
    uint32_t Analog_Input_Create(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    bool Analog_Input_Delete(
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Analog_Input_Cleanup(
        void);
    BACNET_STACK_EXPORT
    void Analog_Input_Init(
        void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
