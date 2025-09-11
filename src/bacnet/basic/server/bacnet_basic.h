/**
 * @file
 * @brief BACnet Basic Stack initialization and basic task handler
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date April 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_H
#define BACNET_BASIC_H

#include <stdint.h>
#include <stdbool.h>
#include "bacnet/bacdef.h"

/**
 * @brief Callback function for BACnet initialization and task
 * @param context - pointer to the context
 */
typedef void (*bacnet_basic_callback)(void *context);

/**
 * @brief Store the BACnet data after a WriteProperty for object property
 * @param object_type - BACnet object type
 * @param object_instance - BACnet object instance
 * @param object_property - BACnet object property
 * @param array_index - BACnet array index
 * @param application_data - pointer to the data
 * @param application_data_len - length of the data
 */
typedef void (*bacnet_basic_store_callback)(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_ID object_property,
    BACNET_ARRAY_INDEX array_index,
    uint8_t *application_data,
    int application_data_len);

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void bacnet_basic_init(void);
BACNET_STACK_EXPORT
void bacnet_basic_init_callback_set(
    bacnet_basic_callback callback, void *context);

BACNET_STACK_EXPORT
void bacnet_basic_task(void);
BACNET_STACK_EXPORT
void bacnet_basic_task_callback_set(
    bacnet_basic_callback callback, void *context);
BACNET_STACK_EXPORT
void bacnet_basic_task_object_timer_set(unsigned long milliseconds);

BACNET_STACK_EXPORT
void bacnet_basic_store_callback_set(bacnet_basic_store_callback callback);

BACNET_STACK_EXPORT
unsigned long bacnet_basic_uptime_seconds(void);
BACNET_STACK_EXPORT
unsigned long bacnet_basic_packet_count(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
