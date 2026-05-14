/**
 * @file
 * @brief State name utility functions for BACnet objects
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */

#include <string.h>

/* me! */
#include "state_name.h"

/**
 * @brief Count the number of states
 * @details The state names are stored as a list of state-text from
 * a C string array. The state_text_list consists of C strings separated
 * by '\0'. For example:
 * {@code
 * static const char *baud_rate_names = {
 *     "9600\0"
 *     "19200\0"
 *     "38400\0"
 *     "57600\0"
 *     "76800\0"
 *     "115200\0"
 * };
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
 * @brief Get the specific state name at index 0..N
 * @param state_names - string of null-terminated state names
 * @param state_index - state index number 1..N of the state names
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
