/**
 * @file
 * @brief API for determination of host Endianness
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_SYS_BIGEND_H
#define BACNET_SYS_BIGEND_H
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#ifndef BACNET_BIG_ENDIAN
/* GCC */
#ifdef __BYTE_ORDER__
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define BACNET_BIG_ENDIAN 1
#else
#define BACNET_BIG_ENDIAN 0
#endif
#endif
#endif

#ifdef BACNET_BIG_ENDIAN
#define big_endian() BACNET_BIG_ENDIAN
#else
BACNET_STACK_EXPORT
int big_endian(void);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
