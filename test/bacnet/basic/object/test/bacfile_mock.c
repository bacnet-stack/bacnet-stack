/**
 * @file
 * @brief mock file object functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2026
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bactext.h>
#include <bacnet/basic/object/bacfile.h>

/**
 * @brief Sets the callback function for writing stream data
 * @param callback - function pointer to the callback
 */
void bacfile_write_stream_data_callback_set(
    size_t (*callback)(const char *, int32_t, const uint8_t *, size_t))
{
    (void)callback;
}

/**
 * @brief Sets the callback function for writing record data
 * @param callback - function pointer to the callback
 */
void bacfile_write_record_data_callback_set(
    bool (*callback)(const char *, int32_t, size_t, const uint8_t *, size_t))
{
    (void)callback;
}

/**
 * @brief Sets the callback function for reading record data
 * @param callback - function pointer to the callback
 */
void bacfile_read_record_data_callback_set(
    bool (*callback)(const char *, int32_t, size_t, uint8_t *, size_t))
{
    (void)callback;
}

/**
 * @brief Sets the callback function for reading stream data
 * @param callback - function pointer to the callback
 */
void bacfile_read_stream_data_callback_set(
    size_t (*callback)(const char *, int32_t, uint8_t *, size_t))
{
    (void)callback;
}

/**
 * @brief Sets the callback function for getting file size
 * @param callback - function pointer to the callback
 */
void bacfile_file_size_callback_set(size_t (*callback)(const char *))
{
    (void)callback;
}

/**
 * @brief Sets the callback function for setting file size
 * @param callback - function pointer to the callback
 */
void bacfile_file_size_set_callback_set(bool (*callback)(const char *, size_t))
{
    (void)callback;
}
