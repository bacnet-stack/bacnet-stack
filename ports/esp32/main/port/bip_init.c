//
// Copyleft  F.Chaxel 2017
//

#include "esp_log.h"
#include "esp_wifi.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include "bacnet/datalink/bip.h"

/* Include BIP_Socket parts from old version:*/
static int BIP_Socket = -1;

/** Setter for the BACnet/IP socket handle.
 *
 * @param sock_fd [in] Handle for the BACnet/IP socket.
 */
void bip_set_socket(
    int sock_fd)
{
    BIP_Socket = sock_fd;
}
/** Getter for the BACnet/IP socket handle.
 *
 * @return The handle to the BACnet/IP socket.
 */
int bip_socket(
    void)
{
    return BIP_Socket;
}

/* 
long bip_get_addr_by_name(const char *host_name)  //already defined into datalink/bip.h
{
    return 0;
}
 */

void bip_set_interface(char *ifname)
{
}

void bip_cleanup(void)
{
    close(bip_socket());
    bip_set_socket(-1);
}
/************************added files*************/


/********************************************/
bool bip_init(char *ifname)
{
    tcpip_adapter_ip_info_t ip_info = { 0 };

    int value = 1;

    tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info);

    bip_set_interface(ifname);
    bip_set_port(0xBAC0U);
    bip_set_addr(ip_info.ip.addr);
    bip_set_broadcast_addr(
        (ip_info.ip.addr & ip_info.netmask.addr) | (~ip_info.netmask.addr));

    int sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
    struct sockaddr_in saddr = { 0 };

    saddr.sin_family = PF_INET;
    saddr.sin_port = htons(0xBAC0U);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr_in));

    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&value, sizeof(value));
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&value, sizeof(value));

    bip_set_socket(sock);

    return true;
}
/******* Add parts from bip.c */

