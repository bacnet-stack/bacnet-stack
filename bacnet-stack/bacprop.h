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

typedef struct
{
  unsigned prop_id; /* index number that matches the text */
  unsigned tag_id; /* text pair - use NULL to end the list */
} PROP_TAG_DATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

unsigned bacprop_tag_by_index_default(
  PROP_TAG_DATA *data_list,
  unsigned index,
  unsigned default_ret);

unsigned bacprop_property_tag(BACNET_OBJECT_TYPE type, unsigned prop);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
