/**
 * @file 
 * @brief module for common function for BACnet/SC implementation
 * @author Kirill Neznamov
 * @date Jule 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC 
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __BACNET__SC__UTIL__INCLUDED__
#define __BACNET__SC__UTIL__INCLUDED__

#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/datalink/bsc/websocket.h"
#include "bacnet/datalink/bsc/bsc-retcodes.h"
#include "bacnet/datalink/bsc/bvlc-sc.h"

BSC_SC_RET bsc_map_websocket_retcode(BSC_WEBSOCKET_RET ret);

void bsc_copy_vmac(BACNET_SC_VMAC_ADDRESS *dst, BACNET_SC_VMAC_ADDRESS *src);
void bsc_copy_uuid(BACNET_SC_UUID *dst, BACNET_SC_UUID *src);
char *bsc_vmac_to_string(BACNET_SC_VMAC_ADDRESS *vmac);
char *bsc_uuid_to_string(BACNET_SC_UUID *uuid);
void bsc_generate_random_vmac(BACNET_SC_VMAC_ADDRESS *p);

#endif