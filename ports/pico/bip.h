/**************************************************************************
 *
 * Copyright (C) 2012 Steve Karg <skarg@users.sourceforge.net>
 * Copyright (C) 2025 Testimony Adams <adamstestimony@gmail.com>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#ifndef BIP_H
#define BIP_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/datalink/bvlc.h"

/* specific defines for BACnet/IP over Ethernet */
#define BIP_HEADER_MAX (1 + 1 + 2)
#define BIP_MPDU_MAX (BIP_HEADER_MAX + MAX_PDU) /* 1506 */

#define BVLL_TYPE_BACNET_IP (0x81)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* User must implement these platform-specific network functions */

/**
 * @brief Initialize the UDP socket for BACnet/IP
 * @param port - UDP port to bind to
 * @return true on success, false on failure
 */
bool bip_socket_init(uint16_t port);

/**
 * @brief Send UDP packet
 * @param dest_addr - destination IP address (4 bytes, network byte order)
 * @param dest_port - destination port (host byte order)
 * @param mtu - data buffer to send
 * @param mtu_len - length of data
 * @return number of bytes sent, or negative on error
 */
int bip_socket_send(
    const uint8_t *dest_addr,
    uint16_t dest_port,
    const uint8_t *mtu,
    uint16_t mtu_len);

/**
 * @brief Receive UDP packet (non-blocking)
 * @param buf - buffer to store received data
 * @param buf_len - maximum buffer size
 * @param src_addr - pointer to store source IP (4 bytes, network byte order)
 * @param src_port - pointer to store source port (host byte order)
 * @return number of bytes received, 0 if no data, negative on error
 */
int bip_socket_receive(
    uint8_t *buf,
    uint16_t buf_len,
    uint8_t *src_addr,
    uint16_t *src_port);

/**
 * @brief Close/cleanup the UDP socket
 */
void bip_socket_cleanup(void);

/**
 * @brief Get local network information
 * @param local_addr - buffer for local IP (4 bytes)
 * @param netmask - buffer for netmask (4 bytes)
 * @return true on success
 */
bool bip_get_local_network_info(uint8_t *local_addr, uint8_t *netmask);



/* BACnet/IP port functions */
bool bip_init(uint16_t port);
void bip_set_interface(void);
void bip_cleanup(void);

/* Convert uint8_t IPv4 to uint32 */
uint32_t convertBIP_Address2uint32(const uint8_t *bip_address);
void convertUint32Address_2_uint8Address(uint32_t ip, uint8_t *address);

/* common BACnet/IP functions */
void bip_set_socket(uint8_t sock_fd);
uint8_t bip_socket(void);
bool bip_valid(void);
void bip_get_broadcast_address(BACNET_ADDRESS *dest);
void bip_get_my_address(BACNET_ADDRESS *my_address);

/* function to send a packet out the BACnet/IP socket */
int bip_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len);

/* Function to send a packet out the BACnet/IP socket (Annex J) */
int bip_send_mpdu(
    const BACNET_IP_ADDRESS *dest,
    const uint8_t *mtu,
    uint16_t mtu_len);

/* receives a BACnet/IP packet */
uint16_t bip_receive(
    BACNET_ADDRESS *src,
    uint8_t *pdu,
    uint16_t max_pdu,
    unsigned timeout);

/* use host byte order for setting */
void bip_set_port(uint16_t port);
uint16_t bip_get_port(void);

/* use network byte order for setting */
void bip_set_addr(const uint8_t *net_address);
uint8_t *bip_get_addr(void);

/* use network byte order for setting */
void bip_set_broadcast_addr(const uint8_t *net_address);
uint8_t *bip_get_broadcast_addr(void);

/* gets an IP address by name */
long bip_getaddrbyname(const char *host_name);

void bip_debug_enable(void);
void bip_debug_disable(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* BIP_H */
