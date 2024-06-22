/**
 * @file
 * @brief This file has the macros that encode and decode the unique 32 bit
 * keys for the keylist when used with BACnet object types and instances
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
*/
#ifndef BACNET_SYS_KEY_H
#define BACNET_SYS_KEY_H
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

/* key used for BACnet object type and instance */
typedef uint32_t KEY;

/* assuming a 32 bit KEY */
#define KEY_TYPE_OFFSET  22     /* bits - for BACnet */
#define KEY_TYPE_MASK    0x000003FFL
#define KEY_ID_MASK  0x003FFFFFL
#define KEY_ID_MAX  (KEY_ID_MASK + 1L)
#define KEY_TYPE_MAX  (KEY_TYPE_MASK + 1L)
#define KEY_LAST(key) ((key & KEY_ID_MASK) == KEY_ID_MAX)

#define KEY_ENCODE(type,id) ( ((unsigned int)\
((unsigned int)(type) & KEY_TYPE_MASK) << KEY_TYPE_OFFSET) |\
 ((unsigned int)(id) & KEY_ID_MASK) )

#define KEY_DECODE_TYPE(key) ((int)(((unsigned int)(key) >> KEY_TYPE_OFFSET)\
 & KEY_TYPE_MASK))

#define KEY_DECODE_ID(key) ((int)((unsigned int)(key) & KEY_ID_MASK))

#endif
