/**
 * @file
 * @brief Unit test for BACnet Calendar object encode/decode APIs
 * @author Mikhail Antropov <michail.antropov@dsr-corporation.com>
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date June 2023
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/calendar.h>
#include <bacnet/bactext.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test Calendar handling
 */
#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_calendar, testCalendar)
#else
static void testCalendar(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    const int32_t *pRequired = NULL;
    const int32_t *pOptional = NULL;
    const int32_t *pProprietary = NULL;
    const uint32_t instance = 1;
    uint32_t test_instance = 0;
    bool status = false;
    unsigned index;
    const char *test_name = NULL;
    char *sample_name = "sample";

    Calendar_Init();
    test_instance = Calendar_Create(instance);
    zassert_equal(test_instance, instance, NULL);
    status = Calendar_Valid_Instance(instance);
    zassert_true(status, NULL);
    index = Calendar_Instance_To_Index(instance);
    zassert_equal(index, 0, NULL);

    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_CALENDAR;
    rpdata.object_instance = instance;
    rpdata.array_index = BACNET_ARRAY_ALL;

    Calendar_Property_Lists(&pRequired, &pOptional, &pProprietary);
    while ((*pRequired) >= 0) {
        rpdata.object_property = *pRequired;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Calendar_Read_Property(&rpdata);
        zassert_not_equal(
            len, BACNET_STATUS_ERROR,
            "property '%s': failed to ReadProperty!\n",
            bactext_property_name(rpdata.object_property));
        if (len >= 0) {
            test_len = bacapp_decode_known_property(
                rpdata.application_data, len, &value, rpdata.object_type,
                rpdata.object_property);
            zassert_equal(
                len, test_len, "property '%s': failed to decode!\n",
                bactext_property_name(rpdata.object_property));
            /* check WriteProperty properties */
            wpdata.object_type = rpdata.object_type;
            wpdata.object_instance = rpdata.object_instance;
            wpdata.object_property = rpdata.object_property;
            wpdata.array_index = rpdata.array_index;
            memcpy(&wpdata.application_data, rpdata.application_data, MAX_APDU);
            wpdata.application_data_len = len;
            wpdata.error_code = ERROR_CODE_SUCCESS;
            status = Calendar_Write_Property(&wpdata);
            if (!status) {
                /* verify WriteProperty property is known */
                zassert_not_equal(
                    wpdata.error_code, ERROR_CODE_UNKNOWN_PROPERTY,
                    "property '%s': WriteProperty Unknown!\n",
                    bactext_property_name(rpdata.object_property));
            }
        }
        pRequired++;
    }
    while ((*pOptional) != -1) {
        rpdata.object_property = *pOptional;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Calendar_Read_Property(&rpdata);
        zassert_not_equal(
            len, BACNET_STATUS_ERROR,
            "property '%s': failed to ReadProperty!\n",
            bactext_property_name(rpdata.object_property));
        if (len > 0) {
            test_len = bacapp_decode_application_data(
                rpdata.application_data, (uint8_t)rpdata.application_data_len,
                &value);
            zassert_equal(
                len, test_len, "property '%s': failed to decode!\n",
                bactext_property_name(rpdata.object_property));
            /* check WriteProperty properties */
            wpdata.object_type = rpdata.object_type;
            wpdata.object_instance = rpdata.object_instance;
            wpdata.object_property = rpdata.object_property;
            wpdata.array_index = rpdata.array_index;
            memcpy(&wpdata.application_data, rpdata.application_data, MAX_APDU);
            wpdata.application_data_len = len;
            wpdata.error_code = ERROR_CODE_SUCCESS;
            status = Calendar_Write_Property(&wpdata);
            if (!status) {
                /* verify WriteProperty property is known */
                zassert_not_equal(
                    wpdata.error_code, ERROR_CODE_UNKNOWN_PROPERTY,
                    "property '%s': WriteProperty Unknown!\n",
                    bactext_property_name(rpdata.object_property));
            }
        }
        pOptional++;
    }
    /* check for unsupported property - use ALL */
    rpdata.object_property = PROP_ALL;
    len = Calendar_Read_Property(&rpdata);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    status = Calendar_Write_Property(&wpdata);
    zassert_false(status, NULL);
    /* test the ASCII name get/set */
    status = Calendar_Name_Set(instance, sample_name);
    zassert_true(status, NULL);
    test_name = Calendar_Name_ASCII(instance);
    zassert_equal(test_name, sample_name, NULL);
    status = Calendar_Name_Set(instance, NULL);
    zassert_true(status, NULL);
    test_name = Calendar_Name_ASCII(instance);
    zassert_equal(test_name, NULL, NULL);
    /* check the delete function */
    status = Calendar_Delete(instance);
    zassert_true(status, NULL);
}

#ifdef CONFIG_ZTEST_NEW_API
ZTEST(bacnet_calendar, testPresentValue)
#else
static void testPresentValue(void)
#endif
{
    const uint32_t instance = 1;
    BACNET_DATE date;
    BACNET_TIME time;
    BACNET_CALENDAR_ENTRY entry;
    BACNET_CALENDAR_ENTRY *value;
    uint32_t test_instance = 0;

    Calendar_Init();
    test_instance = Calendar_Create(instance);
    zassert_equal(test_instance, instance, NULL);

    datetime_local(&date, &time, NULL, NULL);

    // test Date
    zassert_false(Calendar_Present_Value(instance), NULL);
    zassert_equal(0, Calendar_Date_List_Count(instance), NULL);

    entry.tag = BACNET_CALENDAR_DATE;
    entry.type.Date = date;
    entry.type.Date.day++;
    Calendar_Date_List_Add(instance, &entry);
    zassert_equal(1, Calendar_Date_List_Count(instance), NULL);
    zassert_false(Calendar_Present_Value(instance), NULL);

    entry.type.Date = date;
    Calendar_Date_List_Add(instance, &entry);
    zassert_equal(2, Calendar_Date_List_Count(instance), NULL);
    zassert_true(Calendar_Present_Value(instance), NULL);

    value = Calendar_Date_List_Get(instance, 1);
    value->type.Date.day += 2;
    zassert_equal(2, Calendar_Date_List_Count(instance), NULL);
    zassert_false(Calendar_Present_Value(instance), NULL);

    // test Date Range
    entry.tag = BACNET_CALENDAR_DATE_RANGE;
    entry.type.DateRange.startdate = date;
    entry.type.DateRange.enddate = date;
    entry.type.DateRange.startdate.day += 2;
    entry.type.DateRange.enddate.day += 10;
    Calendar_Date_List_Add(instance, &entry);
    zassert_equal(3, Calendar_Date_List_Count(instance), NULL);
    zassert_false(Calendar_Present_Value(instance), NULL);

    value = Calendar_Date_List_Get(instance, 2);
    value->type.DateRange.startdate.day = date.day;
    zassert_true(Calendar_Present_Value(instance), NULL);

    if (date.day > 1) {
        value->type.DateRange.startdate.day--;
        value->type.DateRange.enddate.day = date.day;
        zassert_true(Calendar_Present_Value(instance), NULL);
    }

    value->type.DateRange.startdate.day = date.day + 2;
    value->type.DateRange.enddate.day = date.day + 2;
    zassert_false(Calendar_Present_Value(instance), NULL);

    // test WeekNDay
    entry.tag = BACNET_CALENDAR_WEEK_N_DAY;
    entry.type.WeekNDay.month = 0xff;
    entry.type.WeekNDay.weekofmonth = 0xff;
    entry.type.WeekNDay.dayofweek = 0xff;
    Calendar_Date_List_Add(instance, &entry);
    zassert_equal(4, Calendar_Date_List_Count(instance), NULL);
    value = Calendar_Date_List_Get(instance, 3);
    zassert_true(Calendar_Present_Value(instance), NULL);

    value->type.WeekNDay.month = date.month;
    zassert_true(Calendar_Present_Value(instance), NULL);
    value->type.WeekNDay.month++;
    zassert_false(Calendar_Present_Value(instance), NULL);
    value->type.WeekNDay.month = (date.month % 2) ? 13 : 14;
    zassert_true(Calendar_Present_Value(instance), NULL);
    value->type.WeekNDay.month = (date.month % 2) ? 14 : 13;
    zassert_false(Calendar_Present_Value(instance), NULL);
    value->type.WeekNDay.month = 0xff;

    value->type.WeekNDay.weekofmonth = (date.day - 1) % 7 + 1;
    zassert_true(Calendar_Present_Value(instance), NULL);
    value->type.WeekNDay.weekofmonth++;
    if (value->type.WeekNDay.weekofmonth > 5) {
        value->type.WeekNDay.weekofmonth = 1;
    }
    zassert_false(Calendar_Present_Value(instance), NULL);
    value->type.WeekNDay.weekofmonth = 0xff;

    value->type.WeekNDay.dayofweek = date.wday;
    zassert_true(Calendar_Present_Value(instance), NULL);
    value->type.WeekNDay.dayofweek++;
    if (value->type.WeekNDay.dayofweek > 7) {
        value->type.WeekNDay.dayofweek = 1;
    }
    zassert_false(Calendar_Present_Value(instance), NULL);

    Calendar_Date_List_Delete_All(instance);
    zassert_equal(0, Calendar_Date_List_Count(instance), NULL);

    zassert_true(Calendar_Delete(instance), NULL);
}

/**
 * @}
 */

#ifdef CONFIG_ZTEST_NEW_API
ZTEST_SUITE(bacnet_calendar, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        calendar_tests, ztest_unit_test(testCalendar),
        ztest_unit_test(testPresentValue));

    ztest_run_test_suite(calendar_tests);
}
#endif
