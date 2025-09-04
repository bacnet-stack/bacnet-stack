/**
 * @file
 * @brief Unit test for object property read/write
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2024
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef _BACNET_PROPERTY_TEST_H_
#define _BACNET_PROPERTY_TEST_H_
#include <stdint.h>
#include <bacnet/rp.h>
#include <bacnet/rpm.h>
#include <bacnet/wp.h>

/* function API pattern for testing ASCII name get/set */
typedef bool (*object_name_ascii_set_function)(
    uint32_t object_instance, const char *new_name);
typedef const char *(*object_name_ascii_function)(uint32_t object_instance);

void bacnet_object_name_ascii_test(
    uint32_t object_instance,
    object_name_ascii_set_function ascii_set,
    object_name_ascii_function ascii_get);

void bacnet_object_properties_read_write_test(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    rpm_property_lists_function property_list,
    read_property_function read_property,
    write_property_function write_property,
    const int *skip_fail_property_list);

int bacnet_object_property_read_test(
    BACNET_READ_PROPERTY_DATA *rpdata,
    read_property_function read_property,
    const int *skip_fail_property_list);

bool bacnet_object_property_write_test(
    BACNET_WRITE_PROPERTY_DATA *wpdata,
    write_property_function write_property,
    const int *skip_fail_property_list);

void bacnet_object_property_write_parameter_init(
    BACNET_WRITE_PROPERTY_DATA *wpdata,
    BACNET_READ_PROPERTY_DATA *rpdata,
    int len);

#endif
