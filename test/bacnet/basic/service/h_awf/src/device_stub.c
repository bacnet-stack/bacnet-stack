/**
 * @file
 * @brief Stubs for the Device object backup and restore functions used by
 *  the file access service handlers
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/basic/object/device.h"

/** The File object instance that is a configuration file */
const uint32_t Device_Stub_Configuration_File = 7;
/** Count of Device_Backup_Failure_Timeout_Restart() calls */
unsigned Device_Stub_Restart_Count;

bool Device_Is_Configuration_File(uint32_t instance)
{
    return instance == Device_Stub_Configuration_File;
}

void Device_Backup_Failure_Timeout_Restart(void)
{
    Device_Stub_Restart_Count++;
}
