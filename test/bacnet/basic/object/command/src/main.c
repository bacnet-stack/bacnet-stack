/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet command object APIs
 */

#include <zephyr/ztest.h>
#include <bacnet/basic/object/command.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void test_object_command(void)
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    int test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata;
    /* for decode value data */
    BACNET_APPLICATION_DATA_VALUE value;
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;
    unsigned port = 0;
    unsigned count = 0;
    uint32_t object_instance = 0;

    object_instance = Command_Index_To_Instance(0);
    Command_Init();
    count = Command_Count();
    zassert_true(count > 0, NULL);
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_COMMAND;
    rpdata.object_instance = object_instance;
    Command_Property_Lists(&pRequired, &pOptional, &pProprietary);
    while ((*pRequired) != -1) {
	rpdata.object_property = *pRequired;
	rpdata.array_index = BACNET_ARRAY_ALL;
	len = Command_Read_Property(&rpdata);
	zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
	if (len > 0) {
	    test_len = bacapp_decode_application_data(
		rpdata.application_data,
		(uint8_t)rpdata.application_data_len, &value);
	    zassert_true(test_len >= 0, NULL);
	}
	pRequired++;
    }
    while ((*pOptional) != -1) {
	rpdata.object_property = *pOptional;
	rpdata.array_index = BACNET_ARRAY_ALL;
	len = Command_Read_Property(&rpdata);
	zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
	if (len > 0) {
	    test_len = bacapp_decode_application_data(
		rpdata.application_data,
		(uint8_t)rpdata.application_data_len, &value);
	    zassert_true(test_len >= 0, NULL);
	}
	pOptional++;
    }
    port++;

    return;
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(tests_object_command,
     ztest_unit_test(test_object_command)
     );

    ztest_run_test_suite(tests_object_command);
}
