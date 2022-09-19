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
#ifndef __BSC__RETCODES__INCLUDED__
#define __BSC__RETCODES__INCLUDED__

typedef enum {
    BSC_SC_SUCCESS = 0,
    BSC_SC_NO_RESOURCES = 1,
    BSC_SC_BAD_PARAM = 2,
    BSC_SC_INVALID_OPERATION = 3,
    BSC_SC_DUPLICATED_VMAC = 4,
    BSC_SC_PEER_DISCONNECTED = 5,   // For some reason remote peer has closed connection
    BSC_SC_TIMEDOUT = 6
} BSC_SC_RET;

#endif