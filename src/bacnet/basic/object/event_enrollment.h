/**
 * @file
 * @brief The Event Enrollment object type defines a standardized object
 *  that represents and contains the information required for
 *  algorithmic reporting of events.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_EVENT_ENROLLMENT_H
#define BACNET_BASIC_OBJECT_EVENT_ENROLLMENT_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Event_Enrollment_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary);
BACNET_STACK_EXPORT
bool Event_Enrollment_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Event_Enrollment_Count(void);
BACNET_STACK_EXPORT
uint32_t Event_Enrollment_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Event_Enrollment_Instance_To_Index(uint32_t object_instance);

BACNET_STACK_EXPORT
bool Event_Enrollment_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Event_Enrollment_Name_ASCII_Set(
    uint32_t object_instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Event_Enrollment_Name_ASCII(uint32_t object_instance);

BACNET_STACK_EXPORT
int Event_Enrollment_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);

BACNET_STACK_EXPORT
bool Event_Enrollment_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
const char *Event_Enrollment_Description(uint32_t instance);
BACNET_STACK_EXPORT
bool Event_Enrollment_Description_Set(uint32_t instance, const char *new_name);

BACNET_STACK_EXPORT
void Event_Enrollment_Timer(uint32_t object_instance, uint16_t milliseconds);

BACNET_STACK_EXPORT
uint32_t Event_Enrollment_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Event_Enrollment_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Event_Enrollment_Cleanup(void);
BACNET_STACK_EXPORT
void Event_Enrollment_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
