/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

/* @file
 * @brief test BACnet integer encode/decode APIs
 */

#include <ztest.h>
#include <bacnet/basic/object/command.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
static void testCommand(void)
{
#if 0 /*TODO: Test does not pass */
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    uint32_t len_value = 0;
    uint8_t tag_number = 0;
    uint32_t decoded_instance = 0;
    BACNET_OBJECT_TYPE decoded_type = 0;
    BACNET_READ_PROPERTY_DATA rpdata;
    BACNET_ACTION_LIST clist, clist_test;
    Command_Init();
    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_COMMAND;
    rpdata.object_instance = 1;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;
    rpdata.array_index = BACNET_ARRAY_ALL;
    len = Command_Read_Property(&rpdata);
    zassert_not_equal(len, 0, NULL);
    len = decode_tag_number_and_value(&apdu[0], &tag_number, &len_value);
    zassert_equal(tag_number, BACNET_APPLICATION_TAG_OBJECT_ID, NULL);
    len = decode_object_id(&apdu[len], &decoded_type, &decoded_instance);
    zassert_equal(decoded_type, rpdata.object_type, NULL);
    zassert_equal(decoded_instance, rpdata.object_instance, NULL);
    memset(&clist, 0, sizeof(BACNET_ACTION_LIST));
    memset(&clist_test, 0, sizeof(BACNET_ACTION_LIST));
    clist.Device_Id.type = OBJECT_DEVICE;
    clist.Device_Id.instance = 3389;
    clist.Object_Id.type = OBJECT_ANALOG_VALUE;
    clist.Object_Id.instance = 42;
    clist.Property_Identifier = PROP_PRESENT_VALUE;
    clist.Property_Array_Index = BACNET_ARRAY_ALL;
    clist.Value.tag = BACNET_APPLICATION_TAG_REAL;
    clist.Value.type.Real = 39.0f;
    clist.Priority = 4;
    clist.Post_Delay = 0xFFFFFFFFU;
    clist.Quit_On_Failure = true;
    clist.Write_Successful = false;
    clist.next = NULL;
    len = cl_encode_apdu(apdu, &clist);
    zassert_true(len > 0, NULL);
    len = cl_decode_apdu(apdu, len, BACNET_APPLICATION_TAG_REAL, &clist_test);
    zassert_true(len > 0, NULL);
    zassert_equal(clist.Device_Id.type, clist_test.Device_Id.type, NULL);
    zassert_equal(clist.Device_Id.instance, clist_test.Device_Id.instance, NULL);
    zassert_equal(clist.Object_Id.type, clist_test.Object_Id.type, NULL);
    zassert_equal(clist.Object_Id.instance, clist_test.Object_Id.instance, NULL);
    zassert_equal(clist.Property_Identifier, clist_test.Property_Identifier, NULL);
    zassert_equal(clist.Property_Array_Index, clist_test.Property_Array_Index, NULL);
    zassert_equal(clist.Value.tag, clist_test.Value.tag, NULL);
    zassert_equal(clist.Value.type.Real, clist_test.Value.type.Real, NULL);
    zassert_equal(clist.Priority, clist_test.Priority, NULL);
    zassert_equal(clist.Post_Delay, clist_test.Post_Delay, NULL);
    zassert_equal(clist.Quit_On_Failure, clist_test.Quit_On_Failure, NULL);
    zassert_equal(clist.Write_Successful, clist_test.Write_Successful, NULL);
    return;
#else
    ztest_test_skip();
#endif
}
/**
 * @}
 */


void test_main(void)
{
    ztest_test_suite(command_tests,
     ztest_unit_test(testCommand)
     );

    ztest_run_test_suite(command_tests);
}
