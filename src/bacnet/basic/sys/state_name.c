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
#include "bacnet/bacdcode.h"
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

    if (!list || !search_name) {
        return 0;
    }

    count = Keylist_Count(list);
    for (i = 0; i < count; i++) {
        name = Keylist_Data_Index(list, i);
        if (!name) {
            continue;
        }
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
 *  or NULL to clear the list
 * @return true if the entire state text list was set or cleared
 */
bool state_name_list_init(OS_Keylist list, const char *state_text_list)
{
    bool status = false;
    unsigned count = 0, i;
    char *name;
    int index;

    /* State Text in a Keylist */
    if (list) {
        /* flush existing state list and free the data */
        while (Keylist_Count(list) > 0) {
            name = Keylist_Data_Pop(list);
            if (name) {
                free(name);
            }
        }
        count = state_name_count(state_text_list);
        if (count) {
            for (i = 0; i < count; i++) {
                name =
                    bacnet_strdup(state_name_by_index(state_text_list, i + 1));
                index = Keylist_Data_Add(list, 1 + i, name);
                if (index < 0) {
                    free(name);
                    break;
                }
            }
        } else {
            /* clear the list */
            while (Keylist_Count(list) > 0) {
                name = Keylist_Data_Pop(list);
                if (name) {
                    free(name);
                }
            }
        }
        if (count == Keylist_Count(list)) {
            status = true;
        }
    }

    return status;
}

/**
 * @brief Sets the state text for an existing keylist and index
 * @param list - keylist to set the state text
 * @param text - state text to set
 * @param text_length - length of the state text
 * @param array_index - state index number 1..N to set the text for
 * @return true if the state text was set, false on failure
 */
bool state_name_list_set(
    OS_Keylist list, const char *text, size_t text_length, unsigned array_index)
{
    bool status = false;
    KEY key;
    char *entry;
    int index;

    key = array_index;
    index = Keylist_Index(list, key);
    if (index >= 0) {
        entry = Keylist_Data_Index(list, index);
        if (entry && (strlen(entry) == text_length) &&
            (strncmp(entry, text, text_length)) == 0) {
            /* same text; nothing to update */
            status = true;
        } else {
            /* replace the data in the list */
            free(entry);
            entry = bacnet_strndup(text, text_length);
            if (entry) {
                index = Keylist_Data_Set(list, key, entry);
                if (index >= 0) {
                    status = true;
                } else {
                    free(entry);
                }
            }
        }
    }

    return status;
}

/**
 * @brief WriteProperty handler for a BACnetARRAY of state names in a keylist
 *  where the BACnetARRAY is resizable.
 * @param list - keylist to set the state text
 * @param array_index - state index number 1..N to set the text for
 * @param array_size - new size of the array if array_index is 0,
 *  otherwise it is the current size of the array
 * @return BACnet error code
 */
BACNET_ERROR_CODE state_name_list_write_resizable(
    OS_Keylist list,
    BACNET_ARRAY_INDEX array_index,
    BACNET_UNSIGNED_INTEGER array_size,
    uint8_t *application_data,
    size_t application_data_len)
{
    BACNET_ERROR_CODE error_code = ERROR_CODE_SUCCESS;
    BACNET_CHARACTER_STRING_BUFFER value = { 0 };
    void *data;
    int len = 0;
    int count = 0;

    if (array_index == 0) {
        /* For resizable arrays, resize to the requested length.
            If the new size is larger, new elements are created
            and their values are initialized as defaults.
            If the new size is smaller, the array is truncated and
            elements beyond the new length are discarded. */
        count = Keylist_Count(list);
        if (array_size < count) {
            /* truncate the array */
            while (Keylist_Count(list) > array_size) {
                data =
                    Keylist_Data_Delete_By_Index(list, Keylist_Count(list) - 1);
                free(data);
            }
        } else if (array_size > count) {
            /* expand the array */
            while (Keylist_Count(list) < array_size) {
                if (Keylist_Data_Add(list, Keylist_Count(list) + 1, NULL) < 0) {
                    error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
                    break;
                }
            }
        }
    } else {
        len = bacnet_character_string_buffer_application_decode(
            application_data, application_data_len, &value);
        if (len > 0) {
            if (value.encoding == CHARACTER_ANSI_X34) {
                if (array_index > array_size) {
                    /* For resizable arrays, the array is expanded
                    automatically to accommodate the array_index.
                    Intermediate elements (if any) are initialized as defaults
                  */
                    while (Keylist_Count(list) < array_index) {
                        if (Keylist_Data_Add(
                                list, Keylist_Count(list) + 1, NULL) < 0) {
                            error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
                            break;
                        }
                    }
                }
                if (error_code == ERROR_CODE_SUCCESS) {
                    if (!state_name_list_set(
                            list, value.buffer, value.buffer_length,
                            array_index)) {
                        error_code = ERROR_CODE_NO_SPACE_TO_WRITE_PROPERTY;
                    }
                }
            } else {
                error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
            }
        } else {
            error_code = ERROR_CODE_INVALID_DATA_TYPE;
        }
    }

    return error_code;
}
