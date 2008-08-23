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
#ifndef INDTEXT_H
#define INDTEXT_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* index and text pairs */
typedef struct {
    unsigned index;     /* index number that matches the text */
    const char *pString;        /* text pair - use NULL to end the list */
} INDTEXT_DATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*  Searches for a matching string and returns the index to the string
    in the parameter found_index.
    If the string is not found, false is returned
    If the string is found, true is returned and the found_index contains
    the first index where the string was found. */
    bool indtext_by_string(
        INDTEXT_DATA * data_list,
        const char *search_name,
        unsigned *found_index);
/* case insensitive version */
    bool indtext_by_istring(
        INDTEXT_DATA * data_list,
        const char *search_name,
        unsigned *found_index);
/*  Searches for a matching string and returns the index to the string
    or the default_index if the string is not found. */
    unsigned indtext_by_string_default(
        INDTEXT_DATA * data_list,
        const char *search_name,
        unsigned default_index);
/* case insensitive version */
    unsigned indtext_by_istring_default(
        INDTEXT_DATA * data_list,
        const char *search_name,
        unsigned default_index);
/* for a given index, return the matching string,
   or NULL if not found */
    const char *indtext_by_index(
        INDTEXT_DATA * data_list,
        unsigned index);
/* for a given index, return the matching string,
   or default_name if not found */
    const char *indtext_by_index_default(
        INDTEXT_DATA * data_list,
        unsigned index,
        const char *default_name);
/* for a given index, return the matching string,
   or default_name if not found.
   if the index is before the split,
   the before_split_default_name is used */
    const char *indtext_by_index_split_default(
        INDTEXT_DATA * data_list,
        int index,
        int split_index,
        const char *before_split_default_name,
        const char *default_name);

/* returns the number of elements in the list */
    unsigned indtext_count(
        INDTEXT_DATA * data_list);

#ifdef TEST
#include "ctest.h"
    void testIndexText(
        Test * pTest);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
