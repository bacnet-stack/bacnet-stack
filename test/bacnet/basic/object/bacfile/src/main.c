/**
 * @file
 * @brief Test for BACnet File object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date January 2023
 * @section LICENSE
 *
 * Copyright (C) 2022 Steve Karg <skarg@users.sourceforge.net>
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <string.h>
#include <zephyr/ztest.h>
#include <bacnet/basic/object/bacfile.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacfile_tests, test_BACnet_File_Object)
#else
static void test_BACnet_File_Object(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    const int *required_property = NULL;
    const uint32_t instance = 1;

    bacfile_init();
    bacfile_create(1);
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_FILE;
    rpdata.object_instance = instance;
    rpdata.array_index = BACNET_ARRAY_ALL;

    BACfile_Property_Lists(&required_property, NULL, NULL);
    while ((*required_property) >= 0) {
        rpdata.object_property = *required_property;
        len = bacfile_read_property(&rpdata);
        if (len < 0) {
            printf(
                "property %u: failed to read!\n",
                (unsigned)rpdata.object_property);
        }
        zassert_true(len >= 0, NULL);
        if (len >= 0) {
            test_len = bacapp_decode_known_property(
                rpdata.application_data, len, &value, rpdata.object_type,
                rpdata.object_property);
            if (len != test_len) {
                printf(
                    "property %u: failed to decode!\n",
                    (unsigned)rpdata.object_property);
            }
            zassert_equal(len, test_len, NULL);
        }
        required_property++;
    }

    bacfile_cleanup();

    return;
}

static const char *Test_Records[] = { "first", "", "third" };
static bool Test_Write_Result = true;
static int32_t Test_Write_Start = -2;

static bool test_record_read(
    const char *pathname,
    int32_t start,
    size_t index,
    uint8_t *buffer,
    size_t size)
{
    size_t record = (size_t)start + index;

    (void)pathname;
    if (record >= ARRAY_SIZE(Test_Records)) {
        return false;
    }
    strncpy((char *)buffer, Test_Records[record], size);

    return true;
}

static bool test_record_write(
    const char *pathname,
    int32_t start,
    size_t index,
    const uint8_t *buffer,
    size_t size)
{
    (void)pathname;
    (void)index;
    (void)buffer;
    (void)size;
    Test_Write_Start = start;

    return Test_Write_Result;
}

static size_t test_record_count(const char *pathname)
{
    (void)pathname;

    return ARRAY_SIZE(Test_Records);
}

/**
 * @brief Test AtomicReadFile and AtomicWriteFile record access
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacfile_tests, test_BACnet_File_Record_Access)
#else
static void test_BACnet_File_Record_Access(void)
#endif
{
    BACNET_ATOMIC_READ_FILE_DATA rdata = { 0 };
    BACNET_ATOMIC_WRITE_FILE_DATA wdata = { 0 };
    const uint32_t instance = 1;
    size_t count = 0;
    uint8_t value[] = { 'a' };

    bacfile_init();
    bacfile_create(instance);
    bacfile_pathname_set(instance, "records.txt");
    bacfile_read_record_data_callback_set(test_record_read);
    bacfile_write_record_data_callback_set(test_record_write);

    /* no record count callback */
    zassert_false(bacfile_record_count(instance, &count), NULL);
    bacfile_record_count_callback_set(test_record_count);
    zassert_false(bacfile_record_count(instance + 1, &count), NULL);
    zassert_true(bacfile_record_count(instance, &count), NULL);
    zassert_equal(count, ARRAY_SIZE(Test_Records), NULL);

    /* the response holds one record, which is not the last record */
    rdata.object_instance = instance;
    rdata.type.record.fileStartRecord = 0;
    rdata.type.record.RecordCount = ARRAY_SIZE(Test_Records) + 1;
    zassert_true(bacfile_read_record_data(&rdata), NULL);
    zassert_equal(rdata.type.record.RecordCount, ARRAY_SIZE(rdata.fileData), 0);
    zassert_equal(octetstring_length(&rdata.fileData[0]), 5, NULL);
    zassert_false(rdata.endOfFile, NULL);
    /* the start record can be beyond the response buffer size */
    rdata.type.record.fileStartRecord = 2;
    rdata.type.record.RecordCount = 1;
    zassert_true(bacfile_read_record_data(&rdata), NULL);
    zassert_equal(rdata.type.record.RecordCount, 1, NULL);
    zassert_equal(octetstring_length(&rdata.fileData[0]), 5, NULL);
    zassert_true(rdata.endOfFile, "response has the last record");
    /* an empty record is a record */
    rdata.type.record.fileStartRecord = 1;
    rdata.type.record.RecordCount = 1;
    zassert_true(bacfile_read_record_data(&rdata), NULL);
    zassert_equal(rdata.type.record.RecordCount, 1, NULL);
    zassert_equal(octetstring_length(&rdata.fileData[0]), 0, NULL);
    zassert_false(rdata.endOfFile, NULL);
    /* start at the end of the file returns no records */
    rdata.type.record.fileStartRecord = ARRAY_SIZE(Test_Records);
    rdata.type.record.RecordCount = 1;
    zassert_true(bacfile_read_record_data(&rdata), NULL);
    zassert_equal(rdata.type.record.RecordCount, 0, NULL);
    zassert_true(rdata.endOfFile, NULL);

    /* write failures are reported */
    wdata.object_instance = instance;
    wdata.type.record.fileStartRecord = -1;
    wdata.type.record.returnedRecordCount = 1;
    octetstring_init(&wdata.fileData[0], value, sizeof(value));
    Test_Write_Result = true;
    zassert_true(bacfile_write_record_data(&wdata), NULL);
    zassert_equal(Test_Write_Start, -1, NULL);
    Test_Write_Result = false;
    zassert_false(bacfile_write_record_data(&wdata), NULL);
    Test_Write_Result = true;
    /* a record count over the request buffer size is an error */
    wdata.type.record.returnedRecordCount = ARRAY_SIZE(wdata.fileData) + 1;
    zassert_false(bacfile_write_record_data(&wdata), NULL);

    bacfile_cleanup();
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bacfile_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        bacfile_tests, ztest_unit_test(test_BACnet_File_Object),
        ztest_unit_test(test_BACnet_File_Record_Access));

    ztest_run_test_suite(bacfile_tests);
}
#endif
