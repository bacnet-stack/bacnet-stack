/**
 * @file
 * @brief API for the BACnet MS/TP defines to text functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 * @defgroup DLMSTP BACnet MS/TP DataLink
 * @ingroup DataLink
 */
#ifndef BACNET_MSTPTEXT_H
#define BACNET_MSTPTEXT_H
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
const char *mstptext_receive_state(unsigned index);
BACNET_STACK_EXPORT
const char *mstptext_master_state(unsigned index);
BACNET_STACK_EXPORT
const char *mstptext_frame_type(unsigned index);
BACNET_STACK_EXPORT
const char *mstptext_zero_config_state(unsigned index);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
