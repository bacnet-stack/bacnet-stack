/**************************************************************************
 *
 * Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/

#ifndef BACPORT_H
#define BACPORT_H

#include "bacnet/basic/sys/bacnet_stack_exports.h"

#define WIN32_LEAN_AND_MEAN
#define STRICT 1
/* Windows XP minimum */
#if (_WIN32_WINNT < _WIN32_WINNT_WINXP)
#undef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WINXP
#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WINXP
#endif

#include <windows.h>
#ifndef BACNET_IP_BROADCAST_USE_CLASSADDR
#include <iphlpapi.h>
#if defined(_MSC_VER)
#pragma comment(lib, "IPHLPAPI.lib")
#endif
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#if defined(_MSC_VER)
#pragma comment(lib, "Ws2_32.lib")
#endif
#include <stdlib.h>
#include <stdio.h>
#ifdef __MINGW32__
#include <ws2spi.h>
#else
#pragma warning(push)
#pragma warning(disable : 4101 4191)
#include <wspiapi.h>
#pragma warning(pop)
/* add winmm.lib to our build */
#pragma comment(lib, "winmm.lib")
#endif
#include <mmsystem.h>

#if !defined(_MSC_VER)
#include <sys/time.h>
#endif
#include <sys/timeb.h>

#ifdef _MSC_VER
#define inline __inline
#endif
#ifdef __BORLANDC__
#define inline __inline
#endif

BACNET_STACK_EXPORT
extern int bip_get_local_netmask(struct in_addr *netmask);

#define BACNET_OBJECT_TABLE(                                               \
    table_name, _type, _init, _count, _index_to_instance, _valid_instance, \
    _object_name, _read_property, _write_property, _RPM_list, _RR_info,    \
    _iterator, _value_list, _COV, _COV_clear, _intrinsic_reporting)        \
    static_assert(false, "Unsupported BACNET_OBJECT_TABLE for this platform")

#endif
