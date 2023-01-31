/*
 * Copyright (c) 2022 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @author Mikhail Antropov
 * @brief Example server application using the BACnet Stack with Secure connect.
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/fs/fs.h>
#include <zephyr/fs/littlefs.h>
#include <zephyr/storage/flash_map.h>

LOG_MODULE_DECLARE(bacnet, LOG_LEVEL_DBG);

#include <stdlib.h>
#include "bacnet/config.h"
#include "bacnet/apdu.h"
#include "bacnet/dcc.h"
#include "bacnet/iam.h"
#include "bacnet/version.h"
#include "bacnet/basic/services.h"
#include "bacnet/datalink/dlenv.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/bacfile.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/lc.h"
#include "bacnet/basic/object/trendlog.h"

#include "bacnet/basic/object/netport.h"
#include "bacnet/basic/object/sc_netport.h"
#include "bacnet/datalink/bsc/bsc-event.h"

#define MNTP  "/lfs"
FS_LITTLEFS_DECLARE_DEFAULT_CONFIG(storage);
static struct fs_mount_t mnt = {
    .type = FS_LITTLEFS,
    .fs_data = &storage,
    .storage_dev = (void *)FIXED_PARTITION_ID(storage_partition),
    .mnt_point = MNTP,
};

static const uint8_t Ca_Certificate[] = {
    #include "ca_cert.pem.inc"
};
static const uint8_t Certificate[] = {
    #include "server_cert.pem.inc"
};
static const uint8_t Key[] = {
    #include "server_key.pem.inc"
};

#define SC_HUB_FUNCTION_BINDING "50000"
#define SC_DIRECT_CONNECT_INITIATE "n"

#define DEVICE_INSTANCE 111
#define DEVICE_NAME "NoFred"
#define FILENAME_CA_CERT    MNTP"/ca_cert.pem"
#define FILENAME_CERT       MNTP"/server_cert.pem"
#define FILENAME_KEY        MNTP"/server_key.pem"


/* current version of the BACnet stack */
static const char *BACnet_Version = BACNET_VERSION_TEXT;

/** Buffer used for receiving */
//static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/** Initialize the handlers we will utilize.
 * @see Device_Init, apdu_set_unconfirmed_handler, apdu_set_confirmed_handler
 */
static void Init_Service_Handlers(void)
{
    Device_Init(NULL);
   /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
}

static int littlefs_flash_erase(unsigned int id)
{
    const struct flash_area *pfa;
    int rc;

    rc = flash_area_open(id, &pfa);
    if (rc < 0) {
        LOG_ERR("FAIL: unable to find flash area %u: %d\n",
            id, rc);
        return rc;
    }

    LOG_INF("Area %u at 0x%x on %s for %u bytes\n",
           id, (unsigned int)pfa->fa_off, pfa->fa_dev->name,
           (unsigned int)pfa->fa_size);

    /* Optional wipe flash contents */
    if (IS_ENABLED(CONFIG_APP_WIPE_STORAGE)) {
        rc = flash_area_erase(pfa, 0, pfa->fa_size);
        LOG_ERR("Erasing flash area ... %d", rc);
    }

    flash_area_close(pfa);
    return rc;
}

static int test_statvfs(char *str)
{
    struct fs_statvfs stat;
    int res;

    /* Verify fs_statvfs() */
    res = fs_statvfs(MNTP, &stat);
    if (res) {
        LOG_INF("Error getting volume stats [%d]", res);
        return res;
    }

    LOG_INF("%s", str);
    LOG_INF("Optimal transfer block size   = %lu", stat.f_bsize);
    LOG_INF("Allocation unit size          = %lu", stat.f_frsize);
    LOG_INF("Volume size in f_frsize units = %lu", stat.f_blocks);
    LOG_INF("Free space in f_frsize units  = %lu", stat.f_bfree);

    return 0;
}

static bool file_save(const char *name, const uint8_t *buffer, uint32_t buffer_size)
{
    struct fs_file_t file = {0};
    ssize_t brw;
    int status;

    status = fs_open(&file, name, FS_O_CREATE | FS_O_WRITE);
    if (status < 0) {
        LOG_INF("Failed opening file: %s, flag %d, errno=%d", name,
            FS_O_CREATE | FS_O_WRITE, status);
        test_statvfs("error open");
        return false;
    }

    brw = fs_write(&file, buffer, buffer_size);
    if (brw < 0) {
        LOG_INF("Failed writing to file: %s [%d]", name, (int)brw);
        test_statvfs("error write");
    }

    fs_close(&file);
    return brw >= 0;
}

static bool init_bsc(void)
{
    int rc;

    rc = littlefs_flash_erase((uintptr_t)mnt.storage_dev);
    if (rc < 0) {
        return rc;
    }

    rc = fs_mount(&mnt);
    if (rc != 0) {
        LOG_INF("Error mounting fs [%d]", rc);
        return false;
    }

    if (!file_save(FILENAME_CA_CERT, Ca_Certificate, sizeof(Ca_Certificate)) ||
        !file_save(FILENAME_CERT, Certificate, sizeof(Certificate)) ||
        !file_save(FILENAME_KEY, Key, sizeof(Key))) {
        return false;
    }

    setenv("BACNET_SC_ISSUER_1_CERTIFICATE_FILE", FILENAME_CA_CERT, 1);
    setenv("BACNET_SC_OPERATIONAL_CERTIFICATE_FILE", FILENAME_CERT, 1);
    setenv("BACNET_SC_OPERATIONAL_CERTIFICATE_PRIVATE_KEY_FILE", FILENAME_KEY,
        1);
    setenv("BACNET_SC_HUB_FUNCTION_BINDING", SC_HUB_FUNCTION_BINDING, 1);
    setenv("BACNET_SC_DIRECT_CONNECT_INITIATE", SC_DIRECT_CONNECT_INITIATE, 1);

    return true;
}

void main(void)
{
//    unsigned timeout = 1; /* milliseconds */

    /* allow the device ID to be set */
    Device_Set_Object_Instance_Number(DEVICE_INSTANCE);

    LOG_INF("BACnet SC Server Demo\n"
           "BACnet Stack Version %s\n"
           "BACnet Device ID: %u\n"
           "Max APDU: %d\n",
        BACnet_Version, Device_Object_Instance_Number(), MAX_APDU);
    /* load any static address bindings to show up
       in our device bindings list */
    address_init();
    Init_Service_Handlers();

    Device_Object_Name_ANSI_Init(DEVICE_NAME);
    LOG_INF("BACnet Device Name: %s\n", DEVICE_NAME);

    bacfile_init();
    init_bsc();
    dlenv_init();
    LOG_INF("Run BACNet/SC hub\n");
    atexit(datalink_cleanup);

    /* loop forever */
    for (;;) {
        bsc_wait(1);
        datalink_maintenance_timer(1);
        /* blink LEDs, Turn on or off outputs, etc */
    }
}
