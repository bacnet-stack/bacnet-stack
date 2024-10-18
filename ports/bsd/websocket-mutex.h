/**
 * @file
 * @brief API for encoding/decoding of BACnet/SC BVLC messages
 * @author Kirill Neznamov
 * @date May 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef __BSC_WEBSOCKET_INCLUDED__
#define __BSC_WEBSOCKET_INCLUDED__

void bsc_websocket_global_lock(void);
void bsc_websocket_global_unlock(void);

#endif
