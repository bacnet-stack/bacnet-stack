/**
 * @file
 * @brief Store and retrieve non-volatile data
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2013
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef NVM_DATA_H
#define NVM_DATA_H

#include <avr/eeprom.h>

/* compatible functions could put in header to abstract more */
#define nvm_write(dst, src, len) \
    eeprom_write_block((uint8_t *)(src),(uint8_t *)(dst), (size_t)(len))

#define nvm_read(src, dst, len) \
    eeprom_read_block((uint8_t *)dst, (const uint8_t *)(src),(size_t)(len))

/*=============== EEPROM ================*/
/* define EEPROM signature version */
#define NVM_SIGNATURE 0
#define NVM_VERSION 1

/* define the MAC, BAUD, MAX Master, Device Instance internal
   so that bootloader *could* use them. */
/* note: MAC could come from DIP switch, or be in non-volatile memory */
#define NVM_MAC_ADDRESS 2
/* 9=9.6k, 19=19.2k, 38=38.4k, 57=57.6k, 76=76.8k, 115=115.2k */
#define NVM_BAUD_K 3
#define NVM_MAX_MASTER 4
/* device instance is only 22 bits - easier if we use 32 bits */
#define NVM_DEVICE_0 5
#define NVM_DEVICE_1 6
#define NVM_DEVICE_2 7
#define NVM_DEVICE_3 8

/* free space - 9..31 */

/* BACnet Names - 32 bytes of data each */
#define NVM_NAME_LENGTH(n) ((n)+0)
#define NVM_NAME_ENCODING(n) ((n)+1)
#define NVM_NAME_STRING(n) ((n)+2)
#define NVM_NAME_SIZE 30
#define NVM_NAME_OFFSET (1+1+NVM_NAME_SIZE)
/* Device Name - starting offset */
#define NVM_DEVICE_NAME 32
/* Device Description - starting offset  */
#define NVM_DEVICE_DESCRIPTION \
    (NVM_DEVICE_NAME+NVM_NAME_OFFSET)
/* Device Location - starting offset  */
#define NVM_DEVICE_LOCATION \
    (NVM_DEVICE_DESCRIPTION+NVM_NAME_OFFSET)

/* free space 128..4096 */

#endif
