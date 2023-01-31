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
#if defined(INTRINSIC_REPORTING)
#include "bacnet/basic/object/nc.h"
#endif /* defined(INTRINSIC_REPORTING) */

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
    #include "client_cert.pem.inc"
};
static const uint8_t Key[] = {
    #include "client_key.der.inc"
};

#define SERVER_URL "wss://192.0.2.2:50000"

#define DEVICE_INSTANCE 123
#define DEVICE_NAME "Fred"
#define FILENAME_CA_CERT    MNTP"/ca_cert.pem"
#define FILENAME_CERT       MNTP"/client_cert.pem"
#define FILENAME_KEY        MNTP"/client_key.pem"

/* current version of the BACnet stack */
static const char *BACnet_Version = BACNET_VERSION_TEXT;

/** Buffer used for receiving */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/** Initialize the handlers we will utilize.
 * @see Device_Init, apdu_set_unconfirmed_handler, apdu_set_confirmed_handler
 */
static void Init_Service_Handlers(void)
{
    Device_Init(NULL);
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);

#if 0
    /*  BACnet Testing Observed Incident oi00107
        Server only devices should not indicate that they EXECUTE I-Am
        Revealed by BACnet Test Client v1.8.16 ( www.bac-test.com/bacnet-test-client-download )
            BITS: BIT00040
        Any discussions can be directed to edward@bac-test.com
        Please feel free to remove this comment when my changes accepted after suitable time for
        review by all interested parties. Say 6 months -> September 2016 */
    /* In this demo, we are the server only ( BACnet "B" device ) so we do not indicate
       that we can execute the I-Am message */
    /* handle i-am to support binding to other devices */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_bind);
#endif

    /* set the handler for all the services we don't implement */
    /* It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* Set the handlers for any confirmed services that we support. */
    /* We must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, handler_write_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE, handler_write_property_multiple);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_RANGE, handler_read_range);
#if defined(BACFILE)
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_ATOMIC_READ_FILE, handler_atomic_read_file);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_ATOMIC_WRITE_FILE, handler_atomic_write_file);
#endif
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_REINITIALIZE_DEVICE, handler_reinitialize_device);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION, handler_timesync_utc);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION, handler_timesync);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_SUBSCRIBE_COV, handler_cov_subscribe);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_COV_NOTIFICATION, handler_ucov_notification);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
    /* handle the data coming back from private requests */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_PRIVATE_TRANSFER,
        handler_unconfirmed_private_transfer);
#if defined(INTRINSIC_REPORTING)
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_ACKNOWLEDGE_ALARM, handler_alarm_ack);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_GET_EVENT_INFORMATION, handler_get_event_information);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_GET_ALARM_SUMMARY, handler_get_alarm_summary);
#endif /* defined(INTRINSIC_REPORTING) */
#if defined(BACNET_TIME_MASTER)
    handler_timesync_init();
#endif
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

static bool file_save(const char *name, const uint8_t *buffer,
    uint32_t buffer_size)
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

    setenv("BACNET_SC_PRIMARY_HUB_URI", SERVER_URL, 1);
    setenv("BACNET_SC_FAILOVER_HUB_URI", SERVER_URL, 1);
    setenv("BACNET_SC_ISSUER_1_CERTIFICATE_FILE", FILENAME_CA_CERT, 1);
    setenv("BACNET_SC_OPERATIONAL_CERTIFICATE_FILE", FILENAME_CERT, 1);
    setenv("BACNET_SC_OPERATIONAL_CERTIFICATE_PRIVATE_KEY_FILE", FILENAME_KEY,
        1);

    return true;
}

void main(void)
{
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 1; /* milliseconds */
    uint64_t last_msec = 0;
    uint64_t current_msec = 0;
    uint64_t elapsed_msec = 0;
    uint64_t elapsed_sec = 0;
    uint32_t address_binding_tmr = 0;
#if defined(INTRINSIC_REPORTING)
    uint32_t recipient_scan_tmr = 0;
#endif
#if defined(BACNET_TIME_MASTER)
    BACNET_DATE_TIME bdatetime;
#endif

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
    LOG_INF("Connection to BACNet/SC hub established\n");
    atexit(datalink_cleanup);

    /* configure the timeout values */
    last_msec = k_uptime_get();
    /* broadcast an I-Am on startup */
    Send_I_Am(&Handler_Transmit_Buffer[0]);
    /* loop forever */
    for (;;) {
        /* input */
        current_msec = k_uptime_get();

        /* returns 0 bytes on timeout */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);

        /* process */
        if (pdu_len) {
            //printf("received pdu of size %d\n\n", pdu_len );
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
        /* at least one second has passed */
        elapsed_msec = (current_msec - last_msec);
        if (elapsed_msec > MSEC_PER_SEC) {
            last_msec = current_msec;
            elapsed_sec = elapsed_msec / MSEC_PER_SEC;
            dcc_timer_seconds(elapsed_sec);
            datalink_maintenance_timer(elapsed_sec);
            dlenv_maintenance_timer(elapsed_sec);
            Load_Control_State_Machine_Handler();
            handler_cov_timer_seconds(elapsed_sec);
            tsm_timer_milliseconds(elapsed_msec);
            trend_log_timer(elapsed_sec);
#if defined(INTRINSIC_REPORTING)
            Device_local_reporting();
#endif
#if defined(BACNET_TIME_MASTER)
            Device_getCurrentDateTime(&bdatetime);
            handler_timesync_task(&bdatetime);
#endif
        }
        handler_cov_task();
        /* scan cache address */
        address_binding_tmr += elapsed_sec;
        if (address_binding_tmr >= 60) {
            address_cache_timer(address_binding_tmr);
            address_binding_tmr = 0;
        }
#if defined(INTRINSIC_REPORTING)
        /* try to find addresses of recipients */
        recipient_scan_tmr += elapsed_sec;
        if (recipient_scan_tmr >= NC_RESCAN_RECIPIENTS_SECS) {
            Notification_Class_find_recipient();
            recipient_scan_tmr = 0;
        }
#endif
        /* output */

        /* blink LEDs, Turn on or off outputs, etc */
    }
}
