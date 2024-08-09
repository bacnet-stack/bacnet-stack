/**
 * @file
 * @brief API for BACnet device name handling for the Xplained board
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2010
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_NAME_H
#define BACNET_NAME_H

#include <stdint.h>
#include "bacnet/bacstr.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    int bacnet_name_copy(
        uint16_t offset,
        uint8_t *dest,
        uint8_t dest_len);
    bool bacnet_name_set(
        uint16_t eeprom_offset,
        const BACNET_CHARACTER_STRING * char_string);
    void bacnet_name_init(
        uint16_t eeprom_offset,
        const char *default_string);
    uint8_t bacnet_name_encode(
        uint8_t *buffer,
        uint8_t buffer_len,
        uint8_t encoding,
        const char *str,
        uint8_t str_len);
    bool bacnet_name_save(
        uint16_t offset,
        uint8_t encoding,
        const char *str,
        uint8_t str_len);
    void bacnet_name(
        uint16_t eeprom_offset,
        BACNET_CHARACTER_STRING * char_string,
        const char *default_string);
    bool bacnet_name_write_unique(
        uint16_t offset,
        int object_type,
        uint32_t object_instance,
        const BACNET_CHARACTER_STRING * char_string,
        BACNET_ERROR_CLASS * error_class,
        BACNET_ERROR_CODE * error_code);
    /* no required minumum length or duplicate checking */
    bool bacnet_name_write(
        uint16_t offset,
        const BACNET_CHARACTER_STRING * char_string,
        BACNET_ERROR_CLASS * error_class,
        BACNET_ERROR_CODE * error_code);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
