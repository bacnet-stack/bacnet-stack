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
#include    <sys/types.h>   /* basic system data types */
#include    <sys/time.h>    /* timeval{} for select() */
#include    <time.h>        /* timespec{} for pselect() */
#include    <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include    <arpa/inet.h>   /* inet(3) functions */
#include    <fcntl.h>       /* for nonblocking */
#include    <netdb.h>
#include    <errno.h>
#include    <signal.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <sys/stat.h>    /* for S_xxx file mode constants */
#include    <sys/uio.h>     /* for iovec{} and readv/writev */
#include    <unistd.h>
#include    <sys/wait.h>
#include    <sys/un.h>      /* for Unix domain sockets */

#ifdef  HAVE_SYS_SELECT_H
#include    <sys/select.h>  /* for convenience */
#endif

#ifdef  HAVE_POLL_H
#include    <poll.h>        /* for convenience */
#endif

#ifdef  HAVE_STRINGS_H
#include    <strings.h>     /* for convenience */
#endif

/* Three headers are normally needed for socket/file ioctl's:
 * <sys/ioctl.h>, <sys/filio.h>, and <sys/sockio.h>.
 */
#ifdef  HAVE_SYS_IOCTL_H
#include    <sys/ioctl.h>
#endif
#ifdef  HAVE_SYS_FILIO_H
#include    <sys/filio.h>
#endif
#ifdef  HAVE_SYS_SOCKIO_H
#include    <sys/sockio.h>
#endif

#include <pthread.h>
#include <semaphore.h>

#define ENUMS
#include <sys/socket.h>
#ifndef __CYGWIN__
#include <net/route.h>
#endif
#include <net/if.h>
#ifndef __CYGWIN__
#include <net/if_arp.h>
#endif
#include <features.h>   /* for the glibc version number */
#if __GLIBC__ >= 2 && __GLIBC_MINOR >= 1
#include <netpacket/packet.h>
#include <net/ethernet.h>       /* the L2 protocols */
#else
#include <asm/types.h>
#ifndef __CYGWIN__
#include <linux/if_packet.h>
#include <linux/if_arcnet.h>
#include <linux/if_ether.h>
#endif
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include "bacnet/basic/sys/bacnet_stack_exports.h"

#define BACNET_OBJECT_TABLE(table_name, _type, _init, _count,               \
                            _index_to_instance, _valid_instance, _object_name, \
                            _read_property, _write_property, _RPM_list,     \
                            _RR_info, _iterator, _value_list, _COV,         \
                            _COV_clear, _intrinsic_reporting)               \
    static_assert(false, "Unsupported BACNET_OBJECT_TABLE for this platform")

/** @file linux/bacport.h  Includes Linux network headers. */

/* Local helper functions for this port */
BACNET_STACK_EXPORT
extern int bip_get_local_netmask(
    struct in_addr *netmask);

BACNET_STACK_EXPORT
extern int bip_get_local_address_ioctl(
    char *ifname,
    struct in_addr *addr,
    int request);

#endif
