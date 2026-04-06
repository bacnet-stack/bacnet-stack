/**
 * @file
 * @brief UDP socket bridge used by BACnet/IP on the PlatformIO ESP32 port
 * @author Kato Gangstad
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#if defined(USE_ETH_INTERFACE) && (USE_ETH_INTERFACE)
#include <ETH.h>
#endif

extern "C" {
#include "bip.h"
}

static WiFiUDP BipUdp;
static bool BipSocketOpen = false;

/**
 * @brief Check whether the selected network interface is up
 * @return true if the interface is connected
 */
static bool network_connected(void)
{
#if defined(USE_ETH_INTERFACE) && (USE_ETH_INTERFACE)
    return ETH.linkUp();
#else
    return (WiFi.status() == WL_CONNECTED);
#endif
}

/**
 * @brief Get the local IP address for the selected network interface
 * @return local IP address
 */
static IPAddress network_local_ip(void)
{
#if defined(USE_ETH_INTERFACE) && (USE_ETH_INTERFACE)
    return ETH.localIP();
#else
    return WiFi.localIP();
#endif
}

/**
 * @brief Get the subnet mask for the selected network interface
 * @return subnet mask
 */
static IPAddress network_subnet_mask(void)
{
#if defined(USE_ETH_INTERFACE) && (USE_ETH_INTERFACE)
    return ETH.subnetMask();
#else
    return WiFi.subnetMask();
#endif
}

/**
 * @brief Initialize the UDP socket used by BACnet/IP
 * @param port UDP port to bind
 * @return true if the socket was opened successfully
 */
extern "C" bool bip_socket_init(uint16_t port)
{
    if (!network_connected()) {
        return false;
    }

    BipUdp.stop();
    BipSocketOpen = (BipUdp.begin(port) == 1);
    return BipSocketOpen;
}

/**
 * @brief Send a UDP packet for BACnet/IP
 * @param dest_addr destination IPv4 address
 * @param dest_port destination UDP port
 * @param mtu payload buffer
 * @param mtu_len payload length in bytes
 * @return number of bytes sent, or -1 on error
 */
extern "C" int bip_socket_send(
    const uint8_t *dest_addr,
    uint16_t dest_port,
    const uint8_t *mtu,
    uint16_t mtu_len)
{
    if (!BipSocketOpen || !dest_addr || !mtu || (mtu_len == 0U)) {
        return -1;
    }

    IPAddress ip(dest_addr[0], dest_addr[1], dest_addr[2], dest_addr[3]);
    if (!BipUdp.beginPacket(ip, dest_port)) {
        return -1;
    }

    size_t written = BipUdp.write(mtu, mtu_len);
    if (!BipUdp.endPacket()) {
        return -1;
    }

    return (written == mtu_len) ? (int)written : -1;
}

/**
 * @brief Receive a UDP packet for BACnet/IP
 * @param buf receive buffer
 * @param buf_len receive buffer size
 * @param src_addr source IPv4 address
 * @param src_port source UDP port
 * @return number of bytes received
 */
extern "C" int bip_socket_receive(
    uint8_t *buf, uint16_t buf_len, uint8_t *src_addr, uint16_t *src_port)
{
    if (!BipSocketOpen || !buf || !src_addr || !src_port || (buf_len == 0U)) {
        return 0;
    }

    int packet_size = BipUdp.parsePacket();
    if (packet_size <= 0) {
        return 0;
    }

    IPAddress remote = BipUdp.remoteIP();
    src_addr[0] = remote[0];
    src_addr[1] = remote[1];
    src_addr[2] = remote[2];
    src_addr[3] = remote[3];
    *src_port = BipUdp.remotePort();

    int to_read = packet_size;
    if (to_read > (int)buf_len) {
        to_read = (int)buf_len;
    }

    int len = BipUdp.read(buf, to_read);
    return (len > 0) ? len : 0;
}

/**
 * @brief Close the UDP socket used by BACnet/IP
 */
extern "C" void bip_socket_cleanup(void)
{
    BipUdp.stop();
    BipSocketOpen = false;
}

/**
 * @brief Get the local network address and subnet mask
 * @param local_addr destination IPv4 address buffer
 * @param netmask destination subnet mask buffer
 * @return true if network information was available
 */
extern "C" bool
bip_get_local_network_info(uint8_t *local_addr, uint8_t *netmask)
{
    if (!local_addr || !netmask || !network_connected()) {
        return false;
    }

    IPAddress ip = network_local_ip();
    IPAddress mask = network_subnet_mask();
    local_addr[0] = ip[0];
    local_addr[1] = ip[1];
    local_addr[2] = ip[2];
    local_addr[3] = ip[3];

    netmask[0] = mask[0];
    netmask[1] = mask[1];
    netmask[2] = mask[2];
    netmask[3] = mask[3];

    return true;
}
