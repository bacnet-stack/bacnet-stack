/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2023
 *
 * @copyright SPDX-License-Identifier: MIT
 */

#include <zephyr/ztest.h>
#include <bacnet/bacstr.h>
#include <bacnet/basic/object/schedule.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(schedule_tests, testSchedule)
#else
static void testSchedule(void)
#endif
{
    unsigned count = 0;
    uint32_t object_instance = 0;
    const int32_t skip_fail_property_list[] = { -1 };
    BACNET_DAILY_SCHEDULE daily_schedule = { 0 }, test_daily_schedule = { 0 };
    BACNET_SPECIAL_EVENT special_event = { 0 }, *test_special_event;
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE object_property_reference = { 0 },
                                            test_object_property_reference = {
                                                0
                                            };
    BACNET_DATE start_date = { 2023, 1, 1, 0 }, test_start_date = { 0 };
    BACNET_DATE end_date = { 2023, 12, 31, 0 }, test_end_date = { 0 };
    BACNET_TIME time_of_day = { 0 };
    size_t tv = 0, day = 0, i = 0;
    int diff;
    bool status = false;

    object_instance = Schedule_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);
    count = Schedule_Count();
    zassert_true(count > 0, NULL);
    status = Schedule_Valid_Instance(object_instance);
    zassert_true(status, NULL);
    /* creating the same instance again shall not fail or duplicate it */
    zassert_equal(Schedule_Create(object_instance), object_instance, NULL);
    zassert_equal(Schedule_Count(), count, NULL);

    /* fill the weekly schedule with some data */
    for (day = 0; day < BACNET_WEEKLY_SCHEDULE_SIZE; day++) {
        daily_schedule.TV_Count = BACNET_DAILY_SCHEDULE_TIME_VALUES_SIZE;
        for (tv = 0; tv < daily_schedule.TV_Count; tv++) {
            datetime_set_time(
                &daily_schedule.Time_Values[tv].Time, tv % 24, 0, 0, 0);
            daily_schedule.Time_Values[tv].Value.tag =
                BACNET_APPLICATION_TAG_REAL;
            daily_schedule.Time_Values[tv].Value.type.Real = 1.0f + tv;
        }
        status =
            Schedule_Weekly_Schedule_Set(object_instance, day, &daily_schedule);
        zassert_true(status, NULL);
        zassert_equal(
            Schedule_Weekly_Schedule_Time_Value_Count(object_instance, day),
            daily_schedule.TV_Count, NULL);
        status = Schedule_Weekly_Schedule(
            object_instance, day, &test_daily_schedule);
        zassert_true(status, NULL);
        status =
            bacnet_dailyschedule_same(&daily_schedule, &test_daily_schedule);
        zassert_true(status, NULL);
    }
    /* out of range day is rejected */
    status = Schedule_Weekly_Schedule(
        object_instance, BACNET_WEEKLY_SCHEDULE_SIZE, &test_daily_schedule);
    zassert_false(status, NULL);
    /* single Time-Value accessors */
    Schedule_Weekly_Schedule_Time_Value_Delete_All(object_instance, 0);
    zassert_equal(
        Schedule_Weekly_Schedule_Time_Value_Count(object_instance, 0), 0, NULL);
    daily_schedule.Time_Values[0].Value.tag = BACNET_APPLICATION_TAG_REAL;
    daily_schedule.Time_Values[0].Value.type.Real = 42.0f;
    datetime_set_time(&daily_schedule.Time_Values[0].Time, 1, 0, 0, 0);
    status = Schedule_Weekly_Schedule_Time_Value_Set(
        object_instance, 0, 0, &daily_schedule.Time_Values[0]);
    zassert_true(status, NULL);
    zassert_equal(
        Schedule_Weekly_Schedule_Time_Value_Count(object_instance, 0), 1, NULL);
    status = Schedule_Weekly_Schedule_Time_Value(
        object_instance, 0, 0, &test_daily_schedule.Time_Values[0]);
    zassert_true(status, NULL);
    zassert_within(
        test_daily_schedule.Time_Values[0].Value.type.Real, 42.0f, 0.001f,
        NULL);
    /* DoS guard: cannot exceed the per-day maximum Time-Values */
    for (tv = 0; tv < BACNET_SCHEDULE_DAILY_TIME_VALUES_MAX; tv++) {
        Schedule_Weekly_Schedule_Time_Value_Set(
            object_instance, 0, tv, &daily_schedule.Time_Values[0]);
    }
    status = Schedule_Weekly_Schedule_Time_Value_Set(
        object_instance, 0, BACNET_SCHEDULE_DAILY_TIME_VALUES_MAX,
        &daily_schedule.Time_Values[0]);
    zassert_false(status, NULL);

    for (i = 0; i < BACNET_EXCEPTION_SCHEDULE_SIZE; i++) {
        special_event.periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY;
        special_event.timeValues.TV_Count =
            BACNET_DAILY_SCHEDULE_TIME_VALUES_SIZE;
        for (tv = 0; tv < special_event.timeValues.TV_Count; tv++) {
            datetime_set_time(
                &special_event.timeValues.Time_Values[tv].Time, tv % 24, 0, 0,
                0);
            special_event.timeValues.Time_Values[tv].Value.tag =
                BACNET_APPLICATION_TAG_REAL;
            special_event.timeValues.Time_Values[tv].Value.type.Real =
                1.0f + tv;
            special_event.priority = tv % (BACNET_MAX_PRIORITY + 1);
        }
        status =
            Schedule_Exception_Schedule_Set(object_instance, i, &special_event);
        zassert_true(status, NULL);
        test_special_event = Schedule_Exception_Schedule(object_instance, i);
        status = bacnet_special_event_same(&special_event, test_special_event);
        zassert_true(status, NULL);
    }
    zassert_equal(
        Schedule_Exception_Schedule_Count(object_instance),
        BACNET_EXCEPTION_SCHEDULE_SIZE, NULL);
    /* DoS guard: cannot exceed the maximum Exception_Schedule entries */
    status = Schedule_Exception_Schedule_Add(object_instance, &special_event);
    zassert_false(status, NULL);
    status = Schedule_Exception_Schedule_Delete_All(object_instance);
    zassert_true(status, NULL);
    zassert_equal(Schedule_Exception_Schedule_Count(object_instance), 0, NULL);

    /* fill the object property reference with some data */
    for (i = 0; i <
         Schedule_List_Of_Object_Property_References_Capacity(object_instance);
         i++) {
        object_property_reference.objectIdentifier.type = OBJECT_ANALOG_VALUE;
        object_property_reference.objectIdentifier.instance = i + 1;
        object_property_reference.propertyIdentifier = PROP_PRESENT_VALUE;
        object_property_reference.arrayIndex = BACNET_ARRAY_ALL;
        status = Schedule_List_Of_Object_Property_References_Set(
            object_instance, i, &object_property_reference);
        zassert_true(status, NULL);
        status = Schedule_List_Of_Object_Property_References(
            object_instance, i, &test_object_property_reference);
        zassert_true(status, NULL);
        status = bacnet_device_object_property_reference_same(
            &object_property_reference, &test_object_property_reference);
        zassert_true(status, NULL);
    }
    zassert_equal(
        Schedule_List_Of_Object_Property_References_Count(object_instance),
        BACNET_SCHEDULE_OBJ_PROP_REF_SIZE, NULL);
    /* DoS guard: cannot exceed the maximum list capacity */
    object_property_reference.objectIdentifier.instance =
        BACNET_SCHEDULE_OBJ_PROP_REF_SIZE + 1;
    status = Schedule_List_Of_Object_Property_References_Add(
        object_instance, &object_property_reference);
    zassert_false(status, NULL);
    status =
        Schedule_List_Of_Object_Property_References_Delete_All(object_instance);
    zassert_true(status, NULL);
    zassert_equal(
        Schedule_List_Of_Object_Property_References_Count(object_instance), 0,
        NULL);

    bacnet_object_name_ascii_test(
        object_instance, Schedule_Name_Set, Schedule_Name_ASCII);
    status =
        Schedule_Effective_Period_Set(object_instance, &start_date, &end_date);
    zassert_true(status, NULL);
    status = Schedule_Effective_Period(
        object_instance, &test_start_date, &test_end_date);
    zassert_true(status, NULL);
    diff = datetime_compare_date(&start_date, &test_start_date);
    zassert_equal(diff, 0, NULL);
    diff = datetime_compare_date(&end_date, &test_end_date);
    zassert_equal(diff, 0, NULL);
    status = Schedule_In_Effective_Period(object_instance, &start_date);
    zassert_true(status, NULL);
    status = Schedule_In_Effective_Period(object_instance, &end_date);
    zassert_true(status, NULL);
    /* general purpose test */
    bacnet_object_properties_read_write_test(
        OBJECT_SCHEDULE, object_instance, Schedule_Property_Lists,
        Schedule_Read_Property, Schedule_Write_Property,
        skip_fail_property_list);
    Schedule_Recalculate_PV(
        object_instance, BACNET_WEEKDAY_SUNDAY, &time_of_day);
    /* targeted tests for invalid instance, invalid day of week, and NULL
       pointer */
    Schedule_Recalculate_PV(
        BACNET_MAX_INSTANCE, BACNET_WEEKDAY_SUNDAY, &time_of_day);
    Schedule_Recalculate_PV(object_instance, 0, &time_of_day);
    Schedule_Recalculate_PV(object_instance, 8, &time_of_day);
    Schedule_Recalculate_PV(object_instance, BACNET_WEEKDAY_SUNDAY, NULL);
    Schedule_Timer(BACNET_MAX_INSTANCE, 0);
    Schedule_Timer(object_instance, 0);

    status = Schedule_Delete(object_instance);
    zassert_true(status, NULL);
    zassert_equal(Schedule_Count(), count - 1, NULL);
    status = Schedule_Valid_Instance(object_instance);
    zassert_false(status, NULL);
    /* deleting an already deleted instance is a no-op */
    status = Schedule_Delete(object_instance);
    zassert_false(status, NULL);
}

/**
 * @brief Test the object creation, use, and cleanup, checking for any
 *  memory leaks with each pass.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(schedule_tests, testScheduleCreateDelete)
#else
static void testScheduleCreateDelete(void)
#endif
{
    uint32_t object_instance;
    bool status;

    Schedule_Cleanup();
    zassert_equal(Schedule_Count(), 0, NULL);
    object_instance = Schedule_Create(1);
    zassert_equal(object_instance, 1, NULL);
    zassert_equal(Schedule_Count(), 1, NULL);
    /* invalid instance is rejected */
    zassert_equal(
        Schedule_Create(BACNET_MAX_INSTANCE + 1), BACNET_MAX_INSTANCE, NULL);
    status = Schedule_Delete(object_instance);
    zassert_true(status, NULL);
    zassert_equal(Schedule_Count(), 0, NULL);
    /* Cleanup with no objects present is a no-op */
    Schedule_Cleanup();
    zassert_equal(Schedule_Count(), 0, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(schedule_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        schedule_tests, ztest_unit_test(testSchedule),
        ztest_unit_test(testScheduleCreateDelete));

    ztest_run_test_suite(schedule_tests);
}
#endif
