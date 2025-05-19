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
#include "bacnet/datetime.h"
#include "bacnet/bacerror.h"
#include "bacnet/wp.h"
#include "bacnet/rp.h"
#include "bacnet/bacdevobjpropref.h"
#include "bacnet/bactimevalue.h"
#include "bacnet/dailyschedule.h"
#include "bacnet/special_event.h"

#ifndef BACNET_SCHEDULE_OBJ_PROP_REF_SIZE
/* Maximum number of obj prop references */
#define BACNET_SCHEDULE_OBJ_PROP_REF_SIZE 4
#endif

#ifndef BACNET_EXCEPTION_SCHEDULE_SIZE
/* Maximum number of special events */
#define BACNET_EXCEPTION_SCHEDULE_SIZE 8
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct schedule {
    /* Effective Period: Start and End Date */
    BACNET_DATE Start_Date;
    BACNET_DATE End_Date;
    /* Properties concerning Present Value */
    BACNET_DAILY_SCHEDULE Weekly_Schedule[BACNET_WEEKLY_SCHEDULE_SIZE];
#if BACNET_EXCEPTION_SCHEDULE_SIZE
    BACNET_SPECIAL_EVENT Exception_Schedule[BACNET_EXCEPTION_SCHEDULE_SIZE];
#endif
    BACNET_APPLICATION_DATA_VALUE Schedule_Default;
    /*
     * Caution: This is a converted to BACNET_PRIMITIVE_APPLICATION_DATA_VALUE.
     * Only some data types may be used!
     *
     * Must be set to a valid value. Default is Schedule_Default.
     */
    BACNET_APPLICATION_DATA_VALUE Present_Value;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE
    Object_Property_References[BACNET_SCHEDULE_OBJ_PROP_REF_SIZE];
    uint8_t obj_prop_ref_cnt; /* actual number of obj_prop references */
    uint8_t Priority_For_Writing; /* (1..16) */
    bool Out_Of_Service;
} SCHEDULE_DESCR;

BACNET_STACK_EXPORT
void Schedule_Property_Lists(
    const int **pRequired, const int **pOptional, const int **pProprietary);

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
void Schedule_Out_Of_Service_Set(uint32_t object_instance, bool value);
BACNET_STACK_EXPORT
bool Schedule_Out_Of_Service(uint32_t object_instance);

BACNET_STACK_EXPORT
BACNET_DAILY_SCHEDULE *
Schedule_Weekly_Schedule(uint32_t object_instance, unsigned array_index);
BACNET_STACK_EXPORT
bool Schedule_Weekly_Schedule_Set(
    uint32_t object_instance,
    unsigned array_index,
    const BACNET_DAILY_SCHEDULE *value);

BACNET_STACK_EXPORT
BACNET_SPECIAL_EVENT *
Schedule_Exception_Schedule(uint32_t object_instance, unsigned array_index);
BACNET_STACK_EXPORT
bool Schedule_Exception_Schedule_Set(
    uint32_t object_instance,
    unsigned array_index,
    const BACNET_SPECIAL_EVENT *value);

BACNET_STACK_EXPORT
bool Schedule_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);

BACNET_STACK_EXPORT
int Schedule_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
BACNET_STACK_EXPORT
bool Schedule_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

/* utility functions for calculating current Present Value
 * if Exception Schedule is to be added, these functions must take that into
 * account */
BACNET_STACK_EXPORT
bool Schedule_In_Effective_Period(
    const SCHEDULE_DESCR *desc, const BACNET_DATE *date);
BACNET_STACK_EXPORT
void Schedule_Recalculate_PV(
    SCHEDULE_DESCR *desc, BACNET_WEEKDAY wday, const BACNET_TIME *time);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
