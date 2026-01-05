/**
 * @file
 * @brief Function for filename and path manipulation and validation
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 */
#include <stdio.h>
#include <string.h>
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/filename.h"

/* restrict file paths */
#ifndef BACNET_FILE_PATH_RESTRICTED
#define BACNET_FILE_PATH_RESTRICTED 1
#endif

/**
 * @brief Remove path from filename
 * @param filename_in - input filename with path
 * @return filename without path
 */
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

/**
 * @brief Validate if pathname is valid by checking for patterns
 *  such as relative paths and absolute paths which are prohibited.
 * @param pathname Path to validate
 * @return true if valid, false if not
 */
bool filename_path_valid(const char *pathname)
{
    int path_len;

    if (!pathname) {
        return false;
    }
    if (pathname[0] == 0) {
        return false;
    }
#if BACNET_FILE_PATH_RESTRICTED
    /* check for relative directory patterns */
    if (strstr(pathname, "..") != NULL) {
        debug_printf_stderr("Relative paths are prohibited: %s\n", pathname);
        return false;
    }
    /* check for absolute paths */
    if (pathname[0] == '/') {
        debug_printf_stderr("Absolute paths are prohibited: %s\n", pathname);
        return false;
    }
    /* check for Windows drive letters (should be relative paths only) */
    path_len = strlen(pathname);
    if (path_len >= 2 && pathname[1] == ':') {
        debug_printf_stderr(
            "Windows drive letters are prohibited: %s\n", pathname);
        return false;
    }

    /* check for consecutive path separators */
    if (strstr(pathname, "//") != NULL || strstr(pathname, "\\\\") != NULL) {
        debug_printf_stderr(
            "Consecutive path separators are prohibited: %s\n", pathname);
        return false;
    }

    /* check for path components that are just dots */
    if (strstr(pathname, "/./") != NULL || strstr(pathname, "\\./") != NULL ||
        strstr(pathname, "/.\\") != NULL || strstr(pathname, "\\.\\") != NULL) {
        debug_printf_stderr(
            "Current directory references are prohibited: %s\n", pathname);
        return false;
    }
#endif

    return true;
}

