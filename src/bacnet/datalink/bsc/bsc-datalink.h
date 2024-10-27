/**
 * @file
 * @brief BACNet/SC datalink public interface API.
 * @author Kirill Neznamov
 * @date October 2022
 * @section LICENSE
 *
 * Copyright (C) 2022 Legrand North America, LLC
 * as an unpublished work.
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#ifndef BACNET_DATALINK_BSC_DATALINK_H
#define BACNET_DATALINK_BSC_DATALINK_H
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/npdu.h"
#include "bacnet/datalink/bsc/bvlc-sc.h"
#include "bacnet/datalink/bsc/bsc-retcodes.h"

/**
 * @brief Blocking thread-safe bsc_init() function
 *        initializes BACNet/SC datalink in accordence with value of properties
 *        from BACNet/SC Network Port Object (sc_netport.h). That means that
 *        user must initialize and set corresponded properties required for
 *        configuration of BACNet/SC datalink before calling of that function.
 *        According to "Addendum cc" to ANSI/ASHRAE Standard 135-2020
 *        https://bacnet.org/wp-content/uploads/sites/4/2022/08/Add-135-2020cc.pdf
 *        most important properties are:
 *          SC_Primary_Hub_URI
 *          SC_Failover_Hub_URI
 *          SC_Maximum_Reconnect_Time
 *          SC_Connect_Wait_Timeout
 *          SC_Disconnect_Wait_Timeout
 *          SC_Heartbeat_Timeout
 *          Operational_Certificate_File
 *          Issuer_Certificate_Files
 *          SC_Hub_Function_Enable
 *          SC_Hub_Function_Binding
 *          SC_Direct_Connect_Initiate_Enable
 *          SC_Direct_Connect_Binding
 *          SC_Direct_Connect_Accept_Enable
 * @param ifname - ignored and unused it was added for backward compatibility.
 * @return true if datalink was initiated and started, otherwise returns false.
 */

BACNET_STACK_EXPORT
bool bsc_init(char *ifname);

/**
 * @brief Blocking thread-safe bsc_cleanup() function
 *        de-initializes BACNet/SC datalink.
 */
BACNET_STACK_EXPORT
void bsc_cleanup(void);
/**
 * @brief Function checks if all needed certificate file are present.
 * @return true if all needed certificate file are present otherwise returns
 *         false.
 */

BACNET_STACK_EXPORT
bool bsc_cert_files_check(void);

/**
 * @brief Blocking thread-safe bsc_send_pdu() function
 *        sends pdu over BACNet/SC to node specified by
 *        destination address param.
 *
 * @param dest [in] BACNet/SC node's virtual MAC address as
 *                  defined in Clause AB.1.5.2.
 *                  Can be broadcast.
 * @param npdu_data [in] BACNet/SC datalink does not use that
 *                       parameter. Added for backward
 *                       compatibility.
 * @param pdu [in] protocol data unit to be sent.
 * @param pdu_len [in] - number of bytes to send.
 * @return Number of bytes sent on success, negative number on failure.
 */

BACNET_STACK_EXPORT
int bsc_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len);

/**
 * @brief Blocking thread-safe bsc_receive() function
 *        receives NPDUs transferred over BACNet/SC
 *        from a node specified by it's virtual MAC address as
 *        defined in Clause AB.1.5.2.
 *
 * @param src [out] Source VMAC address
 * @param pdu [out] A buffer to hold the PDU portion of the received packet,
 *                  after the BVLC portion has been stripped off.
 * @param max_pdu [in] Size of the pdu[] buffer.
 * @param timeout [in] The number of milliseconds to wait for a packet.
 * @return The number of octets (remaining) in the PDU, or zero on failure.
 */

BACNET_STACK_EXPORT
uint16_t bsc_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout_ms);

/**
 * @brief Function can be used to retrieve broadcast
 *        VMAC address for BACNet/SC node.
 *
 * @param addr [out] Value of broadcast VMAC address.
 */

BACNET_STACK_EXPORT
void bsc_get_broadcast_address(BACNET_ADDRESS *addr);

/**
 * @brief Function can be used to retrieve local
 *        VMAC address of initialized BACNet/SC datalink.
 *        If function called when datalink is not started,
 *        my_address filled by empty vmac address
 *        X'000000000000' as it defined in clause AB.1.5.2
 *
 * @param my_address [out] Value of local VMAC address.
 */

BACNET_STACK_EXPORT
void bsc_get_my_address(BACNET_ADDRESS *my_address);

/**
 * @brief Function checks if BACNet/SC direct connection is
 *        established with remote BACNet/SC node.
 *        User can check the status of connection using either
 *        destination vmac or list of destination urls.
 * @param dest BACNet/SC vmac of remote node to check direct
 *        connection status.
 * @param urls this array represents the possible URIs of a
 *        remote node for acceptance of direct connections.
 *        Can contain 1 elem.
 * @param urls_cnt - size of urls array.
 * @return true if connection is established otherwise returns
 *         false.
 */

BACNET_STACK_EXPORT
bool bsc_direct_connection_established(
    BACNET_SC_VMAC_ADDRESS *dest, char **urls, size_t urls_cnt);

/**
 * @brief Function starts process of establishing of a
 *        direct BACNet/SC connection to node identified by
 *        either urls or dest parameter. User should note that
 *        if dest parameter is used, local node tries to resolve
 *        it (e.g.to get URIs related to dest vmac from all existent
 *        BACNet/SC nodes in network). As a result the process of
 *        establishing of a BACNet/SC connection by dest may
 *        take unpredictable amount of time depending on a current
 *        network configuration.
 * @param dest BACNet/SC vmac of remote node to check direct
 *        connection status.
 * @param urls this array represents the possible URIs of a
 *        remote node for acceptance of direct connections.
 *        Can contain 1 elem.
 * @param urls_cnt - size of urls array.
 *
 * @return BSC_SC_SUCCESS if process of a establishing of a BACNet/SC
 *         connection was started successfully, otherwise returns
 *         any retcode from BSC_SC_RET enum.
 */

BACNET_STACK_EXPORT
BSC_SC_RET
bsc_connect_direct(BACNET_SC_VMAC_ADDRESS *dest, char **urls, size_t urls_cnt);

BACNET_STACK_EXPORT
void bsc_disconnect_direct(BACNET_SC_VMAC_ADDRESS *dest);

BACNET_STACK_EXPORT
void bsc_maintenance_timer(uint16_t seconds);

#endif
