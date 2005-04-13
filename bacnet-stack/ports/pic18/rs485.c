/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2004 Steve Karg

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
 Boston, MA  02111-1307
 USA.

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

// The module handles sending data out the RS-485 port
// and handles receiving data from the RS-485 port.
// Customize this file for your specific hardware
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "mstp.h"

// Transmits a Frame on the wire
void RS485_Send_Frame(
  struct mstp_port_struct_t *mstp_port, // port specific data
  uint8_t *buffer, // frame to send (up to 501 bytes of data)
  uint16_t nbytes) // number of bytes of data (up to 501)
{
  uint8_t HeaderCRC; // used for running CRC calculation

  (void)buffer; // FIXME: temp until we implement this code
  (void)nbytes; // FIXME: temp until we implement this code

  // in order to avoid line contention
  //while (mstp_port->SilenceTimer < Tturnaround)
  //{
    // wait, yield, or whatever
  //}

	// Disable the receiver, and enable the transmit line driver.

  // Transmit the preamble octets X'55', X'FF'. 
  // As each octet is transmitted, set SilenceTimer to zero.

  HeaderCRC = 0xFF;

  // Transmit the Frame Type, Destination Address, Source Address, 
  // and Data Length octets. Accumulate each octet into HeaderCRC. 
  // As each octet is transmitted, set SilenceTimer to zero.

  // Transmit the ones-complement of HeaderCRC. Set SilenceTimer to zero.

  // If there are data octets, initialize DataCRC to X'FFFF'.

  // Transmit any data octets. Accumulate each octet into DataCRC. 
  // As each octet is transmitted, set SilenceTimer to zero.

  // Transmit the ones-complement of DataCRC, least significant octet first. 
  // As each octet is transmitted, set SilenceTimer to zero.

  // Wait until the final stop bit of the most significant CRC octet 
  // has been transmitted but not more than Tpostdrive.

  // Disable the transmit line driver.

  return;
}

// called by timer, interrupt(?) or other thread
void RS485_Check_UART_Data(struct mstp_port_struct_t *mstp_port)
{
  if (mstp_port->ReceiveError == true)
  {
    // wait for state machine to clear this
  }
  // wait for state machine to read from the DataRegister
  else if (mstp_port->DataAvailable == false)
  {
    // check for data

    // if error,
    // ReceiveError = TRUE;
    // return;

    mstp_port->DataRegister = 0; // FIXME: Get this data from UART or buffer

    // if data is ready,
    // DataAvailable = TRUE;
    // return;
  }
}

