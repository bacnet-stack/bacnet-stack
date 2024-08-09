/**
 * @file
 * @brief API for index and text pairs lookup functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include "bacnet/bacdef.h"
#include "bacnet/indtext.h"

/**
 * @brief Compare two strings, case insensitive
 * @param a - first string
 * @param b - second string
 * @return 0 if the strings are equal, non-zero if not
 * @note The stricmp() function is not included C standard.
 */
int indtext_stricmp(const char *a, const char *b)
{
    int twin_a, twin_b;

    do {
        twin_a = *(const unsigned char *)a;
        twin_b = *(const unsigned char *)b;
        twin_a = tolower(toupper(twin_a));
        twin_b = tolower(toupper(twin_b));
        a++;
        b++;
    } while ((twin_a == twin_b) && (twin_a != '\0'));

    return twin_a - twin_b;
}

/**
 * @brief Search a list of strings to find a matching string
 * @param data_list - list of strings and indices
 * @param search_name - string to search for
 * @param found_index - index of the string found
 * @return true if the string is found
 */
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

/**
 * @brief Search a list of strings to find a matching string, case insensitive
 * @param data_list - list of strings and indices
 * @param search_name - string to search for
 * @param found_index - index of the string found
 * @return true if the string is found
 */
bool indtext_by_istring(
    INDTEXT_DATA *data_list, const char *search_name, unsigned *found_index)
{
    bool found = false;
    unsigned index = 0;

    if (data_list && search_name) {
        while (data_list->pString) {
            if (indtext_stricmp(data_list->pString, search_name) == 0) {
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

/**
 * @brief Search a list of strings to find a matching string,
 * or return a default index
 * @param data_list - list of strings and indices
 * @param search_name - string to search for
 * @param default_index - index to return if the string is not found
 * @return index of the string found, or the default index
 */
unsigned indtext_by_string_default(
    INDTEXT_DATA *data_list, const char *search_name, unsigned default_index)
{
    unsigned index = 0;

    if (!indtext_by_string(data_list, search_name, &index)) {
        index = default_index;
    }

    return index;
}

/**
 * @brief Search a list of strings to find a matching string, case insensitive
 * or return a default index
 * @param data_list - list of strings and indices
 * @param search_name - string to search for
 * @param default_index - index to return if the string is not found
 * @return index of the string found, or the default index
 */
unsigned indtext_by_istring_default(
    INDTEXT_DATA *data_list, const char *search_name, unsigned default_index)
{
    unsigned index = 0;

    if (!indtext_by_istring(data_list, search_name, &index)) {
        index = default_index;
    }

    return index;
}

/**
 * @brief Return the string for a given index
 * @param data_list - list of strings and indices
 * @param index - index to search for
 * @return the string found, or NULL if not found
 */
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

/**
 * @brief Return the string for a given index, or a default string
 * @param data_list - list of strings and indices
 * @param index - index to search for
 * @param before_split_default_name - default string to return if the index
 * is before the split_index
 * @param default_name - default string to return if the index is not found
 * @return the string found, or a default string
 */
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

/**
 * @brief Return the string for a given index
 * @param data_list - list of strings and indices
 * @param index - index to search for
 * @return the string found, or NULL if not found
 */
const char *indtext_by_index(INDTEXT_DATA *data_list, unsigned index)
{
    return indtext_by_index_default(data_list, index, NULL);
}

/**
 * @brief Return the number of elements in the list
 * @param data_list - list of strings and indices
 * @return the number of elements in the list
 */
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
