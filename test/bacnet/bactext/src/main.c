/**
 * @file
 * @brief test BACnet Text utility API
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2026
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/property.h>
#include <bacnet/bactext.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(bactext_tests, testBacText)
#else
static void testBacText(void)
#endif
{
    uint32_t i, j, k;
    const char *pString;
    char ascii_number[64] = "";
    uint32_t index, found_index;
    bool status;
    unsigned count;
    BACNET_PROPERTY_ID property;

    for (i = 0; i < MAX_BACNET_CONFIRMED_SERVICE; i++) {
        pString = bactext_confirmed_service_name(i);
        if (pString) {
            status = bactext_confirmed_service_index(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }

    for (i = 0; i < MAX_BACNET_UNCONFIRMED_SERVICE; i++) {
        pString = bactext_unconfirmed_service_name(i);
        if (pString) {
            status = bactext_unconfirmed_service_index(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }

    for (i = 0; i < BACNET_APPLICATION_TAG_EXTENDED_MAX; i++) {
        pString = bactext_application_tag_name(i);
        if (pString) {
            if (i == MAX_BACNET_APPLICATION_TAG) {
                /* skip no-value */
                continue;
            }
            status = bactext_application_tag_index(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    for (i = 0; i < BACNET_OBJECT_TYPE_RESERVED_MIN; i++) {
        pString = bactext_object_type_name(i);
        if (pString) {
            status = bactext_object_type_index(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
            status = bactext_object_property_strtoul(
                (BACNET_OBJECT_TYPE)i, PROP_OBJECT_TYPE, pString, &found_index);
            zassert_true(status, "i=%u", i);
            zassert_equal(
                index, found_index, "index=%u found_index=%u", index,
                found_index);
            status = bactext_object_type_strtol(pString, &found_index);
            zassert_true(status, "i=%u", i);
            zassert_equal(
                index, found_index, "index=%u found_index=%u", index,
                found_index);
        }
        pString = bactext_object_type_name_default(i, NULL);
        zassert_not_null(pString, "i=%u", i);
        pString = bactext_object_type_name_capitalized_default(i, NULL);
        zassert_not_null(pString, "i=%u", i);
        pString = bactext_object_type_name_capitalized(i);
        zassert_not_null(pString, "i=%u", i);
        if (pString) {
            status =
                bactext_object_type_name_capitalized_index(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    for (i = 0; i < OBJECT_PROPRIETARY_MIN; i++) {
        count = property_list_special_count((BACNET_OBJECT_TYPE)i, PROP_ALL);
        for (j = 0; j < count; j++) {
            property = property_list_special_property(
                (BACNET_OBJECT_TYPE)i, PROP_ALL, j);
            pString = bactext_property_name(property);
            if (pString) {
                status = bactext_property_index(pString, &index);
                zassert_true(
                    status, "object=%s(%u) property=%s(%u)",
                    bactext_object_type_name(i), i, pString, property);
                zassert_equal(
                    index, property, "index=%u property=%u", index, property);
                for (k = 0; k < UINT16_MAX; k++) {
                    pString = bactext_object_property_name(
                        (BACNET_OBJECT_TYPE)i, property, k, NULL);
                    if (!pString) {
                        break;
                    }
                    status = bactext_object_property_strtoul(
                        i, property, pString, &found_index);
                    zassert_true(
                        status, "%s i=%u property=%u k=%u", pString, i,
                        property, k);
                    zassert_equal(
                        k, found_index, "%s index=%u found_index=%u", pString,
                        k, found_index);
                }
            }
            pString = bactext_property_name_default(property, NULL);
            zassert_not_null(
                pString, "object=%s(%u) property=%s(%u)",
                bactext_object_type_name(i), i, pString, property);
            index = bactext_property_id(pString);
            zassert_equal(
                index, property, "index=%u property=%u", index, property);
            status = bactext_property_strtol(pString, &found_index);
            zassert_true(status, "i=%u", i);
            zassert_equal(
                index, found_index, "index=%u found_index=%u", index,
                found_index);
        }
    }
    for (k = 0; k < PROP_STATE_PROPRIETARY_MIN; k++) {
        snprintf(ascii_number, sizeof(ascii_number), "%u", k);
        status = bactext_property_states_strtoul(
            (BACNET_PROPERTY_STATES)k, ascii_number, &index);
        zassert_true(status, "k=%u", k);
        zassert_equal(index, k, "index=%u k=%u", index, k);
        for (i = 0; i < 255; i++) {
            pString = bactext_property_states_name(
                (BACNET_PROPERTY_STATES)k, i, NULL);
            if (!pString) {
                break;
            }
            status = bactext_property_states_strtoul(
                (BACNET_PROPERTY_STATES)k, pString, &found_index);
            zassert_true(status, "%s k=%u i=%u", pString, k, i);
            zassert_equal(
                i, found_index, "state=%u[%u]=%s ==>[%u]", k, i, pString,
                found_index);
        }
    }
    for (i = 0; i < UNITS_RESERVED_RANGE_MAX; i++) {
        pString = bactext_engineering_unit_name(i);
        if (pString) {
            status = bactext_engineering_unit_index(pString, &index);
            zassert_true(status, "i=%u %s", i, pString);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
        pString = bactext_engineering_unit_name_default(i, NULL);
        zassert_not_null(pString, "i=%u", i);
    }
    for (i = 0; i < MAX_BACNET_REJECT_REASON; i++) {
        pString = bactext_reject_reason_name(i);
        if (pString) {
            status = bactext_reject_reason_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
        pString = bactext_reject_reason_name_default(i, NULL);
        zassert_not_null(pString, "i=%u", i);
    }
    for (i = 0; i < MAX_BACNET_ABORT_REASON; i++) {
        pString = bactext_abort_reason_name(i);
        if (pString) {
            status = bactext_abort_reason_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
        pString = bactext_abort_reason_name_default(i, NULL);
        zassert_not_null(pString, "i=%u", i);
    }
    for (i = 0; i < MAX_BACNET_ERROR_CLASS; i++) {
        pString = bactext_error_class_name(i);
        if (pString) {
            status = bactext_error_class_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
        pString = bactext_error_class_name_default(i, NULL);
        zassert_not_null(pString, "i=%u", i);
    }
    for (i = 0; i < ERROR_CODE_RESERVED_MAX; i++) {
        pString = bactext_error_code_name_default(i, NULL);
        if (pString) {
            pString = bactext_error_code_name(i);
            status = bactext_error_code_strtol(pString, &index);
            zassert_true(status, "i=%u %s", i, pString);
            zassert_equal(index, i, "index=%u i=%u %s", index, i, pString);
        } else {
            printf("No error code name for %u\n", i);
        }
    }
    for (i = 0; i <= 255; i++) {
        pString = bactext_month_name_default(i, NULL);
        if (pString) {
            pString = bactext_month_name(i);
            status = bactext_month_strtol(pString, &index);
            zassert_true(status, "i=%u %s", i, pString);
            zassert_equal(index, i, "index=%u i=%u %s", index, i, pString);
        }
    }
    for (i = 0; i <= 255; i++) {
        pString = bactext_week_of_month_name_default(i, NULL);
        if (pString) {
            pString = bactext_week_of_month_name(i);
            status = bactext_week_of_month_strtol(pString, &index);
            zassert_true(status, "i=%u %s", i, pString);
            zassert_equal(index, i, "index=%u i=%u %s", index, i, pString);
        }
    }
    for (i = 0; i <= 255; i++) {
        pString = bactext_day_of_week_name_default(i, NULL);
        if (pString) {
            pString = bactext_day_of_week_name(i);
            status = bactext_day_of_week_strtol(pString, &index);
            zassert_true(status, "i=%u %s", i, pString);
            zassert_equal(index, i, "index=%u i=%u %s", index, i, pString);
        }
    }
    for (i = 0; i < 8; i++) {
        pString = bactext_days_of_week_name_default(i, NULL);
        if (pString) {
            pString = bactext_days_of_week_name(i);
            status = bactext_days_of_week_strtol(pString, &index);
            zassert_true(status, "i=%u %s", i, pString);
            zassert_equal(index, i, "index=%u i=%u %s", index, i, pString);
        }
    }
    for (i = 0; i < NOTIFY_MAX; i++) {
        pString = bactext_notify_type_name_default(i, NULL);
        if (pString) {
            pString = bactext_notify_type_name(i);
            status = bactext_notify_type_strtol(pString, &found_index);
            zassert_true(status, "i=%u %s", i, pString);
            zassert_equal(
                i, found_index, "i=%u found_index=%u", i, found_index);
        }
    }
    for (i = 0; i < MAX_BACNET_EVENT_TRANSITION; i++) {
        pString = bactext_event_transition_name(i);
        if (pString) {
            status = bactext_event_transition_strtol(pString, &index);
            zassert_true(status, "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    for (i = 0; i < EVENT_STATE_MAX; i++) {
        pString = bactext_event_state_name(i);
        zassert_not_null(pString, "i=%u", i);
        status = bactext_event_state_strtol(pString, &index);
        zassert_true(status, "i=%u", i);
        zassert_equal(index, i, "index=%u i=%u", index, i);
    }
    for (i = 0; i <= EVENT_CHANGE_OF_TIMER; i++) {
        pString = bactext_event_type_name(i);
        zassert_not_null(pString, "i=%u", i);
        status = bactext_event_type_strtol(pString, &index);
        zassert_true(status, "i=%u %s", i, pString);
        zassert_equal(index, i, "index=%u i=%u", index, i);
    }
    for (i = 0; i < BINARY_PV_MAX; i++) {
        pString = bactext_binary_present_value_name(i);
        zassert_not_null(pString, "i=%u", i);
        status = bactext_binary_present_value_strtol(pString, &index);
        zassert_true(status, "i=%u %s", i, pString);
        zassert_equal(index, i, "index=%u i=%u", index, i);
    }
    for (i = 0; i < MAX_POLARITY; i++) {
        pString = bactext_binary_polarity_name(i);
        zassert_not_null(pString, "i=%u", i);
        status = bactext_binary_polarity_strtol(pString, &index);
        zassert_true(status, "i=%u %s", i, pString);
        zassert_equal(index, i, "index=%u i=%u", index, i);
    }
    for (i = 0; i < RELIABILITY_RESERVED_MIN; i++) {
        pString = bactext_reliability_name(i);
        zassert_not_null(pString, "i=%u", i);
        status = bactext_reliability_strtol(pString, &index);
        zassert_true(status, "i=%u %s", i, pString);
        zassert_equal(index, i, "index=%u i=%u", index, i);
    }
    for (i = 0; i < MAX_DEVICE_STATUS; i++) {
        pString = bactext_device_status_name(i);
        zassert_not_null(pString, "i=%u", i);
        status = bactext_device_status_strtol(pString, &index);
        zassert_true(status, "i=%u %s", i, pString);
        zassert_equal(index, i, "index=%u i=%u", index, i);
    }
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(bactext_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(bactext_tests, ztest_unit_test(testBacText));

    ztest_run_test_suite(bactext_tests);
}
#endif
