/**
 * @file
 * @brief BACnet ZigBee Link Layer Handler
 * @author Luiz Santana <luiz.santana@dsr-corporation.com>
 * @date April 2026
 * @copyright SPDX-License-Identifier: MIT
 */

#include <stdio.h> /* for standard i/o, like printing */
#include <stdint.h> /* for standard integer types uint8_t etc. */
#include <stdbool.h> /* for the standard bool type. */
#include <string.h> /* for memcpy */
#include <assert.h>
/* me!*/
#include "support.h"

/* Pointer to the device info to be used during the tests*/
struct device_info_t *device_info_ptr = NULL;

/**
 * @brief Set pointer of the device info struct to the test
 * @param device_info [in] - pointer to the device info pointer
 */
void support_set_device_info(struct device_info_t *device_info)
{
    device_info_ptr = device_info;
}

/**
 * @brief Return the Object Instance number for our (single) Device Object.
 * This is a key function, widely invoked by the handler code, since
 * it provides "our" (ie, local) address.
 *
 * @return The Instance number used in the BACNET_OBJECT_ID for the Device.
 */
uint32_t Device_Object_Instance_Number(void)
{
    return device_info_ptr->Device_ID;
}

/**
 * @brief Function to test the change the Object Instance
 * @param object_id [in] - new object id
 * @return true if success
 */
bool Device_Set_Object_Instance_Number(uint32_t object_id)
{
    bool status = (object_id == device_info_ptr->Device_ID);
    assert(status);
    return status;
}
