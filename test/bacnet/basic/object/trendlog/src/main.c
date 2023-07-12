/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2023
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/object/trendlog.h>
#include <bacnet/bactext.h>
#include <bacnet/basic/object/device.h>

#ifdef CONFIG_NVS
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>

static struct nvs_fs fs;
void trend_log_fs_set(struct nvs_fs *fs);

#endif

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void test_Trend_Log_ReadProperty(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    int test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata;
    /* for decode value data */
    BACNET_APPLICATION_DATA_VALUE value;
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;
    unsigned count = 0;
    bool status = false;

    Trend_Log_Init();
    count = Trend_Log_Count();
    zassert_true(count > 0, NULL);
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_TRENDLOG;
    rpdata.object_instance = Trend_Log_Index_To_Instance(0);;
    status = Trend_Log_Valid_Instance(rpdata.object_instance);
    zassert_true(status, NULL);
    Trend_Log_Property_Lists(&pRequired, &pOptional, &pProprietary);
    while ((*pRequired) != -1) {
        rpdata.object_property = *pRequired;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Trend_Log_Read_Property(&rpdata);
        if (len > 0) {
            zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
            test_len = bacapp_decode_application_data(rpdata.application_data,
                (uint8_t)rpdata.application_data_len, &value);
            if (len != test_len) {
                printf("property '%s': failed to decode!\n",
                    bactext_property_name(rpdata.object_property));
            }
            if (rpdata.object_property == PROP_PRIORITY_ARRAY) {
                /* FIXME: known fail to decode */
                len = test_len;
            }
            zassert_true(test_len >= 0, NULL);
        } else {
            printf("property '%s': failed to read!\n",
                bactext_property_name(rpdata.object_property));
        }
        pRequired++;
    }
    while ((*pOptional) != -1) {
        rpdata.object_property = *pOptional;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Trend_Log_Read_Property(&rpdata);
        zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
        if (len > 0) {
            test_len = bacapp_decode_application_data(rpdata.application_data,
                (uint8_t)rpdata.application_data_len, &value);
            if (len != test_len) {
                printf("property '%s': failed to decode!\n",
                    bactext_property_name(rpdata.object_property));
            }
            zassert_true(test_len >= 0, NULL);
        } else {
            printf("property '%s': failed to read!\n",
                bactext_property_name(rpdata.object_property));
        }
        pOptional++;
    }
}

static void Enable_log(int instance)
{
    bool status = false;
    int len;
    BACNET_APPLICATION_DATA_VALUE value;
    BACNET_WRITE_PROPERTY_DATA wp_data;
    BACNET_DATE_TIME datetime;
    const int32_t DAY = 24 * 60;

    Device_getCurrentDateTime(&datetime);

    wp_data.object_type = OBJECT_TRENDLOG;
    wp_data.object_instance = instance;
    wp_data.array_index = BACNET_ARRAY_ALL;
    wp_data.priority = BACNET_NO_PRIORITY;

    /* Set Enable=TRUE */
    wp_data.object_property = PROP_ENABLE;
    value.context_specific = false;
    value.context_tag = 0;
    value.tag = BACNET_APPLICATION_TAG_BOOLEAN;
    value.type.Boolean = true;
    wp_data.application_data_len =
        bacapp_encode_data(&wp_data.application_data[0], &value);
    zassert_true(wp_data.application_data_len > 0, NULL);
    status = Trend_Log_Write_Property(&wp_data);
    zassert_true(status, NULL);

    datetime_add_minutes(&datetime, -DAY);
    wp_data.object_property = PROP_START_TIME;
    value.context_specific = false;
    value.context_tag = 0;
    value.tag = BACNET_APPLICATION_TAG_DATE;
    value.type.Date = datetime.date;
    wp_data.application_data_len =
        bacapp_encode_data(&wp_data.application_data[0], &value);
    zassert_true(wp_data.application_data_len > 0, NULL);
    len = wp_data.application_data_len;
    value.tag = BACNET_APPLICATION_TAG_TIME;
    value.type.Time = datetime.time;
    wp_data.application_data_len =
        bacapp_encode_data(&wp_data.application_data[len], &value);
    zassert_true(wp_data.application_data_len > 0, NULL);
    wp_data.application_data_len += len;
    status = Trend_Log_Write_Property(&wp_data);
    zassert_true(status, NULL);
    
    datetime_add_minutes(&datetime, 2 * DAY);
    wp_data.object_property = PROP_STOP_TIME;
    value.context_specific = false;
    value.context_tag = 0;
    value.tag = BACNET_APPLICATION_TAG_DATE;
    value.type.Date = datetime.date;
    wp_data.application_data_len =
        bacapp_encode_data(&wp_data.application_data[0], &value);
    zassert_true(wp_data.application_data_len > 0, NULL);
    len = wp_data.application_data_len;
    value.tag = BACNET_APPLICATION_TAG_TIME;
    value.type.Time = datetime.time;
    wp_data.application_data_len =
        bacapp_encode_data(&wp_data.application_data[len], &value);
    zassert_true(wp_data.application_data_len > 0, NULL);
    wp_data.application_data_len += len;
    status = Trend_Log_Write_Property(&wp_data);
    zassert_true(status, NULL);
}

static uint32_t log_count(int instance)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = {0};

    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_TRENDLOG;
    rpdata.object_instance = instance;
    rpdata.array_index = BACNET_ARRAY_ALL;
    rpdata.object_property = PROP_RECORD_COUNT;
    len = Trend_Log_Read_Property(&rpdata);
    zassert_true(len >= 0, NULL);
    if (len >= 0) {
        test_len = bacapp_decode_known_property(rpdata.application_data,
            len, &value, rpdata.object_type, rpdata.object_property);
        if (len != test_len) {
            printf("property '%s': failed to decode!\n",
                bactext_property_name(rpdata.object_property));
        }
        zassert_equal(len, test_len, NULL);
    }
    return value.type.Unsigned_Int;
}

static void testLogs(void)
{
    const uint32_t instance = 1;
    BACNET_LOG_STATUS eStatus;
    int len = 0;
    bool b;
    uint8_t apdu1[MAX_APDU] = { 0 };
    uint8_t apdu2[MAX_APDU] = { 0 };

    Trend_Log_Init();
    len = Trend_Log_Count();
    zassert_true(len > 0, NULL);

    zassert_false(TL_Is_Enabled(instance), NULL);
    Enable_log(instance);
    zassert_true(TL_Is_Enabled(instance), NULL);

    zassert_true(log_count(instance) > 100, 0, NULL);

    TL_encode_entry(apdu1, instance, 1);
    TL_Insert_Status_Rec(instance,LOG_STATUS_LOG_INTERRUPTED, false);
    TL_encode_entry(apdu2, instance, 1);
    zassert_not_equal(memcmp(apdu1, apdu2, sizeof(apdu1)), 0, NULL);
}
/**
 * @}
 */

#ifdef CONFIG_NVS

#define NVS_PARTITION       storage_partition
#define NVS_PARTITION_DEVICE    FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET    FIXED_PARTITION_OFFSET(NVS_PARTITION)

void init_trend_log_fs(struct nvs_fs *fs)
{
    int rc = 0, cnt = 0, cnt_his = 0;
    char buf[16];
    uint8_t key[8], longarray[128];
    uint32_t reboot_counter = 0U, reboot_counter_his;
    struct flash_pages_info info;

    fs->flash_device = NVS_PARTITION_DEVICE;
    if (!device_is_ready(fs->flash_device)) {
        printf("Flash device %s is not ready\n", fs.flash_device->name);
        return;
    }
    fs->offset = NVS_PARTITION_OFFSET;
    rc = flash_get_page_info_by_offs(fs->flash_device, fs->offset, &info);
    if (rc) {
        printf("Unable to get page info\n");
        return;
    }
    fs->sector_size = info.size;
    fs->sector_count = 3U;

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
    init_trend_log_fs(&trend_log_fs);
    trend_log_fs_set(&trend_log_fs);
#endif

    ztest_test_suite(trendlog_tests,
        ztest_unit_test(test_Trend_Log_ReadProperty),
        ztest_unit_test(testLogs)
    );

    ztest_run_test_suite(trendlog_tests);
}
