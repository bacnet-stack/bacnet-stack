/**************************************************************************
 *
 * Copyright (C) 2011 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "hardware.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacstr.h"
#include "nvdata.h"
#include "seeprom.h"
#include "bacnet/basic/object/device.h"
#include "bname.h"

static bool bacnet_name_isvalid(uint8_t encoding, uint8_t length, char *str)
{
    bool valid = false;

    if ((encoding < MAX_CHARACTER_STRING_ENCODING) &&
        (length <= NV_EEPROM_NAME_SIZE)) {
        if (encoding == CHARACTER_UTF8) {
            valid = utf8_isvalid(str, length);
        } else {
            valid = true;
        }
    }

    return valid;
}

bool bacnet_name_save(
    uint16_t offset, uint8_t encoding, char *str, uint8_t length)
{
    uint8_t buffer[NV_EEPROM_NAME_SIZE] = { 0 };
    uint8_t i = 0;

    if (bacnet_name_isvalid(encoding, length, str)) {
        eeprom_bytes_write(NV_EEPROM_NAME_LENGTH(offset), &length, 1);
        eeprom_bytes_write(
            NV_EEPROM_NAME_ENCODING(offset), (uint8_t *)&encoding, 1);
        for (i = 0; i < length; i++) {
            buffer[i] = str[i];
        }
        eeprom_bytes_write(
            NV_EEPROM_NAME_STRING(offset), &buffer[0], NV_EEPROM_NAME_SIZE);
        return true;
    }

    return false;
}

bool bacnet_name_set(uint16_t offset, BACNET_CHARACTER_STRING *char_string)
{
    uint8_t encoding = 0;
    uint8_t length = 0;
    char *str = NULL;

    length = characterstring_length(char_string);
    encoding = characterstring_encoding(char_string);
    str = characterstring_value(char_string);
    return bacnet_name_save(offset, encoding, str, length);
}

bool bacnet_name_write_unique(uint16_t offset,
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_CHARACTER_STRING *char_string,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    size_t length = 0;
    uint8_t encoding = 0;
    BACNET_OBJECT_TYPE duplicate_type = OBJECT_NONE;
    uint32_t duplicate_instance = 0;

    length = characterstring_length(char_string);
    if (length < 1) {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
    } else if (length <= NV_EEPROM_NAME_SIZE) {
        encoding = characterstring_encoding(char_string);
        if (encoding < MAX_CHARACTER_STRING_ENCODING) {
            if (Device_Valid_Object_Name(
                    char_string, &duplicate_type, &duplicate_instance)) {
                if ((duplicate_type == object_type) &&
                    (duplicate_instance == object_instance)) {
                    /* writing same name to same object */
                    status = true;
                } else {
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_DUPLICATE_NAME;
                }
            } else {
                status = bacnet_name_set(offset, char_string);
                if (status) {
                    Device_Inc_Database_Revision();
                } else {
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_CHARACTER_SET_NOT_SUPPORTED;
        }
    } else {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
    }

    return status;
}

/* no required minumum length or duplicate checking */
bool bacnet_name_write(uint16_t offset,
    BACNET_CHARACTER_STRING *char_string,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE *error_code)
{
    bool status = false;
    size_t length = 0;
    uint8_t encoding = 0;

    length = characterstring_length(char_string);
    if (length <= NV_EEPROM_NAME_SIZE) {
        encoding = characterstring_encoding(char_string);
        if (encoding < MAX_CHARACTER_STRING_ENCODING) {
            status = bacnet_name_set(offset, char_string);
            if (!status) {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
        } else {
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_CHARACTER_SET_NOT_SUPPORTED;
        }
    } else {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
    }

    return status;
}

void bacnet_name_init(uint16_t offset, char *default_string)
{
    (void)bacnet_name_save(
        offset, CHARACTER_UTF8, default_string, strlen(default_string));
}

void bacnet_name(
    uint16_t offset, BACNET_CHARACTER_STRING *char_string, char *default_string)
{
    uint8_t encoding = 0;
    uint8_t length = 0;
    char name[NV_EEPROM_NAME_SIZE + 1] = "";

    eeprom_bytes_read(NV_EEPROM_NAME_ENCODING(offset), &encoding, 1);
    eeprom_bytes_read(NV_EEPROM_NAME_LENGTH(offset), &length, 1);
    eeprom_bytes_read(
        NV_EEPROM_NAME_STRING(offset), (uint8_t *)&name, NV_EEPROM_NAME_SIZE);
    if (bacnet_name_isvalid(encoding, length, name)) {
        characterstring_init(char_string, encoding, &name[0], length);
    } else if (default_string) {
        bacnet_name_init(offset, default_string);
        characterstring_init_ansi(char_string, default_string);
    }
}
