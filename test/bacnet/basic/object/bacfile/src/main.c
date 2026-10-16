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
    const int32_t *required_property = NULL;
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
/* simulated record file with more records than the response buffer holds */
static size_t Record_File_Total_Records;

static bool test_record_read(
    const char *pathname,
    int32_t fileStartRecord,
    size_t record_index,
    uint8_t *buffer,
    size_t buffer_size)
{
    size_t record = (size_t)fileStartRecord + record_index;

    (void)pathname;
    if (record >= Record_File_Total_Records) {
        return false;
    }
    snprintf((char *)buffer, buffer_size, "record-%u", (unsigned)record);

    return true;
}

/**
 * @brief Test reading records starting beyond the response buffer capacity
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacfile_tests, test_BACnet_File_Read_Record_Data_late_start)
#else
static void test_BACnet_File_Read_Record_Data_late_start(void)
#endif
{
    BACNET_ATOMIC_READ_FILE_DATA data = { 0 };
    const size_t capacity = ARRAY_SIZE(data.fileData);
    uint32_t object_instance;
    bool status;
    size_t i;
    char expected[32];

    Record_File_Total_Records = capacity * 3;
    bacfile_init();
    bacfile_read_record_data_callback_set(test_record_read);
    object_instance = bacfile_create(1);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);
    bacfile_pathname_set(object_instance, "record-file.txt");

    /* start at a record that is beyond the buffer capacity */
    data.object_instance = object_instance;
    data.access = FILE_RECORD_ACCESS;
    data.type.record.fileStartRecord = (int32_t)(capacity + 1);
    data.type.record.RecordCount = capacity;
    status = bacfile_read_record_data(&data);
    zassert_true(status, NULL);
    zassert_false(data.endOfFile, NULL);
    zassert_equal(data.type.record.RecordCount, capacity, NULL);
    for (i = 0; i < capacity; i++) {
        snprintf(
            expected, sizeof(expected), "record-%u",
            (unsigned)(capacity + 1 + i));
        zassert_equal(
            octetstring_length(&data.fileData[i]), strlen(expected), NULL);
        zassert_equal(
            memcmp(
                octetstring_value(&data.fileData[i]), expected,
                strlen(expected)),
            0, NULL);
    }

    /* requested count is limited to the buffer capacity */
    data.type.record.fileStartRecord = (int32_t)(capacity * 2);
    data.type.record.RecordCount = capacity * 10;
    status = bacfile_read_record_data(&data);
    zassert_true(status, NULL);
    zassert_false(data.endOfFile, NULL);
    zassert_equal(data.type.record.RecordCount, capacity, NULL);

    /* start past the end of the file */
    data.type.record.fileStartRecord = (int32_t)(capacity * 3);
    data.type.record.RecordCount = capacity;
    status = bacfile_read_record_data(&data);
    zassert_true(status, NULL);
    zassert_true(data.endOfFile, NULL);
    zassert_equal(data.type.record.RecordCount, 0, NULL);

    /* negative start is invalid */
    data.type.record.fileStartRecord = -1;
    data.type.record.RecordCount = 1;
    status = bacfile_read_record_data(&data);
    zassert_true(status, NULL);
    zassert_true(data.endOfFile, NULL);
    zassert_equal(data.type.record.RecordCount, 0, NULL);

    bacfile_read_record_data_callback_set(NULL);
    bacfile_cleanup();
}
static size_t test_record_count(const char *pathname)
{
    (void)pathname;

    return Record_File_Total_Records;
}

/**
 * @brief Test the End Of File flag with a known number of records
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacfile_tests, test_BACnet_File_Read_Record_Data_end_of_file)
#else
static void test_BACnet_File_Read_Record_Data_end_of_file(void)
#endif
{
    BACNET_ATOMIC_READ_FILE_DATA data = { 0 };
    const size_t capacity = ARRAY_SIZE(data.fileData);
    uint32_t object_instance;
    size_t count = 0;

    Record_File_Total_Records = capacity * 2;
    bacfile_init();
    bacfile_read_record_data_callback_set(test_record_read);
    bacfile_record_count_callback_set(test_record_count);
    object_instance = bacfile_create(1);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);
    bacfile_pathname_set(object_instance, "record-file.txt");
    zassert_true(bacfile_record_count(object_instance, &count), NULL);
    zassert_equal(count, capacity * 2, NULL);
    zassert_false(bacfile_record_count(object_instance + 1, &count), NULL);

    data.object_instance = object_instance;
    data.access = FILE_RECORD_ACCESS;
    /* response does not include the last record */
    data.type.record.fileStartRecord = 0;
    data.type.record.RecordCount = capacity;
    zassert_true(bacfile_read_record_data(&data), NULL);
    zassert_equal(data.type.record.RecordCount, capacity, NULL);
    zassert_false(data.endOfFile, NULL);
    /* response ends exactly on the last record */
    data.type.record.fileStartRecord = (int32_t)capacity;
    data.type.record.RecordCount = capacity;
    zassert_true(bacfile_read_record_data(&data), NULL);
    zassert_equal(data.type.record.RecordCount, capacity, NULL);
    zassert_true(data.endOfFile, NULL);
    /* start record equal to the record count has no records */
    data.type.record.fileStartRecord = (int32_t)(capacity * 2);
    data.type.record.RecordCount = capacity;
    zassert_true(bacfile_read_record_data(&data), NULL);
    zassert_equal(data.type.record.RecordCount, 0, NULL);
    zassert_true(data.endOfFile, NULL);
    /* empty file */
    Record_File_Total_Records = 0;
    data.type.record.fileStartRecord = 0;
    data.type.record.RecordCount = capacity;
    zassert_true(bacfile_read_record_data(&data), NULL);
    zassert_equal(data.type.record.RecordCount, 0, NULL);
    zassert_true(data.endOfFile, NULL);

    bacfile_record_count_callback_set(NULL);
    bacfile_read_record_data_callback_set(NULL);
    bacfile_cleanup();
}

/* write callback that fails on the record index in Record_Write_Fail_Index */
static size_t Record_Write_Fail_Index;
static size_t Record_Write_Calls;

static bool test_record_write(
    const char *pathname,
    int32_t fileStartRecord,
    size_t record_index,
    const uint8_t *buffer,
    size_t buffer_size)
{
    (void)pathname;
    (void)fileStartRecord;
    (void)buffer;
    (void)buffer_size;
    Record_Write_Calls++;

    return record_index != Record_Write_Fail_Index;
}

/**
 * @brief Test that a failed record write is reported as a failure
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bacfile_tests, test_BACnet_File_Write_Record_Data_failure)
#else
static void test_BACnet_File_Write_Record_Data_failure(void)
#endif
{
    BACNET_ATOMIC_WRITE_FILE_DATA data = { 0 };
    uint32_t object_instance;
    uint8_t value[] = { 'a' };
    size_t i;

    bacfile_init();
    bacfile_write_record_data_callback_set(test_record_write);
    object_instance = bacfile_create(1);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);
    bacfile_pathname_set(object_instance, "record-file.txt");
    zassert_equal(ARRAY_SIZE(data.fileData), 3, NULL);

    data.object_instance = object_instance;
    data.access = FILE_RECORD_ACCESS;
    data.type.record.fileStartRecord = 0;
    data.type.record.returnedRecordCount = 3;
    for (i = 0; i < 3; i++) {
        octetstring_init(&data.fileData[i], value, sizeof(value));
    }
    /* all records written */
    Record_Write_Fail_Index = 99;
    Record_Write_Calls = 0;
    zassert_true(bacfile_write_record_data(&data), NULL);
    zassert_equal(Record_Write_Calls, 3, NULL);
    /* second record fails: stop and report the failure */
    Record_Write_Fail_Index = 1;
    Record_Write_Calls = 0;
    zassert_false(bacfile_write_record_data(&data), NULL);
    zassert_equal(Record_Write_Calls, 2, NULL);
    /* more records than the buffer holds is not silently shortened */
    data.type.record.returnedRecordCount = 4;
    Record_Write_Fail_Index = 99;
    Record_Write_Calls = 0;
    zassert_false(bacfile_write_record_data(&data), NULL);
    zassert_equal(Record_Write_Calls, 0, NULL);

    bacfile_write_record_data_callback_set(NULL);
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
        ztest_unit_test(test_BACnet_File_Read_Record_Data_late_start),
        ztest_unit_test(test_BACnet_File_Read_Record_Data_end_of_file),
        ztest_unit_test(test_BACnet_File_Write_Record_Data_failure));

    ztest_run_test_suite(bacfile_tests);
}
#endif
