/**************************************************************************
 *
 * Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/

#ifndef BACPORT_H
#define BACPORT_H

/* common unix sockets headers needed */
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

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h> /* for convenience */
#endif

#ifdef HAVE_POLL_H
#include <poll.h> /* for convenience */
#endif

/* Three headers are normally needed for socket/file ioctl's:
 * <sys/ioctl.h>, <sys/filio.h>, and <sys/sockio.h>.
 */
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif

#include <pthread.h>
#include <dispatch/dispatch.h>

#define ENUMS
#include <sys/socket.h>
#include <net/route.h>
#include <net/if.h>
#include <net/if_var.h>
#include <net/if_arp.h>
#include <net/if_dl.h>
#include <ifaddrs.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include "bacnet/basic/sys/bacnet_stack_exports.h"

#define BACNET_OBJECT_TABLE(                                               \
    table_name, _type, _init, _count, _index_to_instance, _valid_instance, \
    _object_name, _read_property, _write_property, _RPM_list, _RR_info,    \
    _iterator, _value_list, _COV, _COV_clear, _intrinsic_reporting)        \
    static_assert(false, "Unsupported BACNET_OBJECT_TABLE for this platform")

/** @file bsd/bacport.h  Includes BSD network headers. */

/* Local helper functions for this port */
BACNET_STACK_EXPORT
extern int bip_get_local_netmask(struct in_addr *netmask);

BACNET_STACK_EXPORT
extern int bip_get_local_address_ioctl(
    const char *ifname, struct in_addr *addr, uint32_t request);

#endif
