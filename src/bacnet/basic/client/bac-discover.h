/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2024
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef BAC_DISCOVER_H
#define BAC_DISCOVER_H

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacapp.h"
#include "bacnet/bacnet_stack_exports.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void bacnet_discover_cleanup(void);

BACNET_STACK_EXPORT
int bacnet_discover_device_count(void);
BACNET_STACK_EXPORT
uint32_t bacnet_discover_device_instance(unsigned index);
BACNET_STACK_EXPORT
int bacnet_discover_device_object_count(uint32_t device_id);
BACNET_STACK_EXPORT
bool bacnet_discover_device_object_identifier(uint32_t device_id, 
    unsigned index, BACNET_OBJECT_ID *object_id);
BACNET_STACK_EXPORT
unsigned long bacnet_discover_device_elapsed_milliseconds(
    uint32_t device_id);
BACNET_STACK_EXPORT
unsigned int bacnet_discover_object_property_count(
    uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance);
BACNET_STACK_EXPORT
bool bacnet_discover_object_property_identifier(
    uint32_t device_id, 
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    unsigned index, 
    uint32_t *property_id);
BACNET_STACK_EXPORT
bool bacnet_discover_property_value(uint32_t device_id,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    BACNET_APPLICATION_DATA_VALUE *value);

BACNET_STACK_EXPORT
void bacnet_discover_task(void);

BACNET_STACK_EXPORT
void bacnet_discover_dnet_set(uint16_t dnet);
BACNET_STACK_EXPORT
uint16_t bacnet_discover_dnet(void);

BACNET_STACK_EXPORT
void bacnet_discover_vendor_id_set(uint16_t vendor_id);
BACNET_STACK_EXPORT
uint16_t bacnet_discover_vendor_id(void);

BACNET_STACK_EXPORT
void bacnet_discover_seconds_set(unsigned int seconds);
BACNET_STACK_EXPORT
unsigned int bacnet_discover_seconds(void);

BACNET_STACK_EXPORT
void bacnet_discover_read_process_milliseconds_set(unsigned long milliseconds);
BACNET_STACK_EXPORT
unsigned long bacnet_discover_read_process_milliseconds(void);

BACNET_STACK_EXPORT
void bacnet_discover_device_add(
    uint32_t device_instance,
    unsigned max_apdu,
    int segmentation,
    uint16_t vendor_id);

BACNET_STACK_EXPORT
void bacnet_discover_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
