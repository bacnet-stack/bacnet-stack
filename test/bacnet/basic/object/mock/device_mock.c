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
#include <bacnet/basic/object/device.h>


bool Device_Valid_Object_Name(
    BACNET_CHARACTER_STRING * object_name,
    BACNET_OBJECT_TYPE *object_type,
    uint32_t * object_instance)
{
    return true;
}

void Device_Inc_Database_Revision(
    void)
{

}
