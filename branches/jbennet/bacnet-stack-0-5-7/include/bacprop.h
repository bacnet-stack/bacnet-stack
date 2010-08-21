/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 John Goulah

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
#ifndef BACPROP_H
#define BACPROP_H

#include <stdbool.h>
#include <stdint.h>
#include "bacenum.h"

typedef struct {
    signed object_id;   /* known object type, -1 for 'any' */
    signed prop_id;     /* index number that matches the text */
    signed tag_id;      /* text pair - use NULL to end the list */
    bool writable;      /* writable flag according to specification */
} PROP_TAG_DATA;

typedef struct {
    int PropertyId;     /* BACNET_PROPERTY_ID */
    int MinimumLength;  /* Minimum String Lenght as of ASHRAE 135-2008 table K-X4 */
} PROP_MINIMUM_LENGTH;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /*/ Returns minimum recommended character-string length for displaying properties */
    int bacprop_minimum_character_string_length(
        signed prop);

    /*/ Returns a known (Object, Property) application tag type */
    signed bacprop_property_tag(
        BACNET_OBJECT_TYPE type,
        signed prop);

    /*/ Returns true if a known (Object, Property) is writable */
    bool bacprop_property_tag_writeable(
        BACNET_OBJECT_TYPE type,
        signed prop);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
