/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2023
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/ztest.h>
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
    const int skip_fail_property_list[] = { -1 };
    BACNET_DAILY_SCHEDULE daily_schedule, *test_daily_schedule;
    BACNET_SPECIAL_EVENT special_event, *test_special_event;
    size_t tv = 0, day = 0, i = 0;
    bool status = false;

    Schedule_Init();
    count = Schedule_Count();
    zassert_true(count > 0, NULL);
    object_instance = Schedule_Index_To_Instance(0);
    status = Schedule_Valid_Instance(object_instance);
    zassert_true(status, NULL);

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
        Schedule_Weekly_Schedule_Set(object_instance, day, &daily_schedule);
        test_daily_schedule = Schedule_Weekly_Schedule(object_instance, day);
        status =
            bacnet_dailyschedule_same(&daily_schedule, test_daily_schedule);
        zassert_true(status, NULL);
    }
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
        Schedule_Exception_Schedule_Set(object_instance, i, &special_event);
        test_special_event = Schedule_Exception_Schedule(object_instance, i);
        status = bacnet_special_event_same(&special_event, test_special_event);
        zassert_true(status, NULL);
    }
    /* general purpose test */
    bacnet_object_properties_read_write_test(
        OBJECT_SCHEDULE, object_instance, Schedule_Property_Lists,
        Schedule_Read_Property, Schedule_Write_Property,
        skip_fail_property_list);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(schedule_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(schedule_tests, ztest_unit_test(testSchedule));

    ztest_run_test_suite(schedule_tests);
}
#endif
