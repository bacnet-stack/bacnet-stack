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
#include <property_test.h>

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
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };
    const int32_t *pRequired = NULL;
    const int32_t skip_fail_property_list[] = { -1 };
    const uint32_t instance = 1;
    uint32_t test_instance = 0;
    bool status = false;
    unsigned index;
    const char *test_name = NULL;
    char *sample_name = "sample";
    bool writable_name = false;
    bool writable_description = false;
    const int32_t *property = NULL;
    BACNET_CHARACTER_STRING new_name = { 0 };
    BACNET_CHARACTER_STRING new_description = { 0 };
    const char *updated_name = "renamed-calendar";
    const char *updated_description = "updated description";

    Calendar_Init();
    test_instance = Calendar_Create(instance);
    zassert_equal(test_instance, instance, NULL);
    status = Calendar_Valid_Instance(instance);
    zassert_true(status, NULL);
    index = Calendar_Instance_To_Index(instance);
    zassert_equal(index, 0, NULL);

    bacnet_object_properties_read_write_test(
        OBJECT_CALENDAR, instance, Calendar_Property_Lists,
        Calendar_Read_Property, Calendar_Write_Property,
        skip_fail_property_list);
    /* test the writable property list */
    Calendar_Writable_Property_List(instance, &pRequired);
    zassert_not_null(pRequired, NULL);
    writable_name = false;
    writable_description = false;
    property = pRequired;

    while (*property != -1) {
        if (*property == PROP_OBJECT_NAME) {
            writable_name = true;
        }
        if (*property == PROP_DESCRIPTION) {
            writable_description = true;
        }
        property++;
    }
    zassert_true(writable_name || writable_description, NULL);
    /* test the ANSI name get/set */
    status = Calendar_Name_Set(instance, sample_name);
    zassert_true(status, NULL);
    test_name = Calendar_Name_ASCII(instance);
    zassert_true(strcmp(test_name, sample_name) == 0, NULL);
    status = Calendar_Name_Set(instance, NULL);
    zassert_true(status, NULL);
    test_name = Calendar_Name_ASCII(instance);
    zassert_equal(test_name, NULL, NULL);

    characterstring_init_ansi(&new_name, updated_name);
    wpdata.object_type = OBJECT_CALENDAR;
    wpdata.object_instance = instance;
    wpdata.object_property = PROP_OBJECT_NAME;
    wpdata.array_index = BACNET_ARRAY_ALL;
    wpdata.application_data_len =
        encode_application_character_string(&apdu[0], &new_name);
    memcpy(wpdata.application_data, apdu, sizeof(apdu));
    status = Calendar_Write_Property(&wpdata);
    zassert_true(status, "PROP_OBJECT_NAME write should succeed");
    zassert_true(
        strcmp(Calendar_Name_ASCII(instance), updated_name) == 0,
        "object name should match updated value");

    characterstring_init_ansi(&new_description, updated_description);
    wpdata.object_property = PROP_DESCRIPTION;
    wpdata.application_data_len =
        encode_application_character_string(&apdu[0], &new_description);
    memcpy(wpdata.application_data, apdu, sizeof(apdu));
    status = Calendar_Write_Property(&wpdata);
    zassert_true(status, "PROP_DESCRIPTION write should succeed");
    zassert_true(
        strcmp(Calendar_Description(instance), updated_description) == 0,
        "description should match updated value");
    /* test the duplicate (copy) name/description APIs */
    bacnet_object_name_ascii_duplicate_test(
        instance, Calendar_Name_Duplicate, Calendar_Name_ASCII);
    bacnet_object_name_ascii_duplicate_test(
        instance, Calendar_Description_Duplicate, Calendar_Description);
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

    value->type.WeekNDay.weekofmonth = (date.day - 1) / 7 + 1;
    zassert_true(Calendar_Present_Value(instance), NULL);
    /* week 6 means "last 7 days of month", which can overlap with week
       5, so pick a week bucket unrelated to the current date instead
       of incrementing (which could wrap into a matching week 6) */
    value->type.WeekNDay.weekofmonth =
        (value->type.WeekNDay.weekofmonth == 1) ? 2 : 1;
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
