/**
 * @file
 * @author Steve Karg
 * @date February 2024
 * @brief Header file for a basic BACnet Device object handler
 * @copyright
 *  Copyright 2024 by Steve Karg <skarg@users.sourceforge.net>
 *  SPDX-License-Identifier: MIT
 */
#ifndef BACNET_DEVICE_HANDLER_H
#define BACNET_DEVICE_HANDLER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet core library */
#include "bacnet/create_object.h"
#include "bacnet/delete_object.h"
#include "bacnet/list_element.h"
#include "bacnet/readrange.h"
#include "bacnet/rd.h"
#include "bacnet/rp.h"
#include "bacnet/rpm.h"
#include "bacnet/wp.h"

/** Called so a BACnet object can perform any necessary initialization.
 * @ingroup ObjHelpers
 */
typedef void (*object_init_function)(void);

/** Counts the number of objects of this type.
 * @ingroup ObjHelpers
 * @return Count of implemented objects of this type.
 */
typedef unsigned (*object_count_function)(void);

/** Maps an object index position to its corresponding BACnet object instance
 * number.
 * @ingroup ObjHelpers
 * @param index [in] The index of the object, in the array of objects of its
 * type.
 * @return The BACnet object instance number to be used in a BACNET_OBJECT_ID.
 */
typedef uint32_t (*object_index_to_instance_function)(unsigned index);

/** Provides the BACnet Object_Name for a given object instance of this type.
 * @ingroup ObjHelpers
 * @param object_instance [in] The object instance number to be looked up.
 * @param object_name [in,out] Pointer to a character_string structure that
 *         will hold a copy of the object name if this is a valid
 * object_instance.
 * @return True if the object_instance is valid and object_name has been
 *         filled with a copy of the Object's name.
 */
typedef bool (*object_name_function)(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);

/** Look in the table of objects of this type, and see if this is a valid
 *  instance number.
 * @ingroup ObjHelpers
 * @param [in] The object instance number to be looked up.
 * @return True if the object instance refers to a valid object of this type.
 */
typedef bool (*object_valid_instance_function)(uint32_t object_instance);

/** Helper function to step through an array of objects and find either the
 * first one or the next one of a given type. Used to step through an array
 * of objects which is not necessarily contiguous for each type i.e. the
 * index for the 'n'th object of a given type is not necessarily 'n'.
 * @ingroup ObjHelpers
 * @param [in] The index of the current object or a value of ~0 to indicate
 * start at the beginning.
 * @return The index of the next object of the required type or ~0 (all bits
 * == 1) to indicate no more objects found.
 */
typedef unsigned (*object_iterate_function)(unsigned current_index);

/** Look in the table of objects of this type, and get the COV Value List.
 * @ingroup ObjHelpers
 * @param [in] The object instance number to be looked up.
 * @param [out] The value list
 * @return True if the object instance supports this feature, and has changed.
 */
typedef bool (*object_value_list_function)(
    uint32_t object_instance, BACNET_PROPERTY_VALUE *value_list);

/** Look in the table of objects for this instance to see if value changed.
 * @ingroup ObjHelpers
 * @param [in] The object instance number to be looked up.
 * @return True if the object instance has changed.
 */
typedef bool (*object_cov_function)(uint32_t object_instance);

/** Look in the table of objects for this instance to clear the changed flag.
 * @ingroup ObjHelpers
 * @param [in] The object instance number to be looked up.
 */
typedef void (*object_cov_clear_function)(uint32_t object_instance);

/** Intrinsic Reporting funcionality.
 * @ingroup ObjHelpers
 * @param [in] Object instance.
 */
typedef void (*object_intrinsic_reporting_function)(uint32_t object_instance);

/**
 * @brief Updates the object with the elapsed milliseconds
 * @param  object_instance - object-instance number of the object
 * @param milliseconds - number of milliseconds elapsed
 */
typedef void (*object_timer_function)(
    uint32_t object_instance, uint16_t milliseconds);

/** Defines the group of object helper functions for any supported Object.
 * @ingroup ObjHelpers
 * Each Object must provide some implementation of each of these helpers
 * in order to properly support the handlers.  Eg, the ReadProperty handler
 * handler_read_property() relies on the instance of Object_Read_Property
 * for each Object type, or configure the function as NULL.
 * In both appearance and operation, this group of functions acts like
 * they are member functions of a C++ Object base class.
 */
typedef struct object_functions {
    BACNET_OBJECT_TYPE Object_Type;
    object_init_function Object_Init;
    object_count_function Object_Count;
    object_index_to_instance_function Object_Index_To_Instance;
    object_valid_instance_function Object_Valid_Instance;
    object_name_function Object_Name;
    read_property_function Object_Read_Property;
    write_property_function Object_Write_Property;
    rpm_property_lists_function Object_RPM_List;
    rr_info_function Object_RR_Info;
    object_iterate_function Object_Iterator;
    object_value_list_function Object_Value_List;
    object_cov_function Object_COV;
    object_cov_clear_function Object_COV_Clear;
    object_intrinsic_reporting_function Object_Intrinsic_Reporting;
    list_element_function Object_Add_List_Element;
    list_element_function Object_Remove_List_Element;
    create_object_function Object_Create;
    delete_object_function Object_Delete;
    object_timer_function Object_Timer;
} object_functions_t;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
uint32_t handler_device_object_instance_number(void);
BACNET_STACK_EXPORT
void handler_device_object_instance_number_set(uint32_t device_id);

BACNET_STACK_EXPORT
uint32_t handler_device_wildcard_instance_number(
    BACNET_OBJECT_TYPE object_type, uint32_t object_instance);

BACNET_STACK_EXPORT
uint32_t handler_device_object_database_revision(void);
BACNET_STACK_EXPORT
void handler_device_object_database_revision_set(uint32_t database_revision);
BACNET_STACK_EXPORT
void handler_device_object_database_revision_increment(void);

BACNET_STACK_EXPORT
bool handler_device_reinitialize_password_set(const char *password);
BACNET_STACK_EXPORT
bool handler_device_reinitialize(BACNET_REINITIALIZE_DEVICE_DATA *rd_data);
BACNET_STACK_EXPORT
BACNET_REINITIALIZED_STATE handler_device_reinitialized_state(void);
BACNET_STACK_EXPORT
void handler_device_reinitialized_state_set(BACNET_REINITIALIZED_STATE state);

BACNET_STACK_EXPORT
uint16_t handler_device_vendor_identifier(void);
BACNET_STACK_EXPORT
void handler_device_vendor_identifier_set(uint16_t vendor_id);

BACNET_STACK_EXPORT
void handler_device_reinitialize_backup_restore_enabled_set(bool enable);
BACNET_STACK_EXPORT
bool handler_device_reinitialize_backup_restore_enabled(void);

BACNET_STACK_EXPORT
rr_info_function
handler_device_object_read_range_info(BACNET_OBJECT_TYPE object_type);

BACNET_STACK_EXPORT
void handler_device_object_property_list(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    struct special_property_list_t *pPropertyList);
BACNET_STACK_EXPORT
bool handler_device_object_property_list_member(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    int object_property);

BACNET_STACK_EXPORT
bool handler_device_object_instance_valid(
    BACNET_OBJECT_TYPE object_type, uint32_t object_instance);

BACNET_STACK_EXPORT
bool handler_device_write_property(BACNET_WRITE_PROPERTY_DATA *wp_data);

BACNET_STACK_EXPORT
bool handler_device_object_list_identifier(
    uint32_t array_index, BACNET_OBJECT_TYPE *object_type, uint32_t *instance);
BACNET_STACK_EXPORT
unsigned handler_device_object_list_count(void);
BACNET_STACK_EXPORT
int handler_device_object_list_element_encode(
    uint32_t object_instance, BACNET_ARRAY_INDEX array_index, uint8_t *apdu);

BACNET_STACK_EXPORT
int handler_device_object_list_element_add(
    BACNET_LIST_ELEMENT_DATA *list_element);
BACNET_STACK_EXPORT
int handler_device_object_list_element_remove(
    BACNET_LIST_ELEMENT_DATA *list_element);

BACNET_STACK_EXPORT
bool handler_device_valid_object_name(
    const BACNET_CHARACTER_STRING *object_name1,
    BACNET_OBJECT_TYPE *object_type,
    uint32_t *object_instance);
BACNET_STACK_EXPORT
bool handler_device_object_name_copy(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *object_name);
BACNET_STACK_EXPORT
bool handler_device_valid_object_instance(
    BACNET_OBJECT_TYPE object_type, uint32_t object_instance);

BACNET_STACK_EXPORT
void handler_device_services_supported(BACNET_BIT_STRING *bit_string);

BACNET_STACK_EXPORT
void handler_device_object_types_supported(BACNET_BIT_STRING *bit_string);

BACNET_STACK_EXPORT
void handler_device_intrinsic_reporting(void);

BACNET_STACK_EXPORT
bool handler_device_object_value_list_supported(BACNET_OBJECT_TYPE object_type);
BACNET_STACK_EXPORT
bool handler_device_object_value_list(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_VALUE *value_list);
BACNET_STACK_EXPORT
bool handler_device_object_cov(
    BACNET_OBJECT_TYPE object_type, uint32_t object_instance);
BACNET_STACK_EXPORT
void handler_device_object_cov_clear(
    BACNET_OBJECT_TYPE object_type, uint32_t object_instance);

BACNET_STACK_EXPORT
int handler_device_read_property_default(BACNET_READ_PROPERTY_DATA *rpdata);
BACNET_STACK_EXPORT
int handler_device_read_property_common(
    struct object_functions *pObject, BACNET_READ_PROPERTY_DATA *rpdata);
BACNET_STACK_EXPORT
int handler_device_read_property(BACNET_READ_PROPERTY_DATA *rpdata);

BACNET_STACK_EXPORT
bool handler_device_object_create(BACNET_CREATE_OBJECT_DATA *data);
BACNET_STACK_EXPORT
bool handler_device_object_delete(BACNET_DELETE_OBJECT_DATA *data);

BACNET_STACK_EXPORT
void handler_device_timer(uint16_t milliseconds);

BACNET_STACK_EXPORT
void handler_device_object_table_set(object_functions_t *object_table);

BACNET_STACK_EXPORT
void handler_device_object_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
