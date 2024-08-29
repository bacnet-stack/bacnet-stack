/**
 * @file
 * @brief API for memory copy function
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_MEMCOPY_H
#define BACNET_MEMCOPY_H
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    bool memcopylen(
        size_t offset,
        size_t max,
        size_t len);
    size_t memcopy(
        void *dest,
        const void *src,
        size_t offset,
        size_t len,
        size_t max);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
