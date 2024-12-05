/*
 * Copyright (c) 2023 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet auditlog APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/object/auditlog.h>
#include <bacnet/bactext.h>
#include <bacnet/basic/object/device.h>

#ifdef CONFIG_NVS
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>

static struct nvs_fs audit_log_fs;
void audit_log_fs_set(struct nvs_fs *fs);

#endif

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test Auditlog handling

 */
static void testAuditlog(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    const int *required_property = NULL;
    const uint32_t instance = 1;

    Audit_Log_Init();
    len = Audit_Log_Count();
    zassert_true(len > 0, NULL);

    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_AUDIT_LOG;
    rpdata.object_instance = instance;
    rpdata.array_index = BACNET_ARRAY_ALL;

    Audit_Log_Property_Lists(&required_property, NULL, NULL);
    while ((*required_property) >= 0) {
        rpdata.object_property = *required_property;
        len = Audit_Log_Read_Property(&rpdata);
        if (PROP_LOG_BUFFER != rpdata.object_property) {
            zassert_true(len >= 0, NULL);
        }
        if (len >= 0) {
            test_len = bacapp_decode_known_property(
                rpdata.application_data, len, &value, rpdata.object_type,
                rpdata.object_property);
            if (len != test_len) {
                printf(
                    "property '%s': failed to decode!\n",
                    bactext_property_name(rpdata.object_property));
            }
            zassert_equal(len, test_len, NULL);
        }
        required_property++;
    }
}

static void testLogs(void)
{
    const uint32_t instance = 1;
    int len = 0;
    uint8_t apdu1[MAX_APDU] = { 0 };
    uint8_t apdu2[MAX_APDU] = { 0 };

    Audit_Log_Init();
    len = Audit_Log_Count();
    zassert_true(len > 0, NULL);

    AL_encode_entry(apdu1, instance, 1);
    Audit_Log_Insert_Status_Rec(instance, LOG_STATUS_LOG_INTERRUPTED, false);
    AL_encode_entry(apdu2, instance, 1);
    zassert_not_equal(memcmp(apdu1, apdu2, sizeof(apdu1)), 0, NULL);
}
/**
 * @}
 */

#ifdef CONFIG_NVS

#define NVS_PARTITION storage_partition
#define NVS_PARTITION_DEVICE FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET FIXED_PARTITION_OFFSET(NVS_PARTITION)

void init_audit_log_fs(struct nvs_fs *fs)
{
    int rc = 0;
    struct flash_pages_info info;

    fs->flash_device = NVS_PARTITION_DEVICE;
    if (!device_is_ready(fs->flash_device)) {
        printf("Flash device %s is not ready\n", fs->flash_device->name);
        return;
    }
    fs->offset = NVS_PARTITION_OFFSET;
    rc = flash_get_page_info_by_offs(fs->flash_device, fs->offset, &info);
    if (rc) {
        printf("Unable to get page info\n");
        return;
    }
    fs->sector_size = info.size;
    fs->sector_count = 256U;

    rc = nvs_mount(fs);
    if (rc) {
        printf("Flash Init failed\n");
        return;
    }
}
#endif

void test_main(void)
{
#ifdef CONFIG_NVS
    init_audit_log_fs(&audit_log_fs);
    audit_log_fs_set(&audit_log_fs);
#endif

    ztest_test_suite(
        auditlog_tests, ztest_unit_test(testAuditlog),
        ztest_unit_test(testLogs));

    ztest_run_test_suite(auditlog_tests);
}
