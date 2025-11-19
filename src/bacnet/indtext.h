/**
 * @file
 * @brief API for index and text pairs lookup functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_INDEX_TEXT_H
#define BACNET_INDEX_TEXT_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

/* index and text pairs */
typedef const struct {
    const uint32_t index; /* index number that matches the text */
    const char *pString; /* text pair - use NULL to end the list */
} INDTEXT_DATA;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*  Searches for a matching string and returns the index to the string
    in the parameter found_index.
    If the string is not found, false is returned
    If the string is found, true is returned and the found_index contains
    the first index where the string was found. */
BACNET_STACK_EXPORT
bool indtext_by_string(
    INDTEXT_DATA *data_list, const char *search_name, uint32_t *found_index);
/* case insensitive version */
BACNET_STACK_EXPORT
bool indtext_by_istring(
    INDTEXT_DATA *data_list, const char *search_name, uint32_t *found_index);
/*  Searches for a matching string and returns the index to the string
    or the default_index if the string is not found. */
BACNET_STACK_EXPORT
uint32_t indtext_by_string_default(
    INDTEXT_DATA *data_list, const char *search_name, uint32_t default_index);
/* case insensitive version */
BACNET_STACK_EXPORT
uint32_t indtext_by_istring_default(
    INDTEXT_DATA *data_list, const char *search_name, uint32_t default_index);
/* for a given index, return the matching string,
   or NULL if not found */
BACNET_STACK_EXPORT
const char *indtext_by_index(INDTEXT_DATA *data_list, uint32_t index);
/* for a given index, return the matching string,
   or default_name if not found */
BACNET_STACK_EXPORT
const char *indtext_by_index_default(
    INDTEXT_DATA *data_list, uint32_t index, const char *default_name);
/* for a given index, return the matching string,
   or default_name if not found.
   if the index is before the split,
   the before_split_default_name is used */
BACNET_STACK_EXPORT
const char *indtext_by_index_split_default(
    INDTEXT_DATA *data_list,
    uint32_t index,
    uint32_t split_index,
    const char *before_split_default_name,
    const char *default_name);

/* returns the number of elements in the list */
BACNET_STACK_EXPORT
uint32_t indtext_count(INDTEXT_DATA *data_list);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
