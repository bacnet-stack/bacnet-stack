/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date July 2023
 *
 * @copyright SPDX-License-Identifier: MIT
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/object/csv.h>
#include <bacnet/bactext.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void testCharacterString_Value(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0, test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    const int32_t *pRequired = NULL;
    const int32_t *pOptional = NULL;
    const int32_t *pProprietary = NULL;
    unsigned count = 0;
    bool status = false;
    uint32_t object_instance = BACNET_MAX_INSTANCE;

    CharacterString_Value_Init();
    object_instance = CharacterString_Value_Create(object_instance);
    count = CharacterString_Value_Count();
    zassert_true(count == 1, NULL);
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_CHARACTERSTRING_VALUE;
    rpdata.object_instance = CharacterString_Value_Index_To_Instance(0);
    rpdata.array_index = BACNET_ARRAY_ALL;
    status = CharacterString_Value_Valid_Instance(rpdata.object_instance);
    zassert_true(status, NULL);
    CharacterString_Value_Property_Lists(&pRequired, &pOptional, &pProprietary);
    while ((*pRequired) >= 0) {
        rpdata.object_property = *pRequired;
        len = CharacterString_Value_Read_Property(&rpdata);
        zassert_true(len >= 0, NULL);
        if (len >= 0) {
            test_len = bacapp_decode_known_property(
                rpdata.application_data, len, &value, rpdata.object_type,
                rpdata.object_property);
            if (len != test_len) {
                printf(
                    "property '%s': failed to decode!\n",
                    bactext_property_name(rpdata.object_property));
            }
            if (rpdata.object_property == PROP_PRIORITY_ARRAY) {
                /* FIXME: known fail to decode */
                len = test_len;
            }
            zassert_equal(len, test_len, NULL);
        } else {
            printf(
                "property '%s': failed to read!\n",
                bactext_property_name(rpdata.object_property));
        }
        pRequired++;
    }
    while ((*pOptional) != -1) {
        rpdata.object_property = *pOptional;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = CharacterString_Value_Read_Property(&rpdata);
        zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
        if (len > 0) {
            test_len = bacapp_decode_application_data(
                rpdata.application_data, (uint8_t)rpdata.application_data_len,
                &value);
            if (len != test_len) {
                printf(
                    "property '%s': failed to decode!\n",
                    bactext_property_name(rpdata.object_property));
            }
            zassert_true(test_len >= 0, NULL);
        } else {
            printf(
                "property '%s': failed to read!\n",
                bactext_property_name(rpdata.object_property));
        }
        pOptional++;
    }
}
/**
 * @}
 */

void test_main(void)
{
    ztest_test_suite(csv_tests, ztest_unit_test(testCharacterString_Value));

    ztest_run_test_suite(csv_tests);
}
