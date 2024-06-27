/**
 * @file
 * @brief API for index and text pairs lookup functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
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
