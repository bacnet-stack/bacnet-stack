/**
 * @file
 * @brief API for Program object type
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date March 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_PROGRAM_H
#define BACNET_BASIC_OBJECT_PROGRAM_H
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
void Program_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary);
BACNET_STACK_EXPORT
bool Program_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Program_Count(void);
BACNET_STACK_EXPORT
uint32_t Program_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Program_Instance_To_Index(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Program_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Program_Name_Set(uint32_t object_instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Program_Name_ASCII(uint32_t object_instance);

BACNET_STACK_EXPORT
int Program_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);

BACNET_STACK_EXPORT
bool Program_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
BACNET_PROGRAM_STATE Program_State(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Program_State_Set(uint32_t object_instance, BACNET_PROGRAM_STATE value);

BACNET_STACK_EXPORT
BACNET_PROGRAM_REQUEST Program_Change(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Program_Change_Set(
    uint32_t object_instance, BACNET_PROGRAM_REQUEST program_change);
BACNET_STACK_EXPORT
BACNET_PROGRAM_ERROR Program_Reason_For_Halt(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Program_Reason_For_Halt_Set(
    uint32_t object_instance, BACNET_PROGRAM_ERROR reason);

BACNET_STACK_EXPORT
bool Program_Description(
    uint32_t object_instance, BACNET_CHARACTER_STRING *description);
BACNET_STACK_EXPORT
bool Program_Description_Set(uint32_t instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Program_Description_ANSI(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Program_Location(
    uint32_t object_instance, BACNET_CHARACTER_STRING *description);
BACNET_STACK_EXPORT
bool Program_Location_Set(uint32_t instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Program_Location_ANSI(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Program_Instance_Of(
    uint32_t object_instance, BACNET_CHARACTER_STRING *description);
BACNET_STACK_EXPORT
bool Program_Instance_Of_Set(uint32_t instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Program_Instance_Of_ANSI(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Program_Description_Of_Halt(
    uint32_t object_instance, BACNET_CHARACTER_STRING *description);
BACNET_STACK_EXPORT
bool Program_Description_Of_Halt_Set(uint32_t instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Program_Description_Of_Halt_ANSI(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Program_Out_Of_Service(uint32_t instance);
BACNET_STACK_EXPORT
void Program_Out_Of_Service_Set(uint32_t instance, bool oos_flag);

BACNET_STACK_EXPORT
void Program_Timer(uint32_t object_instance, uint16_t milliseconds);
BACNET_STACK_EXPORT
uint32_t Program_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Program_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Program_Cleanup(void);
BACNET_STACK_EXPORT
void Program_Init(void);

/* API for the program requests
    note: return value is 0 for success, non-zero for failure
*/
BACNET_STACK_EXPORT
void *Program_Context_Get(uint32_t object_instance);
BACNET_STACK_EXPORT
void Program_Context_Set(uint32_t object_instance, void *context);
BACNET_STACK_EXPORT
void Program_Load_Set(uint32_t object_instance, int (*load)(void *context));
BACNET_STACK_EXPORT
void Program_Run_Set(uint32_t object_instance, int (*run)(void *context));
BACNET_STACK_EXPORT
void Program_Halt_Set(uint32_t object_instance, int (*halt)(void *context));
BACNET_STACK_EXPORT
void Program_Restart_Set(
    uint32_t object_instance, int (*restart)(void *context));
BACNET_STACK_EXPORT
void Program_Unload_Set(uint32_t object_instance, int (*unload)(void *context));

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
