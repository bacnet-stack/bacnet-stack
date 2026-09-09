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
#include <bacnet/bacdcode.h>
#include <bacnet/basic/object/schedule.h>
#include <property_test.h>

/* test hook defined in test/bacnet/basic/object/test/device_mock.c */
extern void
Device_getCurrentDateTime_Value_Set(const BACNET_DATE_TIME *datetime);

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
    BACNET_DAILY_SCHEDULE_ENTRY
    entries[BACNET_SCHEDULE_DAILY_TIME_VALUES_MAX] = { 0 };
    BACNET_DAILY_SCHEDULE_ENTRY
    test_entries[BACNET_SCHEDULE_DAILY_TIME_VALUES_MAX] = { 0 };
    size_t test_count = 0;
    BACNET_TIME_VALUE single_tv = { 0 };
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
        for (tv = 0; tv < BACNET_SCHEDULE_DAILY_TIME_VALUES_MAX; tv++) {
            datetime_set_time(&entries[tv].Time_Value.Time, tv % 24, 0, 0, 0);
            entries[tv].Time_Value.Value.tag = BACNET_APPLICATION_TAG_REAL;
            entries[tv].Time_Value.Value.type.Real = 1.0f + tv;
            entries[tv].next = (tv + 1 < BACNET_SCHEDULE_DAILY_TIME_VALUES_MAX)
                ? &entries[tv + 1]
                : NULL;
        }
        status =
            Schedule_Weekly_Schedule_Set(object_instance, day, &entries[0]);
        zassert_true(status, NULL);
        zassert_equal(
            Schedule_Weekly_Schedule_Time_Value_Count(object_instance, day),
            BACNET_SCHEDULE_DAILY_TIME_VALUES_MAX, NULL);
        test_count = 0;
        status = Schedule_Weekly_Schedule(
            object_instance, day, test_entries,
            sizeof(test_entries) / sizeof(test_entries[0]), &test_count);
        zassert_true(status, NULL);
        zassert_equal(test_count, BACNET_SCHEDULE_DAILY_TIME_VALUES_MAX, NULL);
        status = bacnet_dailyschedule_list_same(&entries[0], &test_entries[0]);
        zassert_true(status, NULL);
    }
    /* out of range day is rejected */
    status = Schedule_Weekly_Schedule(
        object_instance, BACNET_WEEKLY_SCHEDULE_SIZE, test_entries,
        sizeof(test_entries) / sizeof(test_entries[0]), &test_count);
    zassert_false(status, NULL);
    /* single Time-Value accessors */
    Schedule_Weekly_Schedule_Time_Value_Delete_All(object_instance, 0);
    zassert_equal(
        Schedule_Weekly_Schedule_Time_Value_Count(object_instance, 0), 0, NULL);
    single_tv.Value.tag = BACNET_APPLICATION_TAG_REAL;
    single_tv.Value.type.Real = 42.0f;
    datetime_set_time(&single_tv.Time, 1, 0, 0, 0);
    status = Schedule_Weekly_Schedule_Time_Value_Set(
        object_instance, 0, 0, &single_tv);
    zassert_true(status, NULL);
    zassert_equal(
        Schedule_Weekly_Schedule_Time_Value_Count(object_instance, 0), 1, NULL);
    status =
        Schedule_Weekly_Schedule_Time_Value(object_instance, 0, 0, &single_tv);
    zassert_true(status, NULL);
    zassert_within(single_tv.Value.type.Real, 42.0f, 0.001f, NULL);
    /* DoS guard: cannot exceed the per-day maximum Time-Values */
    for (tv = 0; tv < BACNET_SCHEDULE_DAILY_TIME_VALUES_MAX; tv++) {
        Schedule_Weekly_Schedule_Time_Value_Set(
            object_instance, 0, tv, &single_tv);
    }
    status = Schedule_Weekly_Schedule_Time_Value_Set(
        object_instance, 0, BACNET_SCHEDULE_DAILY_TIME_VALUES_MAX, &single_tv);
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

#if BACNET_EXCEPTION_SCHEDULE_SIZE
/* stub used to resolve a CalendarReference Exception_Schedule period,
 * always reporting the referenced Calendar as "in effect" */
static int testSchedule_Read_Property_Stub(BACNET_READ_PROPERTY_DATA *rp_data)
{
    return encode_application_boolean(rp_data->application_data, true);
}

/* reads back PROP_PRESENT_VALUE as a Real, since there is no direct
 * Present_Value accessor for this object */
static float testSchedule_Present_Value_Real(uint32_t object_instance)
{
    uint8_t apdu[64] = { 0 };
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    int len;

    rpdata.object_type = OBJECT_SCHEDULE;
    rpdata.object_instance = object_instance;
    rpdata.object_property = PROP_PRESENT_VALUE;
    rpdata.array_index = BACNET_ARRAY_ALL;
    rpdata.application_data = apdu;
    rpdata.application_data_len = sizeof(apdu);
    len = Schedule_Read_Property(&rpdata);
    zassert_true(len > 0, NULL);
    len = bacapp_decode_application_data(apdu, (uint32_t)len, &value);
    zassert_true(len > 0, NULL);
    zassert_equal(value.tag, BACNET_APPLICATION_TAG_REAL, NULL);

    return value.type.Real;
}

/* stub used to capture WriteProperty requests issued to
 * List_Of_Object_Property_References members */
static unsigned testSchedule_Write_Property_Call_Count;
static BACNET_WRITE_PROPERTY_DATA testSchedule_Write_Property_Last_Data;

static bool
testSchedule_Write_Property_Stub(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    testSchedule_Write_Property_Call_Count++;
    testSchedule_Write_Property_Last_Data = *wp_data;

    return true;
}
#endif

/**
 * @brief Test Present_Value calculation with Exception_Schedule in effect
 *  per 135-2024 12.24.4: priority ordering, index tie-break, Null
 *  value fallthrough, and CalendarReference resolution
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(schedule_tests, testScheduleCalendarPresentValueUpdate)
#else
static void testScheduleCalendarPresentValueUpdate(void)
#endif
{
#if BACNET_EXCEPTION_SCHEDULE_SIZE
    uint32_t object_instance;
    BACNET_DAILY_SCHEDULE_ENTRY entry = { 0 };
    BACNET_SPECIAL_EVENT special_event = { 0 };
    BACNET_DATE test_date = { 2024, 6, 15, BACNET_WEEKDAY_SATURDAY };
    BACNET_TIME test_time = { 0 };
    bool status;

    object_instance = Schedule_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);
    datetime_set_time(&test_time, 9, 0, 0, 0);
    /* Weekly_Schedule: Saturday 08:00 -> 10.0 */
    datetime_set_time(&entry.Time_Value.Time, 8, 0, 0, 0);
    entry.Time_Value.Value.tag = BACNET_APPLICATION_TAG_REAL;
    entry.Time_Value.Value.type.Real = 10.0f;
    entry.next = NULL;
    status = Schedule_Weekly_Schedule_Set(
        object_instance, BACNET_WEEKDAY_SATURDAY - 1, &entry);
    zassert_true(status, NULL);

    /* no Exception_Schedule entries yet: weekly value wins */
    Schedule_Calendar_Present_Value_Update(
        object_instance, &test_date, &test_time);
    zassert_within(
        testSchedule_Present_Value_Real(object_instance), 10.0f, 0.001f, NULL);

    /* index 0: priority 5, matches test_date, value 20.0 */
    special_event.periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_ENTRY;
    special_event.period.calendarEntry.tag = BACNET_CALENDAR_DATE;
    special_event.period.calendarEntry.type.Date = test_date;
    special_event.priority = 5;
    special_event.timeValues.TV_Count = 1;
    datetime_set_time(
        &special_event.timeValues.Time_Values[0].Time, 8, 0, 0, 0);
    special_event.timeValues.Time_Values[0].Value.tag =
        BACNET_APPLICATION_TAG_REAL;
    special_event.timeValues.Time_Values[0].Value.type.Real = 20.0f;
    status =
        Schedule_Exception_Schedule_Set(object_instance, 0, &special_event);
    zassert_true(status, NULL);

    /* exception (priority 5) now overrides the weekly schedule value */
    Schedule_Calendar_Present_Value_Update(
        object_instance, &test_date, &test_time);
    zassert_within(
        testSchedule_Present_Value_Real(object_instance), 20.0f, 0.001f, NULL);

    /* index 1: higher relative priority (lower number) wins, value 30.0 */
    special_event.priority = 3;
    special_event.timeValues.Time_Values[0].Value.type.Real = 30.0f;
    status =
        Schedule_Exception_Schedule_Set(object_instance, 1, &special_event);
    zassert_true(status, NULL);
    Schedule_Calendar_Present_Value_Update(
        object_instance, &test_date, &test_time);
    zassert_within(
        testSchedule_Present_Value_Real(object_instance), 30.0f, 0.001f, NULL);

    /* index 2: same priority as index 1, but a higher index loses the
     * tie-break, so value 30.0 (index 1) still wins */
    special_event.timeValues.Time_Values[0].Value.type.Real = 40.0f;
    status =
        Schedule_Exception_Schedule_Set(object_instance, 2, &special_event);
    zassert_true(status, NULL);
    Schedule_Calendar_Present_Value_Update(
        object_instance, &test_date, &test_time);
    zassert_within(
        testSchedule_Present_Value_Real(object_instance), 30.0f, 0.001f, NULL);

    /* a Null value at the winning priority falls through to the next-best
     * candidate (index 0, priority 5, value 20.0) */
    special_event.priority = 3;
    special_event.timeValues.Time_Values[0].Value.tag =
        BACNET_APPLICATION_TAG_NULL;
    status =
        Schedule_Exception_Schedule_Set(object_instance, 1, &special_event);
    zassert_true(status, NULL);
    status =
        Schedule_Exception_Schedule_Set(object_instance, 2, &special_event);
    zassert_true(status, NULL);
    Schedule_Calendar_Present_Value_Update(
        object_instance, &test_date, &test_time);
    zassert_within(
        testSchedule_Present_Value_Real(object_instance), 20.0f, 0.001f, NULL);

    /* CalendarReference period with no callback registered is ignored */
    status = Schedule_Exception_Schedule_Delete_All(object_instance);
    zassert_true(status, NULL);
    special_event.periodTag = BACNET_SPECIAL_EVENT_PERIOD_CALENDAR_REFERENCE;
    special_event.period.calendarReference.type = OBJECT_CALENDAR;
    special_event.period.calendarReference.instance = 0;
    special_event.priority = 1;
    special_event.timeValues.Time_Values[0].Value.tag =
        BACNET_APPLICATION_TAG_REAL;
    special_event.timeValues.Time_Values[0].Value.type.Real = 50.0f;
    status =
        Schedule_Exception_Schedule_Set(object_instance, 0, &special_event);
    zassert_true(status, NULL);
    Schedule_Calendar_Present_Value_Update(
        object_instance, &test_date, &test_time);
    zassert_within(
        testSchedule_Present_Value_Real(object_instance), 10.0f, 0.001f, NULL);

    /* with a resolver callback registered, the CalendarReference resolves
     * to "in effect" and the exception applies */
    Schedule_Read_Property_Internal_Callback_Set(
        testSchedule_Read_Property_Stub);
    Schedule_Calendar_Present_Value_Update(
        object_instance, &test_date, &test_time);
    zassert_within(
        testSchedule_Present_Value_Real(object_instance), 50.0f, 0.001f, NULL);
    Schedule_Read_Property_Internal_Callback_Set(NULL);

    /* invalid arguments are a no-op */
    Schedule_Calendar_Present_Value_Update(
        BACNET_MAX_INSTANCE, &test_date, &test_time);
    Schedule_Calendar_Present_Value_Update(object_instance, NULL, &test_time);
    Schedule_Calendar_Present_Value_Update(object_instance, &test_date, NULL);

    status = Schedule_Delete(object_instance);
    zassert_true(status, NULL);
#endif
}

/**
 * @brief Test that Schedule_Timer() only recalculates Present_Value while
 *  the current date is within the Effective_Period, per 135-2024 12.24.3
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(schedule_tests, testScheduleTimerEffectivePeriod)
#else
static void testScheduleTimerEffectivePeriod(void)
#endif
{
#if BACNET_EXCEPTION_SCHEDULE_SIZE
    uint32_t object_instance;
    BACNET_DAILY_SCHEDULE_ENTRY entry = { 0 };
    BACNET_DATE start_date = { 2024, 1, 1, BACNET_WEEKDAY_MONDAY };
    BACNET_DATE end_date = { 2024, 1, 31, BACNET_WEEKDAY_WEDNESDAY };
    BACNET_DATE_TIME in_period = { 0 }, out_of_period = { 0 };
    bool status;

    object_instance = Schedule_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);
    status =
        Schedule_Effective_Period_Set(object_instance, &start_date, &end_date);
    zassert_true(status, NULL);
    /* Weekly_Schedule: Monday 08:00 -> 10.0, distinct from Schedule_Default */
    datetime_set_time(&entry.Time_Value.Time, 8, 0, 0, 0);
    entry.Time_Value.Value.tag = BACNET_APPLICATION_TAG_REAL;
    entry.Time_Value.Value.type.Real = 10.0f;
    entry.next = NULL;
    status = Schedule_Weekly_Schedule_Set(
        object_instance, BACNET_WEEKDAY_MONDAY - 1, &entry);
    zassert_true(status, NULL);

    /* inside the Effective_Period: recalculation happens */
    in_period.date = (BACNET_DATE) { 2024, 1, 15, BACNET_WEEKDAY_MONDAY };
    datetime_set_time(&in_period.time, 9, 0, 0, 0);
    Device_getCurrentDateTime_Value_Set(&in_period);
    Schedule_Timer(object_instance, 0);
    zassert_within(
        testSchedule_Present_Value_Real(object_instance), 10.0f, 0.001f, NULL);

    /* outside the Effective_Period: recalculation is skipped, so
     * Present_Value keeps its prior value instead of falling back to
     * Schedule_Default for a non-matching weekday */
    out_of_period.date = (BACNET_DATE) { 2024, 2, 15, BACNET_WEEKDAY_THURSDAY };
    datetime_set_time(&out_of_period.time, 9, 0, 0, 0);
    Device_getCurrentDateTime_Value_Set(&out_of_period);
    Schedule_Timer(object_instance, 0);
    zassert_within(
        testSchedule_Present_Value_Real(object_instance), 10.0f, 0.001f, NULL);

    /* back inside the Effective_Period: recalculation resumes */
    Device_getCurrentDateTime_Value_Set(&in_period);
    Schedule_Timer(object_instance, 0);
    zassert_within(
        testSchedule_Present_Value_Real(object_instance), 10.0f, 0.001f, NULL);

    Device_getCurrentDateTime_Value_Set(NULL);
    status = Schedule_Delete(object_instance);
    zassert_true(status, NULL);
#endif
}

/**
 * @brief Test that List_Of_Object_Property_References members are written
 *  when Present_Value changes, and that Write_Every_Scheduled_Action
 *  forces one write when a new scheduled action becomes active (even
 *  without a value change), but not on repeated recalculations of the
 *  same still-active action, per 135-2024 12.24.4/12.24.25
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(schedule_tests, testScheduleWriteEveryScheduledAction)
#else
static void testScheduleWriteEveryScheduledAction(void)
#endif
{
#if BACNET_EXCEPTION_SCHEDULE_SIZE
    uint32_t object_instance;
    BACNET_DAILY_SCHEDULE_ENTRY entry = { 0 }, entry2 = { 0 };
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE member = { 0 };
    BACNET_DATE test_date = { 2024, 6, 15, BACNET_WEEKDAY_SATURDAY };
    BACNET_TIME test_time_before = { 0 }, test_time_mid = { 0 },
                test_time_after = { 0 };
    bool status;

    object_instance = Schedule_Create(BACNET_MAX_INSTANCE);
    zassert_not_equal(object_instance, BACNET_MAX_INSTANCE, NULL);
    /* Write_Every_Scheduled_Action defaults to FALSE */
    zassert_false(Schedule_Write_Every_Scheduled_Action(object_instance), NULL);

    /* one member reference to write Present_Value to */
    member.objectIdentifier.type = OBJECT_ANALOG_VALUE;
    member.objectIdentifier.instance = 1;
    member.propertyIdentifier = PROP_PRESENT_VALUE;
    member.arrayIndex = BACNET_ARRAY_ALL;
    status = Schedule_List_Of_Object_Property_References_Add(
        object_instance, &member);
    zassert_true(status, NULL);

    /* Weekly_Schedule: Saturday 08:00 -> 10.0, then 08:30 -> 10.0 (same
     * value as the first action, but a distinct scheduled action) */
    datetime_set_time(&entry.Time_Value.Time, 8, 0, 0, 0);
    entry.Time_Value.Value.tag = BACNET_APPLICATION_TAG_REAL;
    entry.Time_Value.Value.type.Real = 10.0f;
    entry.next = &entry2;
    datetime_set_time(&entry2.Time_Value.Time, 8, 30, 0, 0);
    entry2.Time_Value.Value.tag = BACNET_APPLICATION_TAG_REAL;
    entry2.Time_Value.Value.type.Real = 10.0f;
    entry2.next = NULL;
    status = Schedule_Weekly_Schedule_Set(
        object_instance, BACNET_WEEKDAY_SATURDAY - 1, &entry);
    zassert_true(status, NULL);
    datetime_set_time(&test_time_before, 7, 0, 0, 0);
    datetime_set_time(&test_time_mid, 8, 15, 0, 0);
    datetime_set_time(&test_time_after, 9, 0, 0, 0);

    Schedule_Write_Property_Internal_Callback_Set(
        testSchedule_Write_Property_Stub);
    testSchedule_Write_Property_Call_Count = 0;

    /* before 08:00: falls back to Schedule_Default, unchanged from the
     * initial value, so no write occurs */
    Schedule_Calendar_Present_Value_Update(
        object_instance, &test_date, &test_time_before);
    zassert_equal(testSchedule_Write_Property_Call_Count, 0, NULL);

    /* 08:00 action activates: Present_Value changes (Default -> 10.0):
     * one write */
    Schedule_Calendar_Present_Value_Update(
        object_instance, &test_date, &test_time_mid);
    zassert_equal(testSchedule_Write_Property_Call_Count, 1, NULL);
    zassert_equal(
        testSchedule_Write_Property_Last_Data.object_type, OBJECT_ANALOG_VALUE,
        NULL);
    zassert_equal(
        testSchedule_Write_Property_Last_Data.object_instance, 1, NULL);
    zassert_equal(
        testSchedule_Write_Property_Last_Data.object_property,
        PROP_PRESENT_VALUE, NULL);

    /* same action still active, Present_Value unchanged: no write while
     * Write_Every_Scheduled_Action is FALSE */
    Schedule_Calendar_Present_Value_Update(
        object_instance, &test_date, &test_time_mid);
    zassert_equal(testSchedule_Write_Property_Call_Count, 1, NULL);

    /* Write_Every_Scheduled_Action = TRUE, but the 08:00 action is still
     * the one in effect (no new action, no value change): no write - a
     * poll of an already-active action must not repeat the writeback */
    status = Schedule_Write_Every_Scheduled_Action_Set(object_instance, true);
    zassert_true(status, NULL);
    zassert_true(Schedule_Write_Every_Scheduled_Action(object_instance), NULL);
    Schedule_Calendar_Present_Value_Update(
        object_instance, &test_date, &test_time_mid);
    zassert_equal(testSchedule_Write_Property_Call_Count, 1, NULL);

    /* 08:30 action activates: Present_Value is unchanged (10.0 -> 10.0),
     * but it is a new scheduled action, so it is written */
    Schedule_Calendar_Present_Value_Update(
        object_instance, &test_date, &test_time_after);
    zassert_equal(testSchedule_Write_Property_Call_Count, 2, NULL);

    /* same 08:30 action still active: no additional write */
    Schedule_Calendar_Present_Value_Update(
        object_instance, &test_date, &test_time_after);
    zassert_equal(testSchedule_Write_Property_Call_Count, 2, NULL);

    /* WriteProperty/ReadProperty round-trip for
     * PROP_WRITE_EVERY_SCHEDULED_ACTION */
    {
        uint8_t apdu[16] = { 0 };
        BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };
        BACNET_READ_PROPERTY_DATA rp_data = { 0 };
        BACNET_APPLICATION_DATA_VALUE value = { 0 };
        int len;

        wp_data.object_type = OBJECT_SCHEDULE;
        wp_data.object_instance = object_instance;
        wp_data.object_property = PROP_WRITE_EVERY_SCHEDULED_ACTION;
        wp_data.array_index = BACNET_ARRAY_ALL;
        wp_data.application_data_len =
            encode_application_boolean(wp_data.application_data, false);
        status = Schedule_Write_Property(&wp_data);
        zassert_true(status, NULL);
        zassert_false(
            Schedule_Write_Every_Scheduled_Action(object_instance), NULL);

        rp_data.object_type = OBJECT_SCHEDULE;
        rp_data.object_instance = object_instance;
        rp_data.object_property = PROP_WRITE_EVERY_SCHEDULED_ACTION;
        rp_data.array_index = BACNET_ARRAY_ALL;
        rp_data.application_data = apdu;
        rp_data.application_data_len = sizeof(apdu);
        len = Schedule_Read_Property(&rp_data);
        zassert_true(len > 0, NULL);
        len = bacapp_decode_application_data(apdu, (uint32_t)len, &value);
        zassert_true(len > 0, NULL);
        zassert_equal(value.tag, BACNET_APPLICATION_TAG_BOOLEAN, NULL);
        zassert_false(value.type.Boolean, NULL);
    }

    /* invalid instance / no member references are a no-op, not a crash */
    Schedule_Write_Property_Internal_Callback_Set(NULL);
    Schedule_Calendar_Present_Value_Update(
        object_instance, &test_date, &test_time_before);

    status = Schedule_Delete(object_instance);
    zassert_true(status, NULL);
#endif
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
        ztest_unit_test(testScheduleCalendarPresentValueUpdate),
        ztest_unit_test(testScheduleTimerEffectivePeriod),
        ztest_unit_test(testScheduleWriteEveryScheduledAction),
        ztest_unit_test(testScheduleCreateDelete));

    ztest_run_test_suite(schedule_tests);
}
#endif
