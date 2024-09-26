/**
 * @file
 * @brief Handle Get/Set of non-volatile data
 * @date July 8, 2015
 * @author Steve Karg
 */
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacint.h"
#include "bacnet/bacstr.h"
#include "nvdata.h"
#include "eeprom.h"

/**
 * Gets 8-byte value from non-volatile memory
 *
 * @return device ID
 */
uint64_t nvdata_unsigned64(uint32_t ee_address)
{
    uint64_t long_value = 0;
    uint8_t buffer[8];

    if (eeprom_bytes_read(ee_address, buffer, 8) == 8) {
        decode_unsigned64(buffer, &long_value);
    }

    return long_value;
}

/**
 * Sets a 8-byte value in non-volatile memory
 *
 * @param long_value 8-byte value
 */
int nvdata_unsigned64_set(uint32_t ee_address, uint64_t long_value)
{
    uint8_t buffer[8];

    encode_unsigned64(buffer, long_value);
    return eeprom_bytes_write(ee_address, buffer, 8);
}

/**
 * Gets a 4-byte value from non-volatile memory
 *
 * @return device ID
 */
uint32_t nvdata_unsigned32(uint32_t ee_address)
{
    uint32_t long_value = 0;
    uint8_t buffer[4];

    if (eeprom_bytes_read(ee_address, buffer, 4) == 4) {
        decode_unsigned32(buffer, &long_value);
    }

    return long_value;
}

/**
 * Sets a 4-byte value in non-volatile memory
 *
 * @param long_value 4-byte value
 */
int nvdata_unsigned32_set(uint32_t ee_address, uint32_t long_value)
{
    uint8_t buffer[4];

    encode_unsigned32(buffer, long_value);
    return eeprom_bytes_write(ee_address, buffer, 4);
}

/**
 * Gets a 3-byte value from non-volatile memory
 *
 * @return device ID
 */
uint32_t nvdata_unsigned24(uint32_t ee_address)
{
    uint32_t long_value = 0;
    uint8_t buffer[3];

    if (eeprom_bytes_read(ee_address, buffer, 3) == 3) {
        decode_unsigned24(buffer, &long_value);
    }

    return long_value;
}

/**
 * Sets a 3-byte value in non-volatile memory
 *
 * @param long_value 3-byte value
 */
int nvdata_unsigned24_set(uint32_t ee_address, uint32_t long_value)
{
    uint8_t buffer[3];

    encode_unsigned24(buffer, long_value);
    return eeprom_bytes_write(ee_address, buffer, 3);
}

/**
 * Gets a 2-byte value from non-volatile memory
 *
 * @return device ID
 */
uint16_t nvdata_unsigned16(uint32_t ee_address)
{
    uint16_t short_value = 0;
    uint8_t buffer[2];

    if (eeprom_bytes_read(ee_address, buffer, 2) == 2) {
        decode_unsigned16(buffer, &short_value);
    }

    return short_value;
}

/**
 * Sets a 2-byte value in non-volatile memory
 *
 * @param short_value 2-byte value
 */
int nvdata_unsigned16_set(uint32_t ee_address, uint16_t short_value)
{
    uint8_t buffer[2];

    encode_unsigned16(buffer, short_value);
    return eeprom_bytes_write(ee_address, buffer, 2);
}

/**
 * Gets a 1-byte value from non-volatile memory
 *
 * @return device ID
 */
uint8_t nvdata_unsigned8(uint32_t ee_address)
{
    uint8_t buffer = 0;

    if (eeprom_bytes_read(ee_address, &buffer, 1) == 1) {
        /* something useful */
    }

    return buffer;
}

/**
 * Sets a 1-byte value in non-volatile memory
 *
 * @param byte_value 1-byte value
 */
int nvdata_unsigned8_set(uint32_t ee_address, uint8_t byte_value)
{
    return eeprom_bytes_write(ee_address, &byte_value, 1);
}

bool nvdata_name_isvalid(uint8_t encoding, uint8_t length, const char *str)
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

bool nvdata_name_set(
    uint16_t offset, uint8_t encoding, const char *str, uint8_t length)
{
    uint8_t buffer[NV_EEPROM_NAME_SIZE] = { 0 };
    uint8_t i = 0;

    if (nvdata_name_isvalid(encoding, length, str)) {
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

/**
 * @brief Read a name from the EEPROM
 * @param offset - starting memory address
 * @param encoding - character encoding
 * @param value - place to store the name
 * @param value_size - size of the name buffer
 * @return actual length of the name
 */
uint8_t nvdata_name(
    uint16_t offset, uint8_t *encoding_out, char *value, uint8_t value_size)
{
    uint8_t length = 0, encoding = 0;
    char name[NV_EEPROM_NAME_SIZE + 1] = "";

    eeprom_bytes_read(NV_EEPROM_NAME_ENCODING(offset), &encoding, 1);
    eeprom_bytes_read(NV_EEPROM_NAME_LENGTH(offset), &length, 1);
    if (length > value_size) {
        length = value_size;
    }
    if (length > NV_EEPROM_NAME_SIZE) {
        length = NV_EEPROM_NAME_SIZE;
    }
    eeprom_bytes_read(
        NV_EEPROM_NAME_STRING(offset), (uint8_t *)name, NV_EEPROM_NAME_SIZE);
    if (nvdata_name_isvalid(encoding, length, name)) {
        memmove(value, name, length);
        if (encoding_out) {
            *encoding_out = encoding;
        }
    } else {
        length = 0;
    }

    return length;
}

/**
 * Read bytes from SEEPROM memory at address
 *
 * @param start_address - SEEPROM starting memory address
 * @param buffer - place to store data that has been read
 * @param length - number of bytes to read
 *
 * @return number of bytes read, or 0 on error
 */
uint32_t nvdata_get(uint32_t ee_address, uint8_t *buffer, uint32_t nbytes)
{
    return eeprom_bytes_read(ee_address, buffer, nbytes);
}

/**
 * Write some data and wait until it is sent
 *
 * @param ee_address - SEEPROM starting memory address
 * @param buffer - data to send
 * @param length - number of bytes of data
 *
 * @return number of bytes written, or 0 on error
 */
uint32_t
nvdata_set(uint32_t ee_address, uint8_t *buffer, uint32_t buffer_length)
{
    return eeprom_bytes_write(ee_address, buffer, buffer_length);
}
