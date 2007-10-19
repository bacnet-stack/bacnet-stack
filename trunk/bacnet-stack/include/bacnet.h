/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2006 Steve Karg

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
#ifndef BACNET_H
#define BACNET_H

/* This file is designed to reference the entire BACnet stack library */

/* core files */
#include "version.h"
#include "config.h"
#include "address.h"
#include "apdu.h"
#include "bacapp.h"
#include "bacdcode.h"
#include "bacint.h"
#include "bacreal.h"
#include "bacstr.h"
#include "bacdef.h"
#include "bacenum.h"
#include "bacerror.h"
#include "bactext.h"
#include "datalink.h"
#include "indtext.h"
#include "npdu.h"
#include "reject.h"
#include "tsm.h"

/* services */
#include "arf.h"
#include "awf.h"
#include "cov.h"
#include "dcc.h"
#include "iam.h"
#include "ihave.h"
#include "rd.h"
#include "rp.h"
#include "rpm.h"
#include "timesync.h"
#include "whohas.h"
#include "whois.h"
#include "wp.h"

/* required object - note: developer must supply the device.c file
   since it is not included in the library.  However, the library
   references the device.c members via the device.h API. */
#include "device.h"

/* demo objects */
#include "ai.h"
#include "ao.h"
#include "av.h"
#include "bacfile.h"
#include "bi.h"
#include "bo.h"
#include "bv.h"
#include "lc.h"
#include "lsp.h"
#include "mso.h"

/* demo handlers */
#include "txbuf.h"
#include "client.h"
#include "handlers.h"

#endif
