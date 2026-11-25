/**
 * @file
 * @brief Port-specific system headers for QNX Neutrino (QNX 8.0).
 * @details QNX 8.0 networking (io-sock) is derived from FreeBSD, so the
 *  network headers match the BSD port, without libdispatch (not available
 *  on QNX) and without the kernel-only net/if_var.h. The BSD-style network
 *  headers are only included on QNX, so the MS/TP part of this port can
 *  also be compiled on a POSIX host for testing.
 * @author Vihar N
 * @date 2026
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#ifndef BACNET_PORT_QNX_BACPORT_H
#define BACNET_PORT_QNX_BACPORT_H

#include <sys/types.h> /* basic system data types */
#include <sys/time.h> /* timeval{} for select() */
#include <time.h> /* timespec{} for pselect() */
#include <netinet/in.h> /* sockaddr_in{} and other Internet defns */
#include <arpa/inet.h> /* inet(3) functions */
#include <fcntl.h> /* for nonblocking */
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h> /* for S_xxx file mode constants */
#include <sys/uio.h> /* for iovec{} and readv/writev */
#include <unistd.h>
#include <sys/wait.h>
#include <sys/un.h> /* for Unix domain sockets */
#include <sys/select.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <sched.h>

#define ENUMS
#include <sys/socket.h>
#include <net/if.h>
#ifdef __QNX__
#include <net/route.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <netinet/in_var.h>
#endif
#include <ifaddrs.h>
#include "bacnet/basic/sys/bacnet_stack_exports.h"

#define BACNET_OBJECT_TABLE(                                               \
    table_name, _type, _init, _count, _index_to_instance, _valid_instance, \
    _object_name, _read_property, _write_property, _RPM_list, _RR_info,    \
    _iterator, _value_list, _COV, _COV_clear, _intrinsic_reporting)        \
    static_assert(false, "Unsupported BACNET_OBJECT_TABLE for this platform")

/* Local helper functions for this port (BACnet/IP, from ports/bsd) */
BACNET_STACK_EXPORT
extern int bip_get_local_netmask(struct in_addr *netmask);

BACNET_STACK_EXPORT
extern int bip_get_local_address_ioctl(
    const char *ifname, struct in_addr *addr, uint32_t request);

#endif
