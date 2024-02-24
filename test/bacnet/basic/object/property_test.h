/**
 * @file
 * @brief Unit test for object property read/write
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2024
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef _BACNET_PROPERTY_TEST_H_
#define _BACNET_PROPERTY_TEST_H_
#include <stdint.h>
#include <bacnet/rp.h>
#include <bacnet/rpm.h>
#include <bacnet/wp.h>

void bacnet_object_properties_read_write_test(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    rpm_property_lists_function property_list,
    read_property_function read_property,
    write_property_function write_property,
    const int *known_fail_property_list);

void bacnet_object_property_read_write_test(
    BACNET_READ_PROPERTY_DATA *rpdata,
    read_property_function read_property,
    write_property_function write_property,
    const int *known_fail_property_list);

#endif
