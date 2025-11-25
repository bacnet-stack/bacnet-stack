/**
 * @file
 * @author Mikhail Antropov <michail.antropov@dsr-corporation.com>
 * @date Jul 2023
 * @brief Auditlog object, customize for your use
 *
 * @section DESCRIPTION
 *
 * An Audit Log object combines audit notifications from operation sources and
 * operation targets and stores the combined record in an internal buffer for
 * subsequent retrieval. Each timestamped buffer entry is called an audit log
 * "record."
 *
 * @section LICENSE
 *
 * Copyright (C) 2023 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef AUDITLOG_H
#define AUDITLOG_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacaudit.h"
#include "bacnet/bacdest.h"
#include "bacnet/cov.h"
#include "bacnet/datetime.h"
#include "bacnet/readrange.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void Audit_Log_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary);

BACNET_STACK_EXPORT
bool Audit_Log_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Audit_Log_Count(void);
BACNET_STACK_EXPORT
uint32_t Audit_Log_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Audit_Log_Instance_To_Index(uint32_t instance);
BACNET_STACK_EXPORT
bool Audit_Log_Object_Instance_Add(uint32_t instance);

BACNET_STACK_EXPORT
bool Audit_Log_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Audit_Log_Name_Set(uint32_t object_instance, const char *new_name);
BACNET_STACK_EXPORT
const char *Audit_Log_Name_ASCII(uint32_t object_instance);

BACNET_STACK_EXPORT
const char *Audit_Log_Description(uint32_t instance);
BACNET_STACK_EXPORT
bool Audit_Log_Description_Set(uint32_t instance, const char *new_name);

BACNET_STACK_EXPORT
int Audit_Log_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
BACNET_STACK_EXPORT
bool Audit_Log_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
void *Audit_Log_Context_Get(uint32_t object_instance);
BACNET_STACK_EXPORT
void Audit_Log_Context_Set(uint32_t object_instance, void *context);

BACNET_STACK_EXPORT
uint32_t Audit_Log_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Audit_Log_Delete(uint32_t object_instance);

BACNET_STACK_EXPORT
void Audit_Log_Cleanup(void);
BACNET_STACK_EXPORT
void Audit_Log_Init(void);

BACNET_STACK_EXPORT
uint32_t Audit_Log_Buffer_Size(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Audit_Log_Buffer_Size_Set(uint32_t object_instance, uint32_t buffer_size);

BACNET_STACK_EXPORT
uint32_t Audit_Log_Record_Count(uint32_t object_instance);
BACNET_STACK_EXPORT
uint32_t Audit_Log_Total_Record_Count(uint32_t object_instance);

BACNET_STACK_EXPORT
void Audit_Log_Record_Status_Insert(
    uint32_t instance, BACNET_LOG_STATUS eStatus, bool bState);
BACNET_STACK_EXPORT
void Audit_Log_Record_Notification_Insert(
    uint32_t instance, BACNET_AUDIT_NOTIFICATION *notification);
BACNET_STACK_EXPORT
BACNET_AUDIT_LOG_RECORD *
Audit_Log_Record_Entry(uint32_t object_instance, uint32_t index);
BACNET_STACK_EXPORT
void Audit_Log_Record_Entry_Delete(uint32_t object_instance, uint32_t index);
BACNET_STACK_EXPORT
bool Audit_Log_Record_Entry_Add(
    uint32_t object_instance, const BACNET_AUDIT_LOG_RECORD *value);

BACNET_STACK_EXPORT
bool Audit_Log_Enable(uint32_t instance);
BACNET_STACK_EXPORT
bool Audit_Log_Enable_Set(uint32_t object_instance, bool enable);

BACNET_STACK_EXPORT
int Audit_Log_Read_Range_By_Position(BACNET_READ_RANGE_DATA *pRequest);
BACNET_STACK_EXPORT
int Audit_Log_Read_Range_By_Sequence(BACNET_READ_RANGE_DATA *pRequest);
BACNET_STACK_EXPORT
int Audit_Log_Read_Range_By_Time(BACNET_READ_RANGE_DATA *pRequest);
BACNET_STACK_EXPORT
int Audit_Log_Read_Range(uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
