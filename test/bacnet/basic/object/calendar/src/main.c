/*
 * Copyright (c) 2023 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet calendar encode/decode APIs
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
static void testCalendar(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = {0};
    const int *required_property = NULL;
    const uint32_t instance = 1;

    Calendar_Init();
    zassert_true(Calendar_Create(instance), NULL);

    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_CALENDAR;
    rpdata.object_instance = instance;
    rpdata.array_index = BACNET_ARRAY_ALL;

    Calendar_Property_Lists(&required_property, NULL, NULL);
    while ((*required_property) >= 0) {
        rpdata.object_property = *required_property;
        len = Calendar_Read_Property(&rpdata);
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
        required_property++;
    }
    zassert_true(Calendar_Delete(instance), NULL);
}

static void testPresentValue(void)
{
    const uint32_t instance = 1;
    BACNET_DATE date;
    BACNET_TIME time;
    BACNET_CALENDAR_ENTRY entry;
    BACNET_CALENDAR_ENTRY *value;

    Calendar_Init();
    zassert_true(Calendar_Create(instance), NULL);

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
        value->type.DateRange.startdate.day --;
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
    if (value->type.WeekNDay.weekofmonth >5)
        value->type.WeekNDay.weekofmonth = 1;
    zassert_false(Calendar_Present_Value(instance), NULL);
    value->type.WeekNDay.weekofmonth = 0xff;

    value->type.WeekNDay.dayofweek = date.wday;
    zassert_true(Calendar_Present_Value(instance), NULL);
    value->type.WeekNDay.dayofweek++;
    if (value->type.WeekNDay.dayofweek > 7)
        value->type.WeekNDay.dayofweek = 1;
    zassert_false(Calendar_Present_Value(instance), NULL);

    Calendar_Date_List_Delete_All(instance);
    zassert_equal(0, Calendar_Date_List_Count(instance), NULL);

    zassert_true(Calendar_Delete(instance), NULL);
}

/**
 * @}
 */

void test_main(void)
{
    ztest_test_suite(calendar_tests,
     ztest_unit_test(testCalendar),
     ztest_unit_test(testPresentValue)
     );

    ztest_run_test_suite(calendar_tests);
}
