/**
 * @file
 * @brief State name utility functions for BACnet objects
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "bacnet/bacstr.h"
/* me! */
#include "bacnet/basic/sys/state_name.h"

/**
 * @brief Count the number of states
 * @details The state names are stored as a list of state-text from
 * a C string array. The state_text_list consists of C strings separated
 * by '\0'. For example:
 * @code
 * static const char baud_rate_names[] =
 *     "9600\0"
 *     "19200\0"
 *     "38400\0"
 *     "57600\0"
 *     "76800\0"
 *     "115200\0"
 *     "\0";
 * @endcode
 *
 * @param state_names - string of null-terminated state names
 * @return number of states
 */
unsigned state_name_count(const char *state_names)
{
    unsigned count = 0;
    int len = 0;

    if (state_names) {
        do {
            len = strlen(state_names);
            if (len > 0) {
                count++;
                state_names = state_names + len + 1;
            }
        } while (len > 0);
    }

    return count;
}

/**
 * @brief Get the specific state name at index 1..N
 * @param state_names - string of null-terminated state names
 * @param index - state index number 1..N of the state names
 * @return state name, or NULL
 */
const char *state_name_by_index(const char *state_names, unsigned index)
{
    unsigned count = 0;
    int len = 0;

    if (state_names) {
        do {
            len = strlen(state_names);
            if (len > 0) {
                count++;
                if (index == count) {
                    return state_names;
                }
                state_names = state_names + len + 1;
            }
        } while (len > 0);
    }

    return NULL;
}

/**
 * @brief Get the specific state index from a given name
 * @param state_names - string of null-terminated state names
 * @param state_name - state name to find the index for
 * @return state index number 1..N, or 0 if not found
 */
unsigned state_name_to_index(const char *state_names, const char *state_name)
{
    unsigned count = 0;
    int len = 0;

    if (state_names && state_name) {
        do {
            len = strlen(state_names);
            if (len > 0) {
                count++;
                if (strcmp(state_names, state_name) == 0) {
                    return count;
                }
                state_names = state_names + len + 1;
            }
        } while (len > 0);
    }

    return 0;
}

/**
 * @brief Determines the number of state texts in a keylist
 * @param  list - keylist of state texts
 * @return  number of state texts
 */
unsigned state_name_list_count(OS_Keylist list)
{
    return Keylist_Count(list);
}

/**
 * Find the index of the state text in the state list for
 * a given state text name.
 * @param list - keylist of state text
 * @param search_name - state text name to find the index for
 * @return state index 1..N, or 0 if not found
 */
unsigned state_name_list_index(OS_Keylist list, const char *search_name)
{
    unsigned count, i, index = 0;
    KEY key;
    const char *name;

    count = Keylist_Count(list);
    for (i = 0; i < count; i++) {
        name = Keylist_Data_Index(list, i);
        if (strcmp(name, search_name) == 0) {
            if (Keylist_Index_Key(list, i, &key)) {
                index = key;
            }
            break;
        }
    }

    return index;
}

/**
 * @brief Sets the state text list for a given keylist
 * @param list - keylist to set the state text
 * @param state_text_list - array of state names to use in this keylist
 * @return number of states set
 */
unsigned state_name_list_init(OS_Keylist list, const char *state_text_list)
{
    unsigned count, i;
    char *name;
    int index;

    /* State Text in a Keylist */
    if (list) {
        /* flush existing state list and free the data */
        while ((name = Keylist_Data_Pop(list))) {
            free(name);
        }
    } else {
        list = Keylist_Create();
    }
    count = state_name_count(state_text_list);
    for (i = 0; i < count; i++) {
        name = bacnet_strdup(state_name_by_index(state_text_list, i));
        index = Keylist_Data_Add(list, 1 + i, name);
        if (index < 0) {
            free(name);
            break;
        }
    }

    return count;
}

bool state_name_list_set(
    OS_Keylist list, const char *text, size_t text_length, unsigned array_index)
{
    bool status = false;
    KEY key;
    char *entry;

    key = array_index;
    entry = Keylist_Data(list, key);
    /* same value? */
    if (entry) {
        if (strncmp(entry, text, text_length) == 0) {
            status = true;
        } else {
            /* update the value in the list */
            free(entry);
            entry = bacnet_strndup(text, text_length);
            if (entry) {
                Keylist_Data_Set(list, key, entry);
                status = true;
            } else {
                status = false;
            }
        }
    } else {
        /* add a new entry to the list */
        entry = bacnet_strndup(text, text_length);
        if (entry) {
            Keylist_Data_Set(list, key, entry);
            status = true;
        } else {
            status = false;
        }
    }

    return status;
}
