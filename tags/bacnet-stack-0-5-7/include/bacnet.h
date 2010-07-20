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

/** @file bacnet.h  This file is designed to reference the entire BACnet stack library */

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
#include "event.h"
#include "lso.h"
#include "alarm_ack.h"

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

/* Additions for Doxygen documenting */
/**
 * @mainpage BACnet-stack API Documentation
 * This documents the BACnet-Stack API, OS ports, and sample applications. <br>
 * 
 *  - The high-level handler interface can be found in the Modules tab.
 *  - Specifics for each file can be found in the Files tab.
 *  - A full list of all functions is provided in the index of the 
 *  Files->Globals subtab.
 * 
 * While all the central files are included in the file list, not all important
 * functions have been given the javadoc treatment, nor have Modules (chapters)
 * been created yet for all groupings.  If you are doing work in an under-
 * documented area, please add the javadoc comments at least to the API calls,
 * and consider adding doxygen's module grouping for your area of interest.
 * 
 * See doc/README.doxygen for notes on building and extending this document. <br>
 * In particular, if you have graphviz installed, you can enhance this 
 * documentation by turning on the function call graphs feature.
 */
#endif
