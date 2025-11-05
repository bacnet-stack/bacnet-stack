/**
 * @file
 * @brief API for Timer object type
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_TIMER_H
#define BACNET_BASIC_OBJECT_TIMER_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/timer_value.h"
#include "bacnet/wp.h"
#include "bacnet/rp.h"
#include "bacnet/list_element.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Timer_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary);
BACNET_STACK_EXPORT
bool Timer_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Timer_Count(void);
BACNET_STACK_EXPORT
uint32_t Timer_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Timer_Instance_To_Index(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Timer_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Timer_Name_Set(uint32_t object_instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Timer_Name_ASCII(uint32_t object_instance);

BACNET_STACK_EXPORT
int Timer_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);

BACNET_STACK_EXPORT
bool Timer_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
bool Timer_Description(
    uint32_t object_instance, BACNET_CHARACTER_STRING *description);
BACNET_STACK_EXPORT
bool Timer_Description_Set(uint32_t instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Timer_Description_ANSI(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Timer_Out_Of_Service(uint32_t instance);
BACNET_STACK_EXPORT
void Timer_Out_Of_Service_Set(uint32_t instance, bool oos_flag);

BACNET_STACK_EXPORT
BACNET_RELIABILITY Timer_Reliability(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Timer_Reliability_Set(uint32_t object_instance, BACNET_RELIABILITY value);

BACNET_STACK_EXPORT
uint32_t Timer_Present_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Timer_Present_Value_Set(uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
BACNET_TIMER_STATE Timer_State(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Timer_State_Set(uint32_t object_instance, BACNET_TIMER_STATE value);

BACNET_STACK_EXPORT
BACNET_TIMER_TRANSITION Timer_Last_State_Change(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Timer_Running(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Timer_Running_Set(uint32_t object_instance, bool running);

BACNET_STACK_EXPORT
bool Timer_Update_Time(uint32_t object_instance, BACNET_DATE_TIME *bdatetime);
BACNET_STACK_EXPORT
bool Timer_Update_Time_Set(
    uint32_t object_instance, BACNET_DATE_TIME *bdatetime);

BACNET_STACK_EXPORT
bool Timer_Expiration_Time(
    uint32_t object_instance, BACNET_DATE_TIME *bdatetime);

BACNET_STACK_EXPORT
uint32_t Timer_Initial_Timeout(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Timer_Initial_Timeout_Set(uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
uint32_t Timer_Default_Timeout(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Timer_Default_Timeout_Set(uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
uint32_t Timer_Min_Pres_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Timer_Min_Pres_Value_Set(uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
uint32_t Timer_Max_Pres_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Timer_Max_Pres_Value_Set(uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
uint32_t Timer_Resolution(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Timer_Resolution_Set(uint32_t object_instance, uint32_t value);

BACNET_STACK_EXPORT
uint8_t Timer_Priority_For_Writing(uint8_t object_instance);
BACNET_STACK_EXPORT
bool Timer_Priority_For_Writing_Set(uint32_t object_instance, uint8_t value);

BACNET_STACK_EXPORT
unsigned Timer_Reference_List_Member_Capacity(uint32_t object_instance);
BACNET_STACK_EXPORT
BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *Timer_Reference_List_Member_Element(
    uint32_t object_instance, unsigned list_index);
BACNET_STACK_EXPORT
bool Timer_Reference_List_Member_Element_Set(
    uint32_t object_instance,
    unsigned index,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember);
BACNET_STACK_EXPORT
bool Timer_Reference_List_Member_Element_Add(
    uint32_t object_instance,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pNewMember);
BACNET_STACK_EXPORT
bool Timer_Reference_List_Member_Element_Remove(
    uint32_t object_instance,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pRemoveMember);
BACNET_STACK_EXPORT
unsigned Timer_Reference_List_Member_Element_Count(uint32_t object_instance);

BACNET_STACK_EXPORT
BACNET_TIMER_STATE_CHANGE_VALUE *Timer_State_Change_Value(
    uint32_t object_instance, BACNET_TIMER_TRANSITION transition);
BACNET_STACK_EXPORT
bool Timer_State_Change_Value_Set(
    uint32_t object_instance,
    BACNET_TIMER_TRANSITION transition,
    BACNET_TIMER_STATE_CHANGE_VALUE *value);

BACNET_STACK_EXPORT
void Timer_Task(uint32_t object_instance, uint16_t milliseconds);

BACNET_STACK_EXPORT
void Timer_Write_Property_Internal_Callback_Set(write_property_function cb);

BACNET_STACK_EXPORT
int Timer_Add_List_Element(BACNET_LIST_ELEMENT_DATA *list_element);

BACNET_STACK_EXPORT
int Timer_Remove_List_Element(BACNET_LIST_ELEMENT_DATA *list_element);

BACNET_STACK_EXPORT
uint32_t Timer_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Timer_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Timer_Cleanup(void);
BACNET_STACK_EXPORT
void Timer_Init(void);

/* API for the program requests
    note: return value is 0 for success, non-zero for failure
*/
BACNET_STACK_EXPORT
void *Timer_Context_Get(uint32_t object_instance);
BACNET_STACK_EXPORT
void Timer_Context_Set(uint32_t object_instance, void *context);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
