/**
 * @file
 * @brief Stubs for BACnet File object functions used by
 * handler_atomic_write_file
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

/** Control variable: set true to make bacfile_write_stream_data() succeed */
bool Bacfile_Write_Stream_Data_Result = false;

/**
 * @brief Stub: simulate writing stream data to a file
 */
bool bacfile_write_stream_data(BACNET_ATOMIC_WRITE_FILE_DATA *data)
{
    (void)data;
    return Bacfile_Write_Stream_Data_Result;
}

/** Control variable: set true to make bacfile_write_record_data() succeed */
bool Bacfile_Write_Record_Data_Result = false;

/**
 * @brief Stub: simulate writing record data to a file
 */
bool bacfile_write_record_data(const BACNET_ATOMIC_WRITE_FILE_DATA *data)
{
    (void)data;
    return Bacfile_Write_Record_Data_Result;
}
