/**
 * @file
 * @brief BACnet/IP datalink interface for the PlatformIO ESP32 port
 * @author Kato Gangstad
 */

#ifndef M5STAMPLC_BIP_H
#define M5STAMPLC_BIP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "bacnet/bacdef.h"
#include "bacnet/npdu.h"
#include "bacnet/datalink/bvlc.h"

#define BIP_HEADER_MAX (1 + 1 + 2)
#ifndef BIP_MPDU_MAX
#define BIP_MPDU_MAX (BIP_HEADER_MAX + MAX_PDU)
#endif

#define BVLL_TYPE_BACNET_IP (0x81)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the UDP socket backend used by BACnet/IP
 * @param port UDP port to bind
 * @return true if the socket backend was initialized
 */
bool bip_socket_init(uint16_t port);

/**
 * @brief Send a raw UDP payload for BACnet/IP
 * @param dest_addr destination IPv4 address
 * @param dest_port destination UDP port
 * @param mtu payload buffer
 * @param mtu_len payload length
 * @return number of bytes sent, or -1 on error
 */
int bip_socket_send(
    const uint8_t *dest_addr,
    uint16_t dest_port,
    const uint8_t *mtu,
    uint16_t mtu_len);
/**
 * @brief Receive a raw UDP payload for BACnet/IP
 * @param buf receive buffer
 * @param buf_len receive buffer size
 * @param src_addr source IPv4 address
 * @param src_port source UDP port
 * @return number of bytes received
 */
int bip_socket_receive(
    uint8_t *buf, uint16_t buf_len, uint8_t *src_addr, uint16_t *src_port);

/**
 * @brief Close the UDP socket backend used by BACnet/IP
 */
void bip_socket_cleanup(void);

/**
 * @brief Read the current local IP address and subnet mask
 * @param local_addr destination for the local IPv4 address
 * @param netmask destination for the subnet mask
 * @return true if the network information was available
 */
bool bip_get_local_network_info(uint8_t *local_addr, uint8_t *netmask);

/**
 * @brief Initialize the BACnet/IP datalink
 * @param port UDP port to use
 * @return true if initialization succeeded
 */
bool bip_init(uint16_t port);

/**
 * @brief Refresh the local and broadcast interface addresses
 */
void bip_set_interface(void);

/**
 * @brief Shut down the BACnet/IP datalink
 */
void bip_cleanup(void);

/**
 * @brief Convert a 4-byte address into a 32-bit value
 * @param bip_address pointer to IPv4 address bytes
 * @return packed address value
 */
uint32_t convertBIP_Address2uint32(const uint8_t *bip_address);

/**
 * @brief Convert a 32-bit address into a 4-byte array
 * @param ip packed address value
 * @param address destination buffer
 */
void convertUint32Address_2_uint8Address(uint32_t ip, uint8_t *address);

/**
 * @brief Store the current socket identifier
 * @param sock_fd socket identifier
 */
void bip_set_socket(uint8_t sock_fd);

/**
 * @brief Get the current socket identifier
 * @return socket identifier
 */
uint8_t bip_socket(void);

/**
 * @brief Check whether the BACnet/IP datalink is valid
 * @return true if the datalink is ready to use
 */
bool bip_valid(void);

/**
 * @brief Build the broadcast BACnet/IP address
 * @param dest destination address structure
 */
void bip_get_broadcast_address(BACNET_ADDRESS *dest);

/**
 * @brief Build the local BACnet/IP address
 * @param my_address destination address structure
 */
void bip_get_my_address(BACNET_ADDRESS *my_address);

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
    unsigned pdu_len);

/**
 * @brief Send a raw BACnet/IP MPDU
 * @param dest destination IP address and port
 * @param mtu raw buffer to send
 * @param mtu_len buffer length in bytes
 * @return number of bytes sent, or -1 on error
 */
int bip_send_mpdu(
    const BACNET_IP_ADDRESS *dest, const uint8_t *mtu, uint16_t mtu_len);

/**
 * @brief Receive a BACnet/IP NPDU
 * @param src source BACnet address
 * @param pdu receive buffer
 * @param max_pdu receive buffer size
 * @param timeout receive timeout in milliseconds
 * @return number of NPDU bytes received
 */
uint16_t bip_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout);

/**
 * @brief Store the UDP port used by BACnet/IP
 * @param port UDP port number
 */
void bip_set_port(uint16_t port);

/**
 * @brief Store the UDP broadcast destination port used by BACnet/IP
 * @param port UDP port number
 */
void bip_set_broadcast_port(uint16_t port);

/**
 * @brief Get the UDP port used by BACnet/IP
 * @return UDP port number
 */
uint16_t bip_get_port(void);

/**
 * @brief Get the UDP broadcast destination port used by BACnet/IP
 * @return UDP port number
 */
uint16_t bip_get_broadcast_port(void);

/**
 * @brief Store the local IPv4 address
 * @param net_address pointer to the 4-byte address
 */
void bip_set_addr(const uint8_t *net_address);

/**
 * @brief Get the local IPv4 address
 * @return pointer to the stored address
 */
uint8_t *bip_get_addr(void);

/**
 * @brief Store the IPv4 broadcast address
 * @param net_address pointer to the 4-byte broadcast address
 */
void bip_set_broadcast_addr(const uint8_t *net_address);

/**
 * @brief Get the stored IPv4 broadcast address
 * @return pointer to the stored broadcast address
 */
uint8_t *bip_get_broadcast_addr(void);

/**
 * @brief Resolve a host name into an address when supported
 * @param host_name host name to resolve
 * @return resolved address value, or 0 when unsupported
 */
long bip_getaddrbyname(const char *host_name);

/**
 * @brief Enable BACnet/IP debug behavior
 */
void bip_debug_enable(void);

/**
 * @brief Disable BACnet/IP debug behavior
 */
void bip_debug_disable(void);

#ifdef __cplusplus
}
#endif

#endif
