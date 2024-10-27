/*
 * Copyright (c) 2019 Antmicro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(lib_extensions, CONFIG_BACNETSTACK_LOG_LEVEL);

#include <zephyr/kernel.h>
#include "libc_extensions.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define FN_MISSING() LOG_DBG("[IMPLEMENTATION MISSING : %s]\n", __func__)

static FILE fds[CONFIG_POSIX_MAX_FDS] = { 0 };

FILE *libc_ext_fopen(const char *filename, const char *mode)
{
    int index;
    for (index = 0; index < CONFIG_POSIX_MAX_FDS; ++index) {
        if (fds[index].mp == NULL) {
            break;
        }
    }
    if (index == CONFIG_POSIX_MAX_FDS) {
        return NULL;
    }
    fs_mode_t flags = 0;
    if (mode != NULL) {
        for (const char *pc = mode; *pc != 0; pc++) {
            if (*pc == 'r') {
                flags |= FS_O_READ;
            }
            if (*pc == 'w') {
                flags |= FS_O_WRITE;
            }
            if (*pc == 'a') {
                flags |= FS_O_APPEND;
            }
        }
        if (flags & FS_O_WRITE) {
            flags |= FS_O_CREATE;
        }
    }

    int ret = fs_open(&fds[index], filename, flags);
    if (ret != 0) {
        fs_file_t_init(&fds[index]);
        return NULL;
    }
    return &fds[index];
}

int libc_ext_fclose(FILE *pFile)
{
    if ((pFile < &fds[0]) || (pFile >= &fds[CONFIG_POSIX_MAX_FDS])) {
        return -EINVAL;
    }
    int ret = fs_close(pFile);
    fs_file_t_init(pFile);
    return ret;
}

size_t libc_ext_fwrite(
    const void *ZRESTRICT ptr,
    size_t size,
    size_t nitems,
    FILE *ZRESTRICT pFile)
{
    ssize_t ret = fs_write(pFile, ptr, size * nitems);
    return ret >= 0 ? ret / size : ret;
}

size_t libc_ext_fread(
    void *ZRESTRICT ptr, size_t size, size_t nitems, FILE *ZRESTRICT pFile)
{
    ssize_t ret = fs_read(pFile, ptr, size * nitems);
    return ret >= 0 ? ret / size : ret;
}

char *libc_ext_fgets(char *ZRESTRICT ptr, size_t size, FILE *ZRESTRICT pFile)
{
    ssize_t ret = fs_read(pFile, ptr, size);
    return ret >= 0 ? ptr : NULL;
}

int libc_ext_feof(FILE *ZRESTRICT pFile)
{
    long size;
    long origin;

    origin = ftell(pFile);
    fseek(pFile, 0L, SEEK_END);
    size = ftell(pFile);
    fseek(pFile, origin, SEEK_SET);

    return origin < size;
}
