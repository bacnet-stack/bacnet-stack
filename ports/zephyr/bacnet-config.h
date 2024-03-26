/**************************************************************************
*
* Copyright (c) 2024 Legrand North America, LLC.
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/

#ifndef BACNET_PORTS_ZEPHYR_BACNET_CONFIG_H
#define BACNET_PORTS_ZEPHYR_BACNET_CONFIG_H

#if ! defined BACNET_CONFIG_H || ! BACNET_CONFIG_H
#error bacnet-config.h included outside of BACNET_CONFIG_H control
#endif

#include <zephyr/sys/util.h>  // Provides platform-specific defn of ARRAY_SIZE()

/* some smaller defaults for microcontrollers */
#if !defined(MAX_TSM_TRANSACTIONS)
#define MAX_TSM_TRANSACTIONS 1
#endif
#if !defined(MAX_ADDRESS_CACHE)
#define MAX_ADDRESS_CACHE 1
#endif
#if !defined(MAX_CHARACTER_STRING_BYTES)
#define MAX_CHARACTER_STRING_BYTES 64
#endif
#if !defined(MAX_OCTET_STRING_BYTES)
#define MAX_OCTET_STRING_BYTES 64
#endif

/* K.6.6 BIBB - Network Management-BBMD Configuration-B (NM-BBMDC-B)*/
#if !defined(MAX_BBMD_ENTRIES)
#define MAX_BBMD_ENTRIES 5
#endif
#if !defined(MAX_FD_ENTRIES)
#define MAX_FD_ENTRIES 5
#endif

#endif // BACNET_PORTS_ZEPHYR_BACNET_CONFIG_H
