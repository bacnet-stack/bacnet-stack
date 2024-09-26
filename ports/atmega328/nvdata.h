/**
 * @brief This module contains the map for internal AVR non-volatile data
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef NVDATA_H
#define NVDATA_H

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacdef.h"
#include "eeprom.h"

/*=============== EEPROM ================*/
#define NV_EEPROM_TYPE_ID 0xBAC0U
#define NV_EEPROM_VERSION_ID 1
#define NV_EEPROM_BYTES_MAX EEPROM_BYTES_MAX

/* note to developers: define each byte,
   even if they are not used explicitly */

/* EEPROM Type ID to indicate initialized or not */
#define NV_EEPROM_TYPE_0 0
#define NV_EEPROM_TYPE_1 1
/* EEPROM Version ID to indicate schema change */
#define NV_EEPROM_VERSION 2

/* EEPROM free space - 3..9 */

/* BACnet MS/TP datalink layer */
#define NV_EEPROM_MSTP_MAC 10
/* 9=9.6k, 19=19.2k, 38=38.4k, 57=57.6k, 76=76.8k, 115=115.2k */
#define NV_EEPROM_MSTP_BAUD_K 11
#define NV_EEPROM_MSTP_MAX_MASTER 12
/* device instance is only 22 bits */
#define NV_EEPROM_DEVICE_0 13
#define NV_EEPROM_DEVICE_1 14
#define NV_EEPROM_DEVICE_2 15

/* EEPROM free space - 16..31 */

/* BACnet Names - 32 bytes of data each */
#define NV_EEPROM_NAME_LENGTH(n) ((n) + 0)
#define NV_EEPROM_NAME_ENCODING(n) ((n) + 1)
#define NV_EEPROM_NAME_STRING(n) ((n) + 2)
#define NV_EEPROM_NAME_SIZE 30
#define NV_EEPROM_NAME_OFFSET (1 + 1 + NV_EEPROM_NAME_SIZE)
/* Device Name - starting offset */
#define NV_EEPROM_DEVICE_NAME 32
/* Device Description - starting offset  */
#define NV_EEPROM_DEVICE_DESCRIPTION \
    (NV_EEPROM_DEVICE_NAME + NV_EEPROM_NAME_OFFSET)
/* Device Location - starting offset  */
#define NV_EEPROM_DEVICE_LOCATION \
    (NV_EEPROM_DEVICE_DESCRIPTION + NV_EEPROM_NAME_OFFSET)

/* EEPROM free space 128..1024 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

uint64_t nvdata_unsigned64(uint32_t ee_address);
int nvdata_unsigned64_set(uint32_t ee_address, uint64_t long_value);
int nvdata_unsigned32_set(uint32_t ee_address, uint32_t long_value);
uint32_t nvdata_unsigned32(uint32_t ee_address);
int nvdata_unsigned24_set(uint32_t ee_address, uint32_t long_value);
uint32_t nvdata_unsigned24(uint32_t ee_address);
uint16_t nvdata_unsigned16(uint32_t ee_address);
int nvdata_unsigned16_set(uint32_t ee_address, uint16_t short_value);
uint8_t nvdata_unsigned8(uint32_t ee_address);
int nvdata_unsigned8_set(uint32_t ee_address, uint8_t byte_value);

bool nvdata_name_isvalid(uint8_t encoding, uint8_t length, const char *str);
bool nvdata_name_set(
    uint16_t offset, uint8_t encoding, const char *str, uint8_t length);
uint8_t nvdata_name(
    uint16_t offset, uint8_t *encoding, char *value, uint8_t value_size);

uint32_t nvdata_get(uint32_t ee_address, uint8_t *buffer, uint32_t nbytes);
uint32_t nvdata_set(uint32_t ee_address, uint8_t *buffer, uint32_t nbytes);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
