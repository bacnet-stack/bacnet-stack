/**
 * @file
 * @brief Function for filename manipulation
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdio.h>
#include <string.h>
#include "bacnet/basic/sys/filename.h"

const char *filename_remove_path(const char *filename_in)
{
    const char *filename_out = filename_in;

    /* allow the device ID to be set */
    if (filename_in) {
        filename_out = strrchr(filename_in, '\\');
        if (!filename_out) {
            filename_out = strrchr(filename_in, '/');
        }
        /* go beyond the slash */
        if (filename_out) {
            filename_out++;
        } else {
            /* no slash in filename */
            filename_out = filename_in;
        }
    }

    return filename_out;
}
