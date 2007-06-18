/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2003 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to 
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
#ifndef KEY_H
#define KEY_H

#include <stdint.h>

// This file has the macros that encode and decode the
// keys for the keylist when used with BACnet Object Id's
typedef uint32_t KEY;

// assuming a 32 bit KEY
#define KEY_TYPE_OFFSET  22     /* bits - for BACnet */
#define KEY_TYPE_MASK    0x000003FFL
#define KEY_ID_MASK  0x003FFFFFL
#define KEY_ID_MAX  (KEY_ID_MASK + 1L)
#define KEY_TYPE_MAX  (KEY_TYPE_MASK + 1L)
#define KEY_LAST(key) ((key & KEY_ID_MASK) == KEY_ID_MAX)

#define KEY_ENCODE(type,id) ( ((unsigned int)\
((unsigned int)(type) & KEY_TYPE_MASK) << KEY_TYPE_OFFSET) |\
 ((unsigned int)(id) & KEY_ID_MASK) )

#define KEY_DECODE_TYPE(key) ((int)(((unsigned int)(key) >> KEY_TYPE_OFFSET)\
 & KEY_TYPE_MASK))

#define KEY_DECODE_ID(key) ((int)((unsigned int)(key) & KEY_ID_MASK))

#endif
