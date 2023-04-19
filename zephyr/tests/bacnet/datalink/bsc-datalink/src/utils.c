/*
 * Copyright (c) 2023 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @author Mikhail Antropov
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>
#include "bacnet/basic/sys/debug.h"

#define MNTP  "/lfs"
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
static struct fs_mount_t mnt = {
    .type = FS_LITTLEFS,
    .fs_data = &storage,
    .storage_dev = (void *)FIXED_PARTITION_ID(storage_partition),
    .mnt_point = MNTP,
};

static int littlefs_flash_erase(unsigned int id)
{
    const struct flash_area *pfa;
    int rc;

    rc = flash_area_open(id, &pfa);
    if (rc < 0) {
        debug_printf("FAIL: unable to find flash area %u: %d\n",
            id, rc);
        return rc;
    }

    debug_printf("Area %u at 0x%x on %s for %u bytes\n",
           id, (unsigned int)pfa->fa_off, pfa->fa_dev->name,
           (unsigned int)pfa->fa_size);

    /* Optional wipe flash contents */
    if (IS_ENABLED(CONFIG_APP_WIPE_STORAGE)) {
        rc = flash_area_erase(pfa, 0, pfa->fa_size);
        debug_printf("Erasing flash area ... %d", rc);
    }

    flash_area_close(pfa);
    return rc;
}

bool init_zephyr_env(void)
{
    int rc;

    rc = littlefs_flash_erase((uintptr_t)mnt.storage_dev);
    if (rc < 0) {
        return false;
    }

    rc = fs_mount(&mnt);
    if (rc != 0) {
        debug_printf("Error mounting fs [%d]", rc);
        return false;
    }

    return true;
}
