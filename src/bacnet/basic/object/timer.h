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
#include "bacnet/wp.h"
#include "bacnet/rp.h"

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
BACNET_TIMER_STATE Timer_State(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Timer_State_Set(uint32_t object_instance, BACNET_TIMER_STATE value);

BACNET_STACK_EXPORT
BACNET_TIMER_TRANSITION Timer_Last_State_Change(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Timer_Last_State_Change_Set(
    uint32_t object_instance, BACNET_TIMER_TRANSITION change);

BACNET_STACK_EXPORT
bool Timer_Running(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Timer_Running_Set(uint32_t object_instance, bool running);

BACNET_STACK_EXPORT
bool Timer_Description(
    uint32_t object_instance, BACNET_CHARACTER_STRING *description);
BACNET_STACK_EXPORT
bool Timer_Description_Set(uint32_t instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Timer_Description_ANSI(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Timer_Location(
    uint32_t object_instance, BACNET_CHARACTER_STRING *description);
BACNET_STACK_EXPORT
bool Timer_Location_Set(uint32_t instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Timer_Location_ANSI(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Timer_Out_Of_Service(uint32_t instance);
BACNET_STACK_EXPORT
void Timer_Out_Of_Service_Set(uint32_t instance, bool oos_flag);

BACNET_STACK_EXPORT
BACNET_UNSIGNED_INTEGER Timer_Present_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Timer_Present_Value_Set(
    uint32_t object_instance, BACNET_UNSIGNED_INTEGER value);

BACNET_STACK_EXPORT
bool Timer_List_Of_Object_Property_References_Set(
    uint32_t object_instance,
    unsigned index,
    const BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember);
BACNET_STACK_EXPORT
bool Timer_List_Of_Object_Property_References_Member(
    uint32_t object_instance,
    unsigned index,
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE *pMember);
BACNET_STACK_EXPORT
size_t
Timer_List_Of_Object_Property_References_Capacity(uint32_t object_instance);

BACNET_STACK_EXPORT
void Timer_Timer(uint32_t object_instance, uint16_t milliseconds);
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
