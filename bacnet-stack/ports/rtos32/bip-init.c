/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/

#include <stdint.h>             // for standard integer types uint8_t etc.
#include <stdbool.h>            // for the standard bool type.
#include "bip.h"

bool bip_init(void)
{
    int rv = 0; // return from socket lib calls
    struct sockaddr_in sin = {-1};
    int value = 1;

    // assumes that the driver has already been initialized
    BIP_Socket = socket(AF_INET, SOCK_DGRAM, IPROTO_UDP);
    if (BIP_Socket < 0)
        return false;

    // bind the socket to the local port number and IP address
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(bip_get_port());
    memset(&(sin.sin_zero), '\0', 8);
    rv = bind(BIP_Socket, 
        (const struct sockaddr*)&sin, sizeof(struct sockaddr));
    if (rv < 0)
    {
        close(BIP_Socket);
        BIP_Socket = -1;
        return false;
    }

    return true; 
}
