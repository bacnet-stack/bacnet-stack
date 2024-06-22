/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @brief Defines the bit/byte/word/long conversions that are used in code
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_SYS_BYTES_H
#define BACNET_SYS_BYTES_H
#include <stdint.h>

#ifndef LO_NIB
#define LO_NIB(b) ((b)&0xF)
#endif

#ifndef HI_NIB
#define HI_NIB(b) ((b) >> 4)
#endif

#ifndef LO_BYTE
#define LO_BYTE(w) ((uint8_t)(w))
#endif

#ifndef HI_BYTE
#define HI_BYTE(w) ((uint8_t)((uint16_t)(w) >> 8))
#endif

#ifndef LO_WORD
#define LO_WORD(x) ((uint16_t)(x))
#endif

#ifndef HI_WORD
#define HI_WORD(x) ((uint16_t)((uint32_t)(x) >> 16))
#endif

#ifndef MAKE_WORD
#define MAKE_WORD(lo, hi) \
    ((uint16_t)(((uint8_t)(lo)) | (((uint16_t)((uint8_t)(hi))) << 8)))
#endif

#ifndef MAKE_LONG
#define MAKE_LONG(lo, hi) \
    ((uint32_t)(((uint16_t)(lo)) | (((uint32_t)((uint16_t)(hi))) << 16)))
#endif

#endif /* end of header file */
