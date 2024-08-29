/**
 * @file
 * @brief API for functions used to manipulate filenames
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
*/
#ifndef BACNET_SYS_FILENAME_H
#define BACNET_SYS_FILENAME_H
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    const char *filename_remove_path(
        const char *filename_in);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
