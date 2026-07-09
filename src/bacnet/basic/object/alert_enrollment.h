/**
 * @file
 * @brief API for basic BACnet Alert Enrollemnt Object implementation.
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_ALERT_ENROLLMENT_H
#define BACNET_BASIC_OBJECT_ALERT_ENROLLMENT_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/basic/sys/ringbuf.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

#if BACNET_EVENT_EXTENDED_ENABLED && defined(INTRINSIC_REPORTING)

/* count must be a power of 2 for ringbuf library */
#ifndef ALERT_ENROLLMENT_ALERT_COUNT
#define ALERT_ENROLLMENT_ALERT_COUNT 2
#endif

typedef struct bacnet_alert {
    BACNET_OBJECT_ID source;
    BACNET_CHARACTER_STRING_BUFFER messageText;
    BACNET_TIMESTAMP timeStamp;
    uint16_t vendorID;
    BACNET_UNSIGNED_INTEGER extendedEventType;
} BACNET_ALERT;

typedef struct alert_enrollment_descr {
    BACNET_OBJECT_ID Present_Value;
    BACNET_RELIABILITY Reliability;
    const char *Object_Name;
    const char *Description;
    void *Context;
    uint32_t Notification_Class;
    BACNET_ALERT Alert_Buffer[ALERT_ENROLLMENT_ALERT_COUNT];
    RING_BUFFER Alert_Queue;
} ALERT_ENROLLMENT_DESCR;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Alert_Enrollment_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary);
BACNET_STACK_EXPORT
void Alert_Enrollment_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties);

BACNET_STACK_EXPORT
bool Alert_Enrollment_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Alert_Enrollment_Count(void);
BACNET_STACK_EXPORT
uint32_t Alert_Enrollment_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Alert_Enrollment_Instance_To_Index(uint32_t instance);
BACNET_STACK_EXPORT
bool Alert_Enrollment_Object_Instance_Add(uint32_t instance);

BACNET_STACK_EXPORT
bool Alert_Enrollment_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Alert_Enrollment_Name_Set(uint32_t object_instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Alert_Enrollment_Name_ASCII(uint32_t object_instance);

BACNET_STACK_EXPORT
const char *Alert_Enrollment_Description(uint32_t instance);
BACNET_STACK_EXPORT
bool Alert_Enrollment_Description_Set(uint32_t instance, const char *new_name);

BACNET_STACK_EXPORT
BACNET_OBJECT_ID Alert_Enrollment_Present_Value(uint32_t object_instance);
BACNET_STACK_EXPORT
void Alert_Enrollment_Present_Value_Set(
    uint32_t object_instance, BACNET_OBJECT_ID value);

BACNET_STACK_EXPORT
uint32_t Alert_Enrollment_Notification_Class(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Alert_Enrollment_Notification_Class_Set(
    uint32_t object_instance, uint32_t notification_class);

BACNET_STACK_EXPORT
int Alert_Enrollment_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
BACNET_STACK_EXPORT
bool Alert_Enrollment_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
bool Alert_Enrollment_Queue_Alert(
    uint32_t object_instance, const BACNET_ALERT *alert);

BACNET_STACK_EXPORT
void Alert_Enrollment_Intrinsic_Reporting(uint32_t object_instance);

BACNET_STACK_EXPORT
void *Alert_Enrollment_Context_Get(uint32_t object_instance);
BACNET_STACK_EXPORT
void Alert_Enrollment_Context_Set(uint32_t object_instance, void *context);

BACNET_STACK_EXPORT
uint32_t Alert_Enrollment_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Alert_Enrollment_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Alert_Enrollment_Cleanup(void);
BACNET_STACK_EXPORT
void Alert_Enrollment_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
#endif
