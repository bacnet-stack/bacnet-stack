/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/

#include <stdint.h>             // for standard integer types uint8_t etc.
#include <stdbool.h>            // for the standard bool type.
#include <stdio.h>            // for the standard bool type.
#include <stdlib.h>            // for the standard bool type.
#include <rttarget.h>
#include <rtk32.h>
#include <clock.h>
#include <socket.h>
#include <windows.h>
#include "ethernet.h"
#include "bacdcode.h"

// commonly used comparison address for ethernet
uint8_t Ethernet_Broadcast[MAX_MAC_LEN] =
    { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
// commonly used empty address for ethernet quick compare
uint8_t Ethernet_Empty_MAC[MAX_MAC_LEN] = { 0, 0, 0, 0, 0, 0 };
// my local device data - MAC address
uint8_t Ethernet_MAC_Address[MAX_MAC_LEN] = { 0, 0, 0, 0, 0, 0 };

// static IP address assignment (default)
static BYTE TargetIP[]       = {192, 168,   0,  50};
// net mask - set to be subnet restrictive
static BYTE NetMask[]        = {255, 255, 255,   0};
// gateway - set to zero if not available or required
static BYTE DefaultGateway[] = {0, 0,  0,  0};
// DNS - set to zero if not available or required
static BYTE DNSServer[]      = {0, 0,  0,  0};
// the Interface for Ethernet
// SOCKET_ERROR means no open interface
static int Ethernet_Interface = SOCKET_ERROR;
static SOCKET Ethernet_Socket = -1;
// used for binding 802.2
static struct sockaddr Ethernet_Address = { 0 };

bool ethernet_valid(void)
{
  return (Ethernet_Interface != SOCKET_ERROR);
}

void ethernet_cleanup(void)
{
  if (ethernet_valid())
      xn_interface_close(Ethernet_Interface);
  Ethernet_Interface = SOCKET_ERROR;

  return;    
}

/* function to find the local ethernet MAC address */
static int get_local_hwaddr(int iface, unsigned char *mac)
{
    struct _iface_info ii; // contains the hwaddr of the Ethernet interface

    /* determine the local MAC address */
    xn_interface_info(iface, &ii);
    mac[0] = ii.my_ethernet_address[0];
    mac[1] = ii.my_ethernet_address[1];
    mac[2] = ii.my_ethernet_address[2];
    mac[3] = ii.my_ethernet_address[3];
    mac[4] = ii.my_ethernet_address[4];
    mac[5] = ii.my_ethernet_address[5];

    return 0;
}

static void ethernet_error(const char *text)
{
   fprintf(stderr,"%s, error code: %s\n", 
      text, xn_geterror_string(WSAGetLastError()));
   exit(1);
}

bool ethernet_init(char *interface_name)
{
    struct _iface_info ii; // contains the hwaddr of the Ethernet interface
    int value = 1;  
    int Result = 0;
    
    // FIXME: what about other drivers other than DAVICOM?
    (void)interface_name;
    RTKernelInit(0); // get the kernel going
    if (!RTKDebugVersion())            // switch of all diagnostics and error messages of RTIP-32
      xn_callbacks()->cb_wr_screen_string_fnc = NULL;
    
    CLKSetTimerIntVal(10*1000);        // 10 millisecond tick
    RTKDelay(1);
    RTCMOSSetSystemTime();             // get the right time-of-day

    Result = xn_rtip_init();           // Initialize the RTIP stack
    if (Result == SOCKET_ERROR)
        ethernet_error("ethernet: xn_rtip_init failed");
    atexit(ethernet_cleanup);                          // make sure the driver is shut down properly
    RTCallDebugger(RT_DBG_CALLRESET, (DWORD)exit, 0);  // even if we get restarted by the debugger

    // tell RTIP what Ethernet driver we want 
    Result = xn_bind_davicom(MINOR_0);
    if (Result != 0)
      ethernet_error("ethernet: driver initialization failed");
    // PCI device ignores the IRQ and IO parameters
    Ethernet_Interface = xn_interface_open_config(
      DAVICOM_DEVICE, MINOR_0, 0, 0, 0);
    if (Ethernet_Interface == SOCKET_ERROR)
    {
      fprintf(stderr,"ethernet: Davicom driver failed to initialize\r\n");
      return false;
    }
    if (xn_interface_opt(Ethernet_Interface,IO_802_2,
      (const char *)&value,sizeof(value)))
      fprintf(stderr,"ethernet: xn_interface_opt 802.2 failed \n"); 
    xn_interface_info(Ethernet_Interface, &ii);
    printf("ethernet: MAC address: %02x-%02x-%02x-%02x-%02x-%02x\n",
       ii.my_ethernet_address[0], ii.my_ethernet_address[1],
       ii.my_ethernet_address[2], ii.my_ethernet_address[3],
       ii.my_ethernet_address[4], ii.my_ethernet_address[5]);
    // Set the IP address and interface
    printf("ethernet: static IP address %i.%i.%i.%i\n",
      TargetIP[0], TargetIP[1], TargetIP[2], TargetIP[3]);
    if (xn_set_ip(Ethernet_Interface, TargetIP, NetMask) == SOCKET_ERROR)
    {
      // FIXME: is this because of a duplicate address?  Tell user...
      fprintf(stderr,"ethernet: failed to set IP address!\r\n");
      return false;
    }
    // add a route in the routing table
    // ip_ffaddr is apparently some global...
    xn_rt_add(RT_DEFAULT, ip_ffaddr, DefaultGateway, 1,
      Ethernet_Interface, RT_INF);
    xn_set_server_list((DWORD*)DNSServer, 1);

    // setup the socket
    Ethernet_Socket = socket(AF_INET, SOCK_RAW, 0);
    if (Ethernet_Socket < 0)
      fprintf(stderr,"ethernet: failed to bind to socket!\r\n");
    Ethernet_Address.sa_family = AF_INET;
    memset(Ethernet_Address.sa_data,0,sizeof(Ethernet_Address.sa_data));
    if (bind(Ethernet_Socket, 
        &Ethernet_Address, sizeof(Ethernet_Address)) == SOCKET_ERROR)
      fprintf(stderr,"ethernet: failed to bind to socket!\r\n");
    setsockopt(Ethernet_Socket,SOL_SOCKET,SO_802_2,(char *)&value,sizeof(value));    

    return ethernet_valid(); 
}

/* function to send a packet out the 802.2 socket */
/* returns 0 on success, non-zero on failure */
int ethernet_send(
  BACNET_ADDRESS *dest,  // destination address
  BACNET_ADDRESS *src,  // source address
  uint8_t *pdu, // any data to be sent - may be null
  unsigned pdu_len) // number of bytes of data
{
    int status = -1;
    int bytes = 0;
    uint8_t mtu[MAX_MPDU] = { 0 };
    int mtu_len = 0;
    int i = 0;

    // don't waste time if the socket is not valid
    if (Ethernet_Socket < 0)
    {
        fprintf(stderr, "ethernet: 802.2 socket is invalid!\n");
        return status;
    }
    /* load destination ethernet MAC address */
    if (dest->mac_len == 6)
    {
      for (i = 0; i < 6; i++)
      {
        mtu[mtu_len] = dest->mac[i];
        mtu_len++;
      }
    }
    else
    {
        fprintf(stderr, "ethernet: invalid destination MAC address!\n");
        return status;
    }

    /* load source ethernet MAC address */
    if (src->mac_len == 6)
    {
      for (i = 0; i < 6; i++)
      {
        mtu[mtu_len] = src->mac[i];
        mtu_len++;
      }
    }
    else
    {
        fprintf(stderr, "ethernet: invalid source MAC address!\n");
        return status;
    }
    if ((14 + 3 + pdu_len) > MAX_MPDU)
    {
        fprintf(stderr, "ethernet: PDU is too big to send!\n");
        return status;
    }
    /* packet length */
    mtu_len += encode_unsigned16(&mtu[12], 
      3 /*DSAP,SSAP,LLC*/ + pdu_len);
    // Logical PDU portion
    mtu[mtu_len++] = 0x82; /* DSAP for BACnet */
    mtu[mtu_len++] = 0x82; /* SSAP for BACnet */
    mtu[mtu_len++] = 0x03; /* Control byte in header */
    memcpy(&mtu[mtu_len], pdu, pdu_len);
    mtu_len += pdu_len;
    
    /* Send the packet */
    bytes =
        sendto(Ethernet_Socket, (const char *)&mtu, mtu_len, 0,
            &Ethernet_Address, sizeof(Ethernet_Address));
    /* did it get sent? */
    if (bytes < 0) {            
      fprintf(stderr,"ethernet: Error sending packet: %s\n", 
        strerror(errno));
      return status;
    }
    
    // got this far - must be good!
    status = 0;

    return status;
}

/* function to send a packet out the 802.2 socket */
/* returns zero on success, non-zero on failure */
int ethernet_send_pdu(
  BACNET_ADDRESS *dest,  // destination address
  uint8_t *pdu, // any data to be sent - may be null
  unsigned pdu_len) // number of bytes of data
{
  int i = 0; // counter
  BACNET_ADDRESS src = {0};  // source address
    
  for (i = 0; i < 6; i++)
  {
    src.mac[i] = Ethernet_MAC_Address[i];
    src.mac_len++;
  }
  /* function to send a packet out the 802.2 socket */
  /* returns 1 on success, 0 on failure */
  return ethernet_send(dest,  // destination address
    &src,  // source address
    pdu, // any data to be sent - may be null
    pdu_len); // number of bytes of data
}

// receives an 802.2 framed packet
// returns the number of octets in the PDU, or zero on failure
uint16_t ethernet_receive(
  BACNET_ADDRESS *src,  // source address
  uint8_t *pdu, // PDU data
  uint16_t max_pdu, // amount of space available in the PDU 
  unsigned timeout) // number of milliseconds to wait for a packet
{
    int received_bytes;
    uint8_t buf[MAX_MPDU] = {0}; // data
    uint16_t pdu_len = 0; // return value
    fd_set read_fds;
    int max;
    struct timeval select_timeout;

    /* Make sure the socket is open */
    if (Ethernet_Socket <= 0)
        return 0;

    /* we could just use a non-blocking socket, but that consumes all
       the CPU time.  We can use a timeout; it is only supported as
       a select. */
    if (timeout >= 1000)
    {
        select_timeout.tv_sec = timeout / 1000;
        select_timeout.tv_usec = 
          1000 * (timeout - select_timeout.tv_sec * 1000);
    }
    else
    {
        select_timeout.tv_sec = 0;
        select_timeout.tv_usec = 1000 * timeout;
    }
    FD_ZERO(&read_fds);
    FD_SET(Ethernet_Socket, &read_fds);
    max = Ethernet_Socket;

    if (select(max + 1, &read_fds, NULL, NULL, &select_timeout) > 0)
        received_bytes = recv(Ethernet_Socket, (char *)&buf[0], MAX_MPDU, 0);
    else
        return 0;

    /* See if there is a problem */
    if (received_bytes < 0) {
        // EAGAIN Non-blocking I/O has been selected 
        // using O_NONBLOCK and no data
        // was immediately available for reading.
        if (errno != EAGAIN)
            fprintf(stderr,"ethernet: Read error in receiving packet: %s\n",
                strerror(errno));
        return 0;
    }

    if (received_bytes == 0)
      return 0;
    
    /* the signature of an 802.2 BACnet packet */
    if ((buf[14] != 0x82) && (buf[15] != 0x82)) {
        //fprintf(stderr,"ethernet: Non-BACnet packet\n");
        return 0;
    }
    // copy the source address
    src->mac_len = 6;
    memmove(src->mac, &buf[6], 6);

    // check destination address for when
    // the Ethernet card is in promiscious mode
    if ((memcmp(&buf[0], Ethernet_MAC_Address,6) != 0)
        && (memcmp(&buf[0], Ethernet_Broadcast, 6) != 0)) 
    {
        //fprintf(stderr, "ethernet: This packet isn't for us\n");
        return 0;
    }

    (void)decode_unsigned16(&buf[12],&pdu_len);
    pdu_len -= 3 /* DSAP, SSAP, LLC Control */ ;
    // copy the buffer into the PDU
    if (pdu_len < max_pdu)
      memmove(&pdu[0],&buf[17],pdu_len);

    return pdu_len;
}

void ethernet_get_my_address(BACNET_ADDRESS *my_address)
{
  int i = 0;
  
  my_address->mac_len = 0;
  for (i = 0; i < 6; i++)
  {
    my_address->mac[i] = Ethernet_MAC_Address[i];
    my_address->mac_len++;
  }
  my_address->net = 0; // local only, no routing
  my_address->len = 0;
  for (i = 0; i < MAX_MAC_LEN; i++)
  {
    my_address->adr[i] = 0;
  }

  return;
}

void ethernet_set_broadcast_address(
  BACNET_ADDRESS *dest)  // destination address
{
  int i = 0; // counter
  
  if (dest)
  {
    for (i = 0; i < 6; i++)
    {
      dest->mac[i] = Ethernet_Broadcast[i];
    }
    dest->mac_len = 6;
    dest->net = BACNET_BROADCAST_NETWORK;
    dest->len = 0; // denotes broadcast address 
    for (i = 0; i < MAX_MAC_LEN; i++)
    {
      dest->adr[i] = 0;
    }
  }

  return;
}

void ethernet_debug_address(
  const char *info,
  BACNET_ADDRESS *dest)
{
  int i = 0; // counter

  if (info)
    fprintf(stderr,"%s",info);
  if (dest)
  {
    fprintf(stderr,"Address:\n");
    fprintf(stderr,"  MAC Length=%d\n",dest->mac_len);
    fprintf(stderr,"  MAC Address=");
    for (i = 0; i < MAX_MAC_LEN; i++)
    {
      fprintf(stderr,"%02X ",(unsigned)dest->mac[i]);
    }
    fprintf(stderr,"\n");
    fprintf(stderr,"  Net=%hu\n",dest->net);
    fprintf(stderr,"  Len=%d\n",dest->len);
    fprintf(stderr,"  Adr=");
    for (i = 0; i < MAX_MAC_LEN; i++)
    {
      fprintf(stderr,"%02X ",(unsigned)dest->adr[i]);
    }
    fprintf(stderr,"\n");
  }

  return;
}

