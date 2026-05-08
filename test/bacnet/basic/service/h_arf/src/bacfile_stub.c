/**
 * @file
 * @brief Stubs for BACnet File object functions used by
 * handler_atomic_read_file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/basic/object/bacfile.h"

/** Control variable: set true to make bacfile_valid_instance() return true */
bool Bacfile_Valid_Instance_Result = false;

/**
 * @brief Stub: report whether the file instance is valid
 */
bool bacfile_valid_instance(uint32_t object_instance)
{
    (void)object_instance;
    return Bacfile_Valid_Instance_Result;
}

/** Control variable: set true to make bacfile_read_record_data() succeed */
bool Bacfile_Read_Record_Data_Result = false;

/**
 * @brief Stub: simulate reading record data from a file
 */
bool bacfile_read_record_data(BACNET_ATOMIC_READ_FILE_DATA *data)
{
    (void)data;
    return Bacfile_Read_Record_Data_Result;
}

/**
 * @brief Stub: simulate reading stream data from a file (always succeeds)
 */
bool bacfile_read_stream_data(BACNET_ATOMIC_READ_FILE_DATA *data)
{
    (void)data;
    return true;
}
