/*
 * Copyright (c) 2019 Antmicro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <zephyr/fs/fs.h>

#ifdef FILE
#undef FILE
#endif
#define FILE struct fs_file_t

#ifndef __FILE_defined
#define __FILE_defined
#endif

#define fopen(filename, mode) libc_ext_fopen(filename, mode)
#define fclose(f) libc_ext_fclose(f)
#define fread(ptr, size, nitems, f) libc_ext_fread(ptr, size, nitems, f)
#define fwrite(ptr, size, nitems, f) libc_ext_fwrite(ptr, size, nitems, f)
#define fgets(ptr, size, f) libc_ext_fgets(ptr, size, f)
#define ftell(f) fs_tell(f)
#define fseek(f, offset, whence) fs_seek(f, offset, whence)
#ifdef feof
#undef feof
#endif
#define feof(f) libc_ext_feof(f)

FILE *libc_ext_fopen(const char *filename, const char *mode);
int libc_ext_fclose(FILE *pFile);
size_t libc_ext_fwrite(
    const void *ZRESTRICT ptr,
    size_t size,
    size_t nitems,
    FILE *ZRESTRICT pFile);
size_t libc_ext_fread(
    void *ZRESTRICT ptr, size_t size, size_t nitems, FILE *ZRESTRICT pFile);
char *libc_ext_fgets(char *ZRESTRICT ptr, size_t size, FILE *ZRESTRICT pFile);
int libc_ext_feof(FILE *ZRESTRICT pFile);
