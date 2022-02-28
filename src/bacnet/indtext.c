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
#include <stdbool.h>
#include <string.h>
#include "bacnet/indtext.h"

/** @file indtext.c  Maps text strings and indices of type INDTEXT_DATA */

#if !defined(__BORLANDC__) && !defined(_MSC_VER)
#include <ctype.h>
int stricmp(const char *s1, const char *s2)
{
    unsigned char c1, c2;

    do {
        c1 = (unsigned char)*s1;
        c2 = (unsigned char)*s2;
        c1 = (unsigned char)tolower(c1);
        c2 = (unsigned char)tolower(c2);
        s1++;
        s2++;
    } while ((c1 == c2) && (c1 != '\0'));

    return (int)c1 - c2;
}
#endif
#if defined(_MSC_VER)
#define stricmp _stricmp
#endif

bool indtext_by_string(
    INDTEXT_DATA *data_list, const char *search_name, unsigned *found_index)
{
    bool found = false;
    unsigned index = 0;

    if (data_list && search_name) {
        while (data_list->pString) {
            if (strcmp(data_list->pString, search_name) == 0) {
                index = data_list->index;
                found = true;
                break;
            }
            data_list++;
        }
    }

    if (found && found_index) {
        *found_index = index;
    }

    return found;
}

/* case insensitive version */
bool indtext_by_istring(
    INDTEXT_DATA *data_list, const char *search_name, unsigned *found_index)
{
    bool found = false;
    unsigned index = 0;

    if (data_list && search_name) {
        while (data_list->pString) {
            if (stricmp(data_list->pString, search_name) == 0) {
                index = data_list->index;
                found = true;
                break;
            }
            data_list++;
        }
    }

    if (found && found_index) {
        *found_index = index;
    }

    return found;
}

unsigned indtext_by_string_default(
    INDTEXT_DATA *data_list, const char *search_name, unsigned default_index)
{
    unsigned index = 0;

    if (!indtext_by_string(data_list, search_name, &index)) {
        index = default_index;
    }

    return index;
}

unsigned indtext_by_istring_default(
    INDTEXT_DATA *data_list, const char *search_name, unsigned default_index)
{
    unsigned index = 0;

    if (!indtext_by_istring(data_list, search_name, &index)) {
        index = default_index;
    }

    return index;
}

const char *indtext_by_index_default(
    INDTEXT_DATA *data_list, unsigned index, const char *default_string)
{
    const char *pString = NULL;

    if (data_list) {
        while (data_list->pString) {
            if (data_list->index == index) {
                pString = data_list->pString;
                break;
            }
            data_list++;
        }
    }

    return pString ? pString : default_string;
}

const char *indtext_by_index_split_default(INDTEXT_DATA *data_list,
    unsigned index,
    unsigned split_index,
    const char *before_split_default_name,
    const char *default_name)
{
    if (index < split_index) {
        return indtext_by_index_default(
            data_list, index, before_split_default_name);
    } else {
        return indtext_by_index_default(data_list, index, default_name);
    }
}

const char *indtext_by_index(INDTEXT_DATA *data_list, unsigned index)
{
    return indtext_by_index_default(data_list, index, NULL);
}

unsigned indtext_count(INDTEXT_DATA *data_list)
{
    unsigned count = 0; /* return value */

    if (data_list) {
        while (data_list->pString) {
            count++;
            data_list++;
        }
    }
    return count;
}
