/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2007 Steve Karg

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
#ifndef VERSION_H
#define VERSION_H

/* This BACnet protocol stack version 0.0.0 - FF.FF.FF */
#ifndef BACNET_VERSION
#define BACNET_VERSION(x,y,z) (((x)<<16)+((y)<<8)+(z))
#endif
#define BACNET_VERSION_CODE BACNET_VERSION(0,3,4)
#define BACNET_VERSION_MAJOR ((BACNET_VERSION_CODE>>16)&0xFF)
#define BACNET_VERSION_MINOR ((BACNET_VERSION_CODE>>8)&0xFF)
#define BACNET_VERSION_MAINTENANCE (BACNET_VERSION_CODE&0xFF)
extern char *BACnet_Version;

#endif
