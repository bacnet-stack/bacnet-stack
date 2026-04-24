/**
 * @file
 * @brief BACnet/IP datalink implementation for the PlatformIO ESP32 port
 * @author Kato Gangstad
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "bacnet/bacdcode.h"
#include "bacnet/bacint.h"
#include "bacnet/datalink/bvlc.h"

#include "bip.h"
#include "bvlc.h"

#if PRINT_ENABLED || DEBUG
#include <stdio.h>
#endif

#define MAX_SOCK_NUM 8

static uint8_t BIP_Socket = MAX_SOCK_NUM;
static uint16_t BIP_Port = 0;
/* broadcast destination port to use */
static uint16_t BIP_Broadcast_Port = 0;
static uint8_t BIP_Address[4] = { 0, 0, 0, 0 };
static uint8_t BIP_Broadcast_Address[4] = { 0, 0, 0, 0 };

/**
 * @brief Convert a 4-byte IP address into a uint32_t value
 * @param bip_address pointer to the 4-byte address
 * @return 32-bit packed address
 */
uint32_t convertBIP_Address2uint32(const uint8_t *bip_address)
{
    return ((uint32_t)bip_address[0] << 24) | ((uint32_t)bip_address[1] << 16) |
        ((uint32_t)bip_address[2] << 8) | ((uint32_t)bip_address[3]);
}

/**
 * @brief Convert a uint32_t IP address into a 4-byte array
 * @param ip packed IP address
 * @param address destination buffer for the 4-byte address
 */
void convertUint32Address_2_uint8Address(uint32_t ip, uint8_t *address)
{
    address[0] = (uint8_t)(ip >> 24);
    address[1] = (uint8_t)(ip >> 16);
    address[2] = (uint8_t)(ip >> 8);
    address[3] = (uint8_t)(ip >> 0);
}

/**
 * @brief Store the active socket identifier used by the BACnet/IP layer
 * @param sock_fd socket identifier
 */
void bip_set_socket(uint8_t sock_fd)
{
    BIP_Socket = sock_fd;
}

/**
 * @brief Get the stored BACnet/IP socket identifier
 * @return socket identifier
 */
uint8_t bip_socket(void)
{
    return BIP_Socket;
}

/**
 * @brief Check whether the BACnet/IP socket state is valid
 * @return true if the stored socket identifier is valid
 */
bool bip_valid(void)
{
    return (BIP_Socket < MAX_SOCK_NUM);
}

/**
 * @brief Store the local IPv4 address used by BACnet/IP
 * @param net_address pointer to the 4-byte local address
 */
void bip_set_addr(const uint8_t *net_address)
{
    uint8_t i;

    for (i = 0; i < 4; i++) {
        BIP_Address[i] = net_address[i];
    }
}

/**
 * @brief Get the stored local IPv4 address
 * @return pointer to the 4-byte local address
 */
uint8_t *bip_get_addr(void)
{
    return BIP_Address;
}

/**
 * @brief Store the broadcast IPv4 address used by BACnet/IP
 * @param net_address pointer to the 4-byte broadcast address
 */
void bip_set_broadcast_addr(const uint8_t *net_address)
{
    uint8_t i;

    for (i = 0; i < 4; i++) {
        BIP_Broadcast_Address[i] = net_address[i];
    }
}

/**
 * @brief Get the stored broadcast IPv4 address
 * @return pointer to the 4-byte broadcast address
 */
uint8_t *bip_get_broadcast_addr(void)
{
    return BIP_Broadcast_Address;
}

/**
 * @brief Store the UDP port used by BACnet/IP
 * @param port UDP port number
 */
void bip_set_port(uint16_t port)
{
    BIP_Port = port;
}

/**
 * @brief Store the UDP broadcast destination port used by BACnet/IP
 * @param port UDP port number
 */
void bip_set_broadcast_port(uint16_t port)
{
    BIP_Broadcast_Port = port;
}

/**
 * @brief Get the UDP port used by BACnet/IP
 * @return UDP port number
 */
uint16_t bip_get_port(void)
{
    return BIP_Port;
}

/**
 * @brief Get the UDP broadcast destination port used by BACnet/IP
 * @return UDP port number
 */
uint16_t bip_get_broadcast_port(void)
{
    if (BIP_Broadcast_Port) {
        return BIP_Broadcast_Port;
    }

    return BIP_Port;
}

/**
 * @brief Decode a BACnet/IP MAC address into IPv4 address and port parts
 * @param bac_addr source BACnet address
 * @param address destination IPv4 address buffer
 * @param port destination UDP port pointer
 * @return number of bytes decoded from the MAC address
 */
static int bip_decode_bip_address(
    const BACNET_ADDRESS *bac_addr, uint8_t *address, uint16_t *port)
{
    int len = 0;

    if (bac_addr) {
        memcpy(address, &bac_addr->mac[0], 4);
        memcpy(port, &bac_addr->mac[4], 2);
        len = 6;
    }

    return len;
}

/**
 * @brief Send a BACnet NPDU over BACnet/IP
 * @param dest destination BACnet address
 * @param npdu_data NPDU metadata
 * @param pdu payload buffer
 * @param pdu_len payload length in bytes
 * @return number of bytes sent, or -1 on error
 */
int bip_send_pdu(
    BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len)
{
    uint8_t mtu[BIP_MPDU_MAX] = { 0 };
    int mtu_len = 0;
    int bytes_sent = 0;
    uint8_t address[] = { 0, 0, 0, 0 };
    uint16_t port = 0;
    uint8_t i;

    (void)npdu_data;

    if (BIP_Socket >= MAX_SOCK_NUM) {
        return -1;
    }

    mtu[0] = BVLL_TYPE_BACNET_IP;
    if ((dest->net == BACNET_BROADCAST_NETWORK) ||
        ((dest->net > 0) && (dest->len == 0)) || (dest->mac_len == 0)) {
        for (i = 0; i < 4; i++) {
            address[i] = BIP_Broadcast_Address[i];
        }
        port = bip_get_broadcast_port();
        mtu[1] = BVLC_ORIGINAL_BROADCAST_NPDU;
    } else if (dest->mac_len == 6) {
        bip_decode_bip_address(dest, address, &port);
        mtu[1] = BVLC_ORIGINAL_UNICAST_NPDU;
    } else {
        return -1;
    }

    mtu_len = 2;
    mtu_len += encode_unsigned16(&mtu[mtu_len], (uint16_t)(pdu_len + 4));
    memcpy(&mtu[mtu_len], pdu, pdu_len);
    mtu_len += (int)pdu_len;

    bytes_sent = bip_socket_send(address, port, mtu, (uint16_t)mtu_len);

    return bytes_sent;
}

/**
 * @brief Receive a BACnet/IP NPDU and strip BVLC framing
 * @param src source BACnet address
 * @param pdu receive buffer for NPDU data
 * @param max_pdu receive buffer size
 * @param timeout receive timeout in milliseconds
 * @return NPDU length in bytes, or 0 if no valid frame was received
 */
uint16_t bip_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout)
{
    int received_bytes = 0;
    int max = 0;
    uint16_t pdu_len = 0;
    uint8_t src_addr[] = { 0, 0, 0, 0 };
    uint16_t src_port = 0;
    uint16_t i = 0;
    int function = 0;

    (void)timeout;

    if (BIP_Socket >= MAX_SOCK_NUM) {
        return 0;
    }

    received_bytes = bip_socket_receive(pdu, max_pdu, src_addr, &src_port);
    if (received_bytes <= 0) {
        return 0;
    }

    if (pdu[0] != BVLL_TYPE_BACNET_IP) {
        return 0;
    }

    max = (int)max_pdu - received_bytes;
    if (max > 0) {
        if (max > 16) {
            max = 16;
        }
        memset(&pdu[received_bytes], 0, (size_t)max);
    }

    if (bvlc_for_non_bbmd(src_addr, &src_port, pdu, (uint16_t)received_bytes) >
        0) {
        return 0;
    }

    function = pico_bvlc_get_function_code();
    if ((function == BVLC_ORIGINAL_UNICAST_NPDU) ||
        (function == BVLC_ORIGINAL_BROADCAST_NPDU)) {
        if ((convertBIP_Address2uint32(src_addr) ==
             convertBIP_Address2uint32(BIP_Address)) &&
            (src_port == BIP_Port)) {
            pdu_len = 0;
        } else {
            src->mac_len = 6;
            memcpy(&src->mac[0], &src_addr, 4);
            memcpy(&src->mac[4], &src_port, 2);
            (void)decode_unsigned16(&pdu[2], &pdu_len);
            pdu_len -= 4;
            if (pdu_len < max_pdu) {
                for (i = 0; i < pdu_len; i++) {
                    pdu[i] = pdu[4 + i];
                }
            } else {
                pdu_len = 0;
            }
        }
    } else if (function == BVLC_FORWARDED_NPDU) {
        memcpy(&src_addr, &pdu[4], 4);
        memcpy(&src_port, &pdu[8], 2);
        if ((convertBIP_Address2uint32(src_addr) ==
             convertBIP_Address2uint32(BIP_Address)) &&
            (src_port == BIP_Port)) {
            pdu_len = 0;
        } else {
            src->mac_len = 6;
            memcpy(&src->mac[0], &src_addr, 4);
            memcpy(&src->mac[4], &src_port, 2);
            (void)decode_unsigned16(&pdu[2], &pdu_len);
            pdu_len -= 10;
            if (pdu_len < max_pdu) {
                for (i = 0; i < pdu_len; i++) {
                    pdu[i] = pdu[10 + i];
                }
            } else {
                pdu_len = 0;
            }
        }
    }

    return pdu_len;
}

/**
 * @brief Build the local BACnet/IP address structure
 * @param my_address destination BACnet address
 */
void bip_get_my_address(BACNET_ADDRESS *my_address)
{
    if (my_address) {
        my_address->mac_len = 6;
        memcpy(&my_address->mac[0], &BIP_Address[0], 4);
        memcpy(&my_address->mac[4], &BIP_Port, 2);
        my_address->net = 0;
        my_address->len = 0;
    }
}

/**
 * @brief Build the BACnet/IP broadcast address structure
 * @param dest destination BACnet address
 */
void bip_get_broadcast_address(BACNET_ADDRESS *dest)
{
    if (dest) {
        dest->mac_len = 6;
        memcpy(&dest->mac[0], &BIP_Broadcast_Address[0], 4);
        {
            uint16_t port = bip_get_broadcast_port();

            memcpy(&dest->mac[4], &port, 2);
        }
        dest->net = BACNET_BROADCAST_NETWORK;
        dest->len = 0;
    }
}

/**
 * @brief Send a raw BACnet/IP MPDU to a specific host and port
 * @param dest destination IPv4 address and port
 * @param mtu raw BVLC/NPDU buffer
 * @param mtu_len buffer length in bytes
 * @return number of bytes sent, or -1 on error
 */
int bip_send_mpdu(
    const BACNET_IP_ADDRESS *dest, const uint8_t *mtu, uint16_t mtu_len)
{
    return bip_socket_send(dest->address, dest->port, mtu, mtu_len);
}
