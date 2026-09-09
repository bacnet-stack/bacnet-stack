/**
 * @file
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @brief API for a basic BACnet Schedule object implementation.
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_SCHEDULE_H
#define BACNET_BASIC_OBJECT_SCHEDULE_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacapp.h"
#include "bacnet/bacstr.h"
#include "bacnet/datetime.h"
#include "bacnet/bacerror.h"
#include "bacnet/wp.h"
#include "bacnet/rp.h"
#include "bacnet/list_element.h"
#include "bacnet/bacdevobjpropref.h"
#include "bacnet/bactimevalue.h"
#include "bacnet/dailyschedule.h"
#include "bacnet/special_event.h"

/* DoS guard: maximum List_Of_Object_Property_References entries (resizable
   BACnetLIST, backed by an OS_Keylist, up to this many per object) */
#ifndef BACNET_SCHEDULE_OBJ_PROP_REF_SIZE
#define BACNET_SCHEDULE_OBJ_PROP_REF_SIZE 4
#endif

/* DoS guard: maximum Exception_Schedule entries (resizable BACnetARRAY,
   backed by an OS_Keylist, up to this many per object) */
#ifndef BACNET_EXCEPTION_SCHEDULE_SIZE
#define BACNET_EXCEPTION_SCHEDULE_SIZE 8
#endif

/* DoS guard: maximum Time-Values per Weekly_Schedule day (resizable
   per-day storage, backed by an OS_Keylist, up to this many per day) */
#ifndef BACNET_SCHEDULE_DAILY_TIME_VALUES_MAX
#define BACNET_SCHEDULE_DAILY_TIME_VALUES_MAX \
    BACNET_DAILY_SCHEDULE_TIME_VALUES_SIZE
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Schedule_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary);
BACNET_STACK_EXPORT
void Schedule_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties);

BACNET_STACK_EXPORT
bool Schedule_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Schedule_Count(void);
BACNET_STACK_EXPORT
uint32_t Schedule_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Schedule_Instance_To_Index(uint32_t instance);
BACNET_STACK_EXPORT
void Schedule_Init(void);

BACNET_STACK_EXPORT
uint32_t Schedule_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Schedule_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Schedule_Cleanup(void);

BACNET_STACK_EXPORT
void Schedule_Out_Of_Service_Set(uint32_t object_instance, bool value);
BACNET_STACK_EXPORT
bool Schedule_Out_Of_Service(uint32_t object_instance);

/* Weekly_Schedule: fixed BACnetARRAY[7] of day schedules (per the standard);
   each day's Time-Values are stored dynamically (see accessors below) */
BACNET_STACK_EXPORT
bool Schedule_Weekly_Schedule(
    uint32_t object_instance,
    unsigned array_index,
    BACNET_DAILY_SCHEDULE_ENTRY *entries,
    size_t entries_size,
    size_t *entries_count);
BACNET_STACK_EXPORT
bool Schedule_Weekly_Schedule_Set(
    uint32_t object_instance,
    unsigned array_index,
    const BACNET_DAILY_SCHEDULE_ENTRY *entries);
BACNET_STACK_EXPORT
size_t Schedule_Weekly_Schedule_Time_Value_Count(
    uint32_t object_instance, unsigned array_index);
BACNET_STACK_EXPORT
bool Schedule_Weekly_Schedule_Time_Value(
    uint32_t object_instance,
    unsigned array_index,
    unsigned index,
    BACNET_TIME_VALUE *value);
BACNET_STACK_EXPORT
bool Schedule_Weekly_Schedule_Time_Value_Set(
    uint32_t object_instance,
    unsigned array_index,
    unsigned index,
    const BACNET_TIME_VALUE *value);
BACNET_STACK_EXPORT
bool Schedule_Weekly_Schedule_Time_Value_Delete_All(
    uint32_t object_instance, unsigned array_index);

/* Exception_Schedule: resizable BACnetARRAY of BACnetSpecialEvent */
BACNET_STACK_EXPORT
BACNET_SPECIAL_EVENT *
Schedule_Exception_Schedule(uint32_t object_instance, unsigned index);
BACNET_STACK_EXPORT
bool Schedule_Exception_Schedule_Set(
    uint32_t object_instance,
    unsigned index,
    const BACNET_SPECIAL_EVENT *value);
BACNET_STACK_EXPORT
unsigned Schedule_Exception_Schedule_Count(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Schedule_Exception_Schedule_Add(
    uint32_t object_instance, const BACNET_SPECIAL_EVENT *value);
BACNET_STACK_EXPORT
bool Schedule_Exception_Schedule_Delete_All(uint32_t object_instance);

/* List_Of_Object_Property_References: resizable BACnetLIST of
   BACnetDeviceObjectPropertyReference */
BACNET_STACK_EXPORT
bool Schedule_List_Of_Object_Property_References_Set(
    uint32_t object_instance,
    unsigned index,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember);
BACNET_STACK_EXPORT
bool Schedule_List_Of_Object_Property_References(
    uint32_t object_instance,
    unsigned index,
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember);
BACNET_STACK_EXPORT
size_t
Schedule_List_Of_Object_Property_References_Capacity(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned
Schedule_List_Of_Object_Property_References_Count(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Schedule_List_Of_Object_Property_References_Add(
    uint32_t object_instance,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember);
BACNET_STACK_EXPORT
bool Schedule_List_Of_Object_Property_References_Delete_All(
    uint32_t object_instance);

BACNET_STACK_EXPORT
bool Schedule_Effective_Period_Set(
    uint32_t object_instance,
    const BACNET_DATE *start_date,
    const BACNET_DATE *end_date);
BACNET_STACK_EXPORT
bool Schedule_Effective_Period(
    uint32_t object_instance, BACNET_DATE *start_date, BACNET_DATE *end_date);

BACNET_STACK_EXPORT
bool Schedule_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Schedule_Name_Set(uint32_t object_instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Schedule_Name_ASCII(uint32_t object_instance);
BACNET_STACK_EXPORT
const char *Schedule_Description(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Schedule_Description_Set(uint32_t object_instance, const char *new_name);

BACNET_STACK_EXPORT
int Schedule_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
BACNET_STACK_EXPORT
bool Schedule_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);
BACNET_STACK_EXPORT
int Schedule_Add_List_Element(BACNET_LIST_ELEMENT_DATA *list_element);
BACNET_STACK_EXPORT
int Schedule_Remove_List_Element(BACNET_LIST_ELEMENT_DATA *list_element);

/* utility functions for calculating current Present Value */
BACNET_STACK_EXPORT
bool Schedule_In_Effective_Period(
    uint32_t object_instance, const BACNET_DATE *date);
/* weekly-schedule-only Present Value calculation (no Exception_Schedule) */
BACNET_STACK_EXPORT
void Schedule_Recalculate_PV(
    uint32_t object_instance, BACNET_WEEKDAY wday, const BACNET_TIME *time);
/* full Present Value calculation per 135-2024 12.24.4, including
 * Exception_Schedule priority evaluation */
BACNET_STACK_EXPORT
void Schedule_Calendar_Present_Value_Update(
    uint32_t object_instance, const BACNET_DATE *date, const BACNET_TIME *time);
#if BACNET_EXCEPTION_SCHEDULE_SIZE
BACNET_STACK_EXPORT
void Schedule_Read_Property_Internal_Callback_Set(read_property_function cb);
#endif

BACNET_STACK_EXPORT
void Schedule_Timer(uint32_t object_instance, uint16_t milliseconds);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
