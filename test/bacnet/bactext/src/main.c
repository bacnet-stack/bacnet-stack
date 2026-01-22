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

    for (i = 0; i < BACNET_APPLICATION_TAG_EXTENDED_MAX; i++) {
        pString = bactext_application_tag_name(i);
        if (pString) {
            if (i == MAX_BACNET_APPLICATION_TAG) {
                /* skip no-value */
                continue;
            }
            zassert_true(
                bactext_application_tag_index(pString, &index), "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
        }
    }
    for (i = 0; i < BACNET_OBJECT_TYPE_RESERVED_MIN; i++) {
        pString = bactext_object_type_name(i);
        if (pString) {
            zassert_true(bactext_object_type_index(pString, &index), "i=%u", i);
            zassert_equal(index, i, "index=%u i=%u", index, i);
            status = bactext_object_property_strtoul(
                (BACNET_OBJECT_TYPE)i, PROP_OBJECT_TYPE, pString, &found_index);
            zassert_true(status, "i=%u", i);
            zassert_equal(
                index, found_index, "index=%u found_index=%u", index,
                found_index);
        }
    }
    for (i = 0; i < OBJECT_PROPRIETARY_MIN; i++) {
        count = property_list_special_count((BACNET_OBJECT_TYPE)i, PROP_ALL);
        for (j = 0; j < count; j++) {
            property = property_list_special_property(
                (BACNET_OBJECT_TYPE)i, PROP_ALL, j);
            pString = bactext_property_name(property);
            if (pString) {
                zassert_true(
                    bactext_property_index(pString, &index),
                    "object=%s(%u) property=%s(%u)",
                    bactext_object_type_name(i), i, pString, property);
                zassert_equal(
                    index, property, "index=%u property=%u", index, property);
                status = bactext_object_property_strtoul(
                    i, property, pString, &found_index);
                if (status) {
                    /* only enumerated properties */
                    zassert_true(status, "i=%u", i);
                    zassert_equal(
                        index, found_index, "index=%u found_index=%u", index,
                        found_index);
                }
            }
        }
    }
    for (k = 0; k < PROP_STATE_PROPRIETARY_MIN; k++) {
        snprintf(ascii_number, sizeof(ascii_number), "%u", k);
        status = bactext_property_states_strtoul(
            (BACNET_PROPERTY_STATES)k, ascii_number, &index);
        zassert_true(status, "k=%u", k);
        /* only enumerated properties */
        zassert_equal(index, k, "index=%u k=%u", index, k);
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
