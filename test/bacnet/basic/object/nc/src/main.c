/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date December 2022
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/bactext.h>
#include <bacnet/rp.h>
#include <bacnet/wp.h>
#include <bacnet/list_element.h>
#include <bacnet/basic/object/nc.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(notification_class_tests, test_Notification_Class_Read_Write_Property)
#else
static void test_Notification_Class_Read_Write_Property(void)
#endif
{
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    int test_len = 0;
    BACNET_READ_PROPERTY_DATA rpdata = { 0 };
    BACNET_APPLICATION_DATA_VALUE value = { 0 };
    const int *pRequired = NULL;
    const int *pOptional = NULL;
    const int *pProprietary = NULL;
    const uint32_t instance = 1;
    uint32_t test_instance = 0;
    BACNET_WRITE_PROPERTY_DATA wpdata = { 0 };
    bool status = false;
    unsigned index;
    unsigned count;

    Notification_Class_Init();
    status = Notification_Class_Valid_Instance(instance);
    zassert_true(status, NULL);
    index = Notification_Class_Instance_To_Index(instance);
    zassert_equal(index, instance, "index=%u", index);
    test_instance = Notification_Class_Index_To_Instance(index);
    zassert_equal(test_instance, instance, "test_instance=%u", test_instance);
    count = Notification_Class_Count();
    zassert_true(count > 0, NULL);
    status = Notification_Class_Valid_Instance(BACNET_MAX_INSTANCE);
    zassert_false(status, NULL);

    rpdata.application_data = &apdu[0];
    rpdata.application_data_len = sizeof(apdu);
    rpdata.object_type = OBJECT_NOTIFICATION_CLASS;
    rpdata.object_instance = instance;
    rpdata.object_property = PROP_OBJECT_IDENTIFIER;

    len = Notification_Class_Read_Property(NULL);
    zassert_equal(len, 0, NULL);
    Notification_Class_Property_Lists(&pRequired, &pOptional, &pProprietary);
    while ((*pRequired) >= 0) {
        rpdata.object_property = *pRequired;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Notification_Class_Read_Property(&rpdata);
        zassert_not_equal(
            len, BACNET_STATUS_ERROR,
            "property '%s': failed to ReadProperty!\n",
            bactext_property_name(rpdata.object_property));
        if (len >= 0) {
            test_len = bacapp_decode_known_property(
                rpdata.application_data, len, &value, rpdata.object_type,
                rpdata.object_property);
            if ((rpdata.object_property != PROP_PRIORITY) &&
                (rpdata.object_property != PROP_RECIPIENT_LIST)) {
                zassert_equal(
                    len, test_len, "property '%s': failed to decode!\n",
                    bactext_property_name(rpdata.object_property));
            }
            /* check WriteProperty properties */
            wpdata.object_type = rpdata.object_type;
            wpdata.object_instance = rpdata.object_instance;
            wpdata.object_property = rpdata.object_property;
            wpdata.array_index = rpdata.array_index;
            memcpy(&wpdata.application_data, rpdata.application_data, MAX_APDU);
            wpdata.application_data_len = len;
            wpdata.error_code = ERROR_CODE_SUCCESS;
            status = Notification_Class_Write_Property(&wpdata);
            if (!status) {
                /* verify WriteProperty property is known */
                zassert_not_equal(
                    wpdata.error_code, ERROR_CODE_UNKNOWN_PROPERTY,
                    "property '%s': WriteProperty Unknown!\n",
                    bactext_property_name(rpdata.object_property));
            }
        }
        pRequired++;
    }
    while ((*pOptional) != -1) {
        rpdata.object_property = *pOptional;
        rpdata.array_index = BACNET_ARRAY_ALL;
        len = Notification_Class_Read_Property(&rpdata);
        zassert_not_equal(
            len, BACNET_STATUS_ERROR,
            "property '%s': failed to ReadProperty!\n",
            bactext_property_name(rpdata.object_property));
        if (len > 0) {
            test_len = bacapp_decode_application_data(
                rpdata.application_data, (uint8_t)rpdata.application_data_len,
                &value);
            zassert_equal(
                len, test_len, "property '%s': failed to decode!\n",
                bactext_property_name(rpdata.object_property));
            /* check WriteProperty properties */
            wpdata.object_type = rpdata.object_type;
            wpdata.object_instance = rpdata.object_instance;
            wpdata.object_property = rpdata.object_property;
            wpdata.array_index = rpdata.array_index;
            memcpy(&wpdata.application_data, rpdata.application_data, MAX_APDU);
            wpdata.application_data_len = len;
            wpdata.error_code = ERROR_CODE_SUCCESS;
            status = Notification_Class_Write_Property(&wpdata);
            if (!status) {
                /* verify WriteProperty property is known */
                zassert_not_equal(
                    wpdata.error_code, ERROR_CODE_UNKNOWN_PROPERTY,
                    "property '%s': WriteProperty Unknown!\n",
                    bactext_property_name(rpdata.object_property));
            }
        }
        pOptional++;
    }
    /* array property - index 0 = size */
    rpdata.object_property = PROP_PRIORITY;
    rpdata.array_index = 0;
    len = Notification_Class_Read_Property(&rpdata);
    zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
    wpdata.object_type = rpdata.object_type;
    wpdata.object_property = rpdata.object_property;
    wpdata.array_index = rpdata.array_index;
    memcpy(&wpdata.application_data, rpdata.application_data, MAX_APDU);
    wpdata.error_code = ERROR_CODE_SUCCESS;
    status = Notification_Class_Write_Property(&wpdata);
    zassert_false(status, NULL);
    /* array property - index 1..N */
    rpdata.array_index = 1;
    len = Notification_Class_Read_Property(&rpdata);
    zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
    rpdata.array_index = 2;
    len = Notification_Class_Read_Property(&rpdata);
    zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
    rpdata.array_index = 3;
    len = Notification_Class_Read_Property(&rpdata);
    zassert_not_equal(len, BACNET_STATUS_ERROR, NULL);
    /* array property - index N+1 - non-existing */
    rpdata.array_index = 4;
    len = Notification_Class_Read_Property(&rpdata);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    /* WriteProperty - priority property */
    /* non-existing property of the object */
    rpdata.object_property = PROP_ALL;
    len = Notification_Class_Read_Property(&rpdata);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);
    wpdata.object_property = PROP_ALL;
    status = Notification_Class_Write_Property(&wpdata);
    zassert_false(status, NULL);

    return;
}

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(notification_class_tests, test_Notification_Class_Priority)
#else
static void test_Notification_Class_Priority(void)
#endif
{
    const uint32_t instance = 1;
    bool status = false;
    uint32_t priority_array[3] = { 0 };

    Notification_Class_Init();
    status = Notification_Class_Valid_Instance(instance);
    zassert_true(status, NULL);

    /* API testing */
    Notification_Class_Get_Priorities(BACNET_MAX_INSTANCE, priority_array);
    zassert_equal(
        priority_array[0], 255, "priority_array[0]=%u", priority_array[0]);
    zassert_equal(
        priority_array[1], 255, "priority_array[1]=%u", priority_array[1]);
    zassert_equal(
        priority_array[2], 255, "priority_array[2]=%u", priority_array[2]);
    Notification_Class_Get_Priorities(instance, priority_array);
    zassert_equal(
        priority_array[0], 255, "priority_array[0]=%u", priority_array[0]);
    zassert_equal(
        priority_array[1], 255, "priority_array[1]=%u", priority_array[1]);
    zassert_equal(
        priority_array[2], 255, "priority_array[2]=%u", priority_array[2]);

    priority_array[0] = 1;
    priority_array[1] = 2;
    priority_array[2] = 3;
    Notification_Class_Set_Priorities(instance, priority_array);
    Notification_Class_Get_Priorities(instance, priority_array);
    zassert_equal(
        priority_array[0], 1, "priority_array[0]=%u", priority_array[0]);
    zassert_equal(
        priority_array[1], 2, "priority_array[1]=%u", priority_array[1]);
    zassert_equal(
        priority_array[2], 3, "priority_array[2]=%u", priority_array[2]);

    return;
}

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(notification_class_tests, test_Notification_Class_Ack_Required)
#else
static void test_Notification_Class_Ack_Required(void)
#endif
{
    const uint32_t instance = 1;
    bool status = false;
    uint8_t ack_required = 0;

    Notification_Class_Init();
    status = Notification_Class_Valid_Instance(instance);
    zassert_true(status, NULL);

    /* API testing */
    Notification_Class_Get_Ack_Required(BACNET_MAX_INSTANCE, &ack_required);
    zassert_equal(ack_required, 0, "ack_required=%u", ack_required);
    Notification_Class_Get_Ack_Required(instance, &ack_required);
    zassert_equal(ack_required, 0, "ack_required=%u", ack_required);

    Notification_Class_Set_Ack_Required(instance, 1);
    Notification_Class_Get_Ack_Required(instance, &ack_required);
    zassert_equal(ack_required, 1, "ack_required=%u", ack_required);

    return;
}

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(notification_class_tests, test_Notification_Class_Recipient_List)
#else
static void test_Notification_Class_Recipient_List(void)
#endif
{
    const uint32_t instance = 1;
    bool status = false;
    BACNET_LIST_ELEMENT_DATA list_element = { 0 };
    BACNET_DESTINATION destination = { 0 };
    BACNET_DESTINATION recipient_list[NC_MAX_RECIPIENTS] = { 0 };
    uint8_t apdu[MAX_APDU] = { 0 };
    int len = 0;
    int err = 0;

    Notification_Class_Init();
    status = Notification_Class_Valid_Instance(instance);
    zassert_true(status, NULL);

    /* invalid element */
    err = Notification_Class_Add_List_Element(NULL);
    zassert_equal(err, BACNET_STATUS_ABORT, NULL);
    /* valid element, invalid object property type (not a list) */
    list_element.object_type = OBJECT_NOTIFICATION_CLASS;
    list_element.object_instance = instance;
    list_element.object_property = PROP_ALL;
    list_element.array_index = BACNET_ARRAY_ALL;
    list_element.application_data = NULL;
    list_element.application_data_len = 0;
    list_element.first_failed_element_number = 0;
    err = Notification_Class_Add_List_Element(&list_element);
    zassert_equal(err, BACNET_STATUS_ERROR, NULL);
    zassert_equal(list_element.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(
        list_element.error_code, ERROR_CODE_WRITE_ACCESS_DENIED, NULL);
    /* valid element, valid property, array element (object is not an array) */
    list_element.object_property = PROP_RECIPIENT_LIST;
    list_element.array_index = 0;
    err = Notification_Class_Add_List_Element(&list_element);
    zassert_equal(err, BACNET_STATUS_ERROR, NULL);
    zassert_equal(list_element.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(
        list_element.error_code, ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY, NULL);
    /* valid element, valid property, invalid instance */
    list_element.object_property = PROP_RECIPIENT_LIST;
    list_element.object_instance = BACNET_MAX_INSTANCE;
    list_element.array_index = BACNET_ARRAY_ALL;
    err = Notification_Class_Add_List_Element(&list_element);
    zassert_equal(err, BACNET_STATUS_ERROR, NULL);
    zassert_equal(list_element.error_class, ERROR_CLASS_OBJECT, NULL);
    zassert_equal(list_element.error_code, ERROR_CODE_UNKNOWN_OBJECT, NULL);
    /* valid element, valid property, valid instance, no data */
    list_element.object_property = PROP_RECIPIENT_LIST;
    list_element.object_instance = instance;
    list_element.array_index = BACNET_ARRAY_ALL;
    list_element.application_data = NULL;
    list_element.application_data_len = 0;
    err = Notification_Class_Add_List_Element(&list_element);
    zassert_equal(err, BACNET_STATUS_OK, NULL);
    /* valid element with application data */
    for (unsigned i = 0; i < MAX_BACNET_DAYS_OF_WEEK; i++) {
        bitstring_set_bit(&destination.ValidDays, i, true);
    }
    datetime_set_time(&destination.FromTime, 0, 0, 0, 0);
    datetime_set_time(&destination.ToTime, 23, 59, 59, 99);
    destination.ProcessIdentifier = 1;
    destination.ConfirmedNotify = true;
    bacnet_recipient_device_set(&destination.Recipient, OBJECT_DEVICE, 1);
    bitstring_set_bit(&destination.Transitions, TRANSITION_TO_OFFNORMAL, true);
    bitstring_set_bit(&destination.Transitions, TRANSITION_TO_FAULT, true);
    bitstring_set_bit(&destination.Transitions, TRANSITION_TO_NORMAL, true);
    len = bacnet_destination_encode(apdu, &destination);
    list_element.application_data = apdu;
    list_element.application_data_len = len;
    err = Notification_Class_Add_List_Element(&list_element);
    zassert_equal(err, BACNET_STATUS_OK, NULL);
    /* add a second recipient */
    bacnet_recipient_device_set(&destination.Recipient, OBJECT_DEVICE, 2);
    destination.ProcessIdentifier = 2;
    err = Notification_Class_Add_List_Element(&list_element);
    zassert_equal(err, BACNET_STATUS_OK, NULL);
    /* remove a recipient */
    bacnet_recipient_device_set(&destination.Recipient, OBJECT_DEVICE, 1);
    destination.ProcessIdentifier = 1;
    err = Notification_Class_Remove_List_Element(&list_element);
    zassert_equal(err, BACNET_STATUS_OK, NULL);
    /* negative tests */
    err = Notification_Class_Remove_List_Element(NULL);
    zassert_equal(err, BACNET_STATUS_ABORT, NULL);
    /* invalid property */
    list_element.object_property = PROP_ALL;
    err = Notification_Class_Remove_List_Element(&list_element);
    zassert_equal(err, BACNET_STATUS_ERROR, NULL);
    zassert_equal(list_element.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(
        list_element.error_code, ERROR_CODE_WRITE_ACCESS_DENIED, NULL);
    /* invalid array */
    list_element.object_property = PROP_RECIPIENT_LIST;
    list_element.array_index = 0;
    err = Notification_Class_Remove_List_Element(&list_element);
    zassert_equal(err, BACNET_STATUS_ERROR, NULL);
    zassert_equal(list_element.error_class, ERROR_CLASS_PROPERTY, NULL);
    zassert_equal(
        list_element.error_code, ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY, NULL);
    /* invalid instance */
    list_element.object_property = PROP_RECIPIENT_LIST;
    list_element.array_index = BACNET_ARRAY_ALL;
    list_element.object_instance = BACNET_MAX_INSTANCE;
    err = Notification_Class_Remove_List_Element(&list_element);
    zassert_equal(err, BACNET_STATUS_ERROR, NULL);
    zassert_equal(list_element.error_class, ERROR_CLASS_OBJECT, NULL);
    zassert_equal(list_element.error_code, ERROR_CODE_UNKNOWN_OBJECT, NULL);

    status =
        Notification_Class_Get_Recipient_List(instance, &recipient_list[0]);
    zassert_true(status, NULL);
    status = Notification_Class_Get_Recipient_List(
        BACNET_MAX_INSTANCE, &recipient_list[0]);
    zassert_false(status, NULL);
    status =
        Notification_Class_Set_Recipient_List(instance, &recipient_list[0]);
    zassert_true(status, NULL);
    status = Notification_Class_Set_Recipient_List(
        BACNET_MAX_INSTANCE, &recipient_list[0]);
    zassert_false(status, NULL);

    Notification_Class_find_recipient();
}

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(notification_class_tests, test_Notification_Class_Common_Reporting)
#else
static void test_Notification_Class_Common_Reporting(void)
#endif
{
    const uint32_t instance = 1;
    bool status = false;
    BACNET_EVENT_NOTIFICATION_DATA event_data = { 0 };

    Notification_Class_Init();
    status = Notification_Class_Valid_Instance(instance);
    zassert_true(status, NULL);

    Notification_Class_common_reporting_function(&event_data);
}

/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(notification_class_tests, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        notification_class_tests,
        ztest_unit_test(test_Notification_Class_Read_Write_Property),
        ztest_unit_test(test_Notification_Class_Priority),
        ztest_unit_test(test_Notification_Class_Ack_Required),
        ztest_unit_test(test_Notification_Class_Recipient_List),
        ztest_unit_test(test_Notification_Class_Common_Reporting));

    ztest_run_test_suite(notification_class_tests);
}
#endif
