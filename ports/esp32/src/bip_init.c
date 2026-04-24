/**
 * @file
 * @brief BACnet/IP initialization helpers for the PlatformIO ESP32 port
 * @author Kato Gangstad
 */

#include <stdbool.h>
#include <stdint.h>

#include "bip.h"

static bool BIP_Debug = false;

/**
 * @brief Enable BACnet/IP debug mode
 */
void bip_debug_enable(void)
{
    BIP_Debug = true;
}

/**
 * @brief Disable BACnet/IP debug mode
 */
void bip_debug_disable(void)
{
    BIP_Debug = false;
}

/**
 * @brief Resolve a host name into an address when supported
 * @param host_name host name to resolve
 * @return resolved address, or 0 when unsupported by this port
 */
long bip_getaddrbyname(const char *host_name)
{
    (void)host_name;
    return 0;
}

/**
 * @brief Refresh the local and broadcast BACnet/IP addresses
 */
void bip_set_interface(void)
{
    uint8_t local_address[] = { 0, 0, 0, 0 };
    uint8_t broadcast_address[] = { 0, 0, 0, 0 };
    uint8_t netmask[] = { 0, 0, 0, 0 };
    uint8_t inverted_netmask[] = { 0, 0, 0, 0 };
    int i;

    if (!bip_get_local_network_info(local_address, netmask)) {
        return;
    }

    bip_set_addr(local_address);

    for (i = 0; i < 4; i++) {
        inverted_netmask[i] = (uint8_t)~netmask[i];
        broadcast_address[i] =
            (uint8_t)(local_address[i] | inverted_netmask[i]);
    }

    bip_set_broadcast_addr(broadcast_address);
}

/**
 * @brief Initialize the BACnet/IP datalink for the selected UDP port
 * @param port UDP port number
 * @return true if initialization succeeded
 */
bool bip_init(uint16_t port)
{
    (void)BIP_Debug;

    if (!bip_socket_init(port)) {
        return false;
    }

    bip_set_interface();
    bip_set_port(port);

    bip_set_socket(0);

    return true;
}

/**
 * @brief Shut down the BACnet/IP datalink
 */
void bip_cleanup(void)
{
    if (bip_valid()) {
        bip_socket_cleanup();
    }

    bip_set_broadcast_port(0);
}
