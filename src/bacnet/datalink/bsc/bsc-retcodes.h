/**
 * @file
 * @brief BACNet secure connect main include header.
 * @author Kirill Neznamov
 * @date August 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef BACNET_DATALINK_BSC_RETCODES_H
#define BACNET_DATALINK_BSC_RETCODES_H
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

typedef enum {
    BSC_SC_SUCCESS = 0,
    BSC_SC_NO_RESOURCES = 1,
    BSC_SC_BAD_PARAM = 2,
    BSC_SC_INVALID_OPERATION = 3
} BSC_SC_RET;

#endif
