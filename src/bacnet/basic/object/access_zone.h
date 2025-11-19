/**
 * @file
 * @brief A basic BACnet Access Zone Objects implementation.
 * @author Nikola Jelic <nikola.jelic@euroicc.com>
 * @date 2015
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_ACCESS_ZONE_H
#define BACNET_BASIC_OBJECT_ACCESS_ZONE_H

#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacerror.h"
#include "bacnet/bacdevobjpropref.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

#ifndef MAX_ACCESS_ZONES
#define MAX_ACCESS_ZONES 4
#endif

#ifndef MAX_ACCESS_ZONE_ENTRY_POINTS
#define MAX_ACCESS_ZONE_ENTRY_POINTS 4
#endif

#ifndef MAX_ACCESS_ZONE_EXIT_POINTS
#define MAX_ACCESS_ZONE_EXIT_POINTS 4
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct {
    uint32_t global_identifier;
    BACNET_ACCESS_ZONE_OCCUPANCY_STATE occupancy_state;
    BACNET_EVENT_STATE event_state;
    BACNET_RELIABILITY reliability;
    bool out_of_service;
    uint32_t entry_points_count, exit_points_count;
    BACNET_DEVICE_OBJECT_REFERENCE
    entry_points[MAX_ACCESS_ZONE_ENTRY_POINTS];
    BACNET_DEVICE_OBJECT_REFERENCE
    exit_points[MAX_ACCESS_ZONE_EXIT_POINTS];
} ACCESS_ZONE_DESCR;

BACNET_STACK_EXPORT
void Access_Zone_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary);
BACNET_STACK_EXPORT
bool Access_Zone_Valid_Instance(uint32_t object_instance);
unsigned Access_Zone_Count(void);
BACNET_STACK_EXPORT
uint32_t Access_Zone_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Access_Zone_Instance_To_Index(uint32_t instance);
BACNET_STACK_EXPORT
bool Access_Zone_Object_Instance_Add(uint32_t instance);

BACNET_STACK_EXPORT
bool Access_Zone_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool Access_Zone_Name_Set(uint32_t object_instance, char *new_name);

BACNET_STACK_EXPORT
bool Access_Zone_Out_Of_Service(uint32_t instance);
BACNET_STACK_EXPORT
void Access_Zone_Out_Of_Service_Set(uint32_t instance, bool oos_flag);

BACNET_STACK_EXPORT
int Access_Zone_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);
BACNET_STACK_EXPORT
bool Access_Zone_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
uint32_t Access_Zone_Create(uint32_t object_instance);
BACNET_STACK_EXPORT
bool Access_Zone_Delete(uint32_t object_instance);
BACNET_STACK_EXPORT
void Access_Zone_Cleanup(void);
BACNET_STACK_EXPORT
void Access_Zone_Init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
