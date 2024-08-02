/**
 * @file
 * @brief mock Device object functions
 * @author Mikhail Antropov
 * @date July 2022
 *
 * SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bactext.h>
#include <bacnet/basic/services.h>

int handler_device_read_property(BACNET_READ_PROPERTY_DATA *rpdata)
{
    (void)rpdata;
    return 0;
}

uint32_t handler_device_object_instance_number(void)
{
    return 0;
}
