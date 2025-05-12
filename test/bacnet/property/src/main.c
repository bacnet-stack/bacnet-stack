/**
 * @file
 * @brief Unit test for BACnet property special lists
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2012
 *
 * SPDX-License-Identifier: MIT
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
ZTEST(property_tests, testPropList)
#else
static void testPropList(void)
#endif
{
    unsigned i = 0, j = 0;
    unsigned count = 0;
    BACNET_PROPERTY_ID property = MAX_BACNET_PROPERTY_ID;
    unsigned object_id = 0, object_name = 0, object_type = 0;
    struct special_property_list_t property_list = { 0 };
    bool status = false;

    for (i = 0; i < OBJECT_PROPRIETARY_MIN; i++) {
        count = property_list_special_count((BACNET_OBJECT_TYPE)i, PROP_ALL);
        zassert_true(count >= 3, NULL);
        object_id = 0;
        object_name = 0;
        object_type = 0;
        for (j = 0; j < count; j++) {
            property = property_list_special_property(
                (BACNET_OBJECT_TYPE)i, PROP_ALL, j);
            if (property == PROP_OBJECT_TYPE) {
                object_type++;
            }
            if (property == PROP_OBJECT_IDENTIFIER) {
                object_id++;
            }
            if (property == PROP_OBJECT_NAME) {
                object_name++;
            }
        }
        zassert_equal(
            object_type, 1, "%s: duplicate object type property",
            bactext_object_type_name((BACNET_OBJECT_TYPE)i));
        zassert_equal(object_id, 1, NULL);
        zassert_equal(object_name, 1, NULL);
        /* test member function */
        property_list_special((BACNET_OBJECT_TYPE)i, &property_list);
        zassert_true(
            property_list_member(
                property_list.Required.pList, PROP_OBJECT_TYPE),
            NULL);
        zassert_true(
            property_list_member(
                property_list.Required.pList, PROP_OBJECT_IDENTIFIER),
            NULL);
        zassert_true(
            property_list_member(
                property_list.Required.pList, PROP_OBJECT_NAME),
            NULL);
        /* validate commandable object property list */
        if ((i == OBJECT_CHANNEL) ||
            (property_list_member(
                 property_list.Required.pList, PROP_PRESENT_VALUE) &&
             (property_list_member(
                  property_list.Required.pList, PROP_PRIORITY_ARRAY) ||
              property_list_member(
                  property_list.Optional.pList, PROP_PRIORITY_ARRAY)))) {
            zassert_true(
                property_list_commandable_member(
                    (BACNET_OBJECT_TYPE)i, PROP_PRESENT_VALUE),
                "Object %s present-value is not listed as commandable",
                bactext_object_type_name((BACNET_OBJECT_TYPE)i));
        } else {
            zassert_false(
                property_list_commandable_member(
                    (BACNET_OBJECT_TYPE)i, PROP_PRESENT_VALUE),
                "Object %s present-value is listed as commandable",
                bactext_object_type_name((BACNET_OBJECT_TYPE)i));
        }
    }
    /* property is a BACnetARRAY */
    for (i = 0; i < OBJECT_PROPRIETARY_MIN; i++) {
        object_type = i;
        status =
            property_list_bacnet_array_member(object_type, PROP_PRESENT_VALUE);
        if (object_type == OBJECT_GLOBAL_GROUP) {
            zassert_true(status, NULL);
        } else {
            zassert_false(status, NULL);
        }
        status =
            property_list_bacnet_array_member(object_type, PROP_PRIORITY_ARRAY);
        zassert_true(status, NULL);
    }
    count = property_list_count(property_list_bacnet_array());
    zassert_true(count > 0, NULL);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(property_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(property_tests, ztest_unit_test(testPropList));

    ztest_run_test_suite(property_tests);
}
#endif
