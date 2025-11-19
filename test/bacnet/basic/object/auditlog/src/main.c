/**
 * @file
 * @brief test BACnet AuditLog object APIs
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @author Mikhail Antropov <michail.antropov@dsr-corporation.com>
 * @date 2023
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/auditlog.h>
#include <bacnet/bactext.h>
#include <bacnet/basic/object/device.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test Auditlog handling

 */
static void testAuditlog(void)
{
    int len = 0, index = 0;
    const uint32_t instance = 1;
    uint32_t test_instance = 0;
    const int32_t skip_fail_property_list[] = { -1 };
    bool status = false;
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
    uint32_t buffer_size = 0;

    Audit_Log_Init();
    len = Audit_Log_Count();
    zassert_true(len == 0, NULL);
    status = Audit_Log_Valid_Instance(instance);
    zassert_false(status, NULL);
    test_instance = Audit_Log_Create(instance);
    zassert_true(test_instance == instance, NULL);
    len = Audit_Log_Count();
    zassert_true(len == 1, NULL);
    test_instance = Audit_Log_Index_To_Instance(0);
    zassert_true(test_instance == instance, NULL);
    index = Audit_Log_Instance_To_Index(instance);
    zassert_true(index == 0, NULL);
    status = Audit_Log_Valid_Instance(instance);
    zassert_true(status, NULL);

    /* configure ReadProperty test */
    /* perform a general test for RP/WP */
    bacnet_object_properties_read_write_test(
        OBJECT_AUDIT_LOG, instance, Audit_Log_Property_Lists,
        Audit_Log_Read_Property, Audit_Log_Write_Property,
        skip_fail_property_list);

    wp_data.object_type = OBJECT_AUDIT_LOG;
    wp_data.object_instance = instance;
    wp_data.priority = BACNET_NO_PRIORITY;
    wp_data.array_index = BACNET_ARRAY_ALL;

    /* test buffer size */
    buffer_size = 512;
    wp_data.object_property = PROP_BUFFER_SIZE;
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, buffer_size);
    status = Audit_Log_Write_Property(&wp_data);
    zassert_true(status, NULL);
    buffer_size = INT_MAX;
    buffer_size++;
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, buffer_size);
    status = Audit_Log_Write_Property(&wp_data);
    /* error out of range */
    zassert_false(status, NULL);
    zassert_equal(wp_data.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(wp_data.error_code, ERROR_CODE_VALUE_OUT_OF_RANGE, NULL);

    wp_data.object_property = PROP_ENABLE;
    wp_data.application_data_len =
        encode_application_boolean(wp_data.application_data, true);
    status = Audit_Log_Write_Property(&wp_data);
    zassert_true(status, NULL);

    wp_data.object_property = PROP_BUFFER_SIZE;
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, buffer_size);
    status = Audit_Log_Write_Property(&wp_data);
    /* error when enabled=true */
    zassert_false(status, NULL);
    zassert_equal(wp_data.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(wp_data.error_code, ERROR_CODE_WRITE_ACCESS_DENIED, NULL);

    /* test object name */
    bacnet_object_name_ascii_test(
        instance, Audit_Log_Name_Set, Audit_Log_Name_ASCII);
    bacnet_object_name_ascii_test(
        instance, Audit_Log_Description_Set, Audit_Log_Description);

    status = Audit_Log_Delete(instance);
    zassert_true(status, NULL);
    Audit_Log_Cleanup();
    len = Audit_Log_Count();
    zassert_true(len == 0, NULL);
    status = Audit_Log_Valid_Instance(instance);
    zassert_false(status, NULL);
}

static void testLogs(void)
{
    const uint32_t instance = 1;
    uint32_t test_instance = 0;
    int len = 0;
    uint32_t buffer_size = 0, test_buffer_size = 0, original_buffer_size = 0;
    uint32_t record_count = 0;
    uint32_t total_record_count = 0;
    bool status = false;
    BACNET_AUDIT_LOG_RECORD *record;

    Audit_Log_Init();
    test_instance = Audit_Log_Create(instance);
    zassert_true(test_instance == instance, NULL);
    len = Audit_Log_Count();
    zassert_true(len > 0, NULL);
    /* Log Buffer settings */
    buffer_size = Audit_Log_Buffer_Size(instance);
    zassert_true(buffer_size > 0, NULL);
    buffer_size = INT_MAX;
    buffer_size++;
    status = Audit_Log_Buffer_Size_Set(instance, buffer_size);
    zassert_false(status, NULL);
    original_buffer_size = Audit_Log_Buffer_Size(instance);
    buffer_size = original_buffer_size / 2;
    status = Audit_Log_Buffer_Size_Set(instance, buffer_size);
    zassert_true(status, NULL);
    test_buffer_size = Audit_Log_Buffer_Size(instance);
    zassert_true(test_buffer_size == buffer_size, NULL);
    status = Audit_Log_Buffer_Size_Set(instance, original_buffer_size);
    zassert_true(status, NULL);
    /* Log Buffer records manipulation */
    record_count = Audit_Log_Record_Count(instance);
    zassert_true(record_count == 0, NULL);
    total_record_count = Audit_Log_Total_Record_Count(instance);
    zassert_true(total_record_count == 0, NULL);
    status = Audit_Log_Enable(instance);
    zassert_false(status, NULL);
    status = Audit_Log_Enable_Set(instance, false);
    zassert_true(status, NULL);
    /* start logging */
    status = Audit_Log_Enable_Set(instance, true);
    zassert_true(status, NULL);
    record_count = Audit_Log_Record_Count(instance);
    zassert_true(record_count == 1, NULL);
    total_record_count = Audit_Log_Total_Record_Count(instance);
    zassert_true(total_record_count == 1, NULL);
    record = Audit_Log_Record_Entry(instance, 0);
    zassert_not_null(record, NULL);
    zassert_true(record->tag == AUDIT_LOG_DATUM_TAG_STATUS, NULL);
    zassert_true(
        BIT_CHECK(record->log_datum.log_status, LOG_STATUS_LOG_DISABLED) == 0,
        NULL);
    Audit_Log_Record_Status_Insert(instance, LOG_STATUS_LOG_INTERRUPTED, true);
    record_count = Audit_Log_Record_Count(instance);
    zassert_true(record_count == 2, NULL);
    total_record_count = Audit_Log_Total_Record_Count(instance);
    zassert_true(total_record_count == 2, NULL);
    record = Audit_Log_Record_Entry(instance, 1);
    zassert_not_null(record, NULL);
    zassert_true(record->tag == AUDIT_LOG_DATUM_TAG_STATUS, NULL);
    zassert_true(
        BIT_CHECK(record->log_datum.log_status, LOG_STATUS_LOG_INTERRUPTED),
        NULL);
    Audit_Log_Record_Entry_Delete(instance, 1);
    record_count = Audit_Log_Record_Count(instance);
    zassert_true(record_count == 1, "record_count: %d", record_count);
    total_record_count = Audit_Log_Total_Record_Count(instance);
    zassert_true(total_record_count == 2, NULL);
    record = Audit_Log_Record_Entry(instance, 0);
    zassert_not_null(record, NULL);
    zassert_true(record->tag == AUDIT_LOG_DATUM_TAG_STATUS, NULL);
    zassert_true(
        BIT_CHECK(record->log_datum.log_status, LOG_STATUS_LOG_DISABLED) == 0,
        NULL);

    Audit_Log_Cleanup();
}
/**
 * @}
 */

void test_main(void)
{
    ztest_test_suite(
        auditlog_tests, ztest_unit_test(testAuditlog),
        ztest_unit_test(testLogs));

    ztest_run_test_suite(auditlog_tests);
}
