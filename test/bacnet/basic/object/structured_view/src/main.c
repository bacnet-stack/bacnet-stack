/**
 * @file
 * @brief Unit test for object
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date May 2024
 *
 * @copyright SPDX-License-Identifier: MIT
 */
#include <zephyr/ztest.h>
#include <bacnet/basic/object/structured_view.h>
#include <property_test.h>

/**
 * @addtogroup bacnet_tests
 * @{
 */

/**
 * @brief Test
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_structured_view, test_object_structured_view)
#else
static void test_object_structured_view(void)
#endif
{
    bool status = false;
    int diff = 0;
    unsigned count = 0, index = 0;
    const uint32_t instance = 123;
    const int skip_fail_property_list[] = { -1 };
    const char *test_name = "name-1234";
    const char *test_description = "description-1234";
    const char *test_node_subtype = "node-subtype-1234";
    BACNET_NODE_TYPE test_node_type = BACNET_NODE_UNKNOWN;
    BACNET_RELATIONSHIP test_relationship = BACNET_RELATIONSHIP_DEFAULT;
    BACNET_DEVICE_OBJECT_REFERENCE test_represents = {
        { OBJECT_DEVICE, 1234 }, { OBJECT_DEVICE, 1234 }
    };
    BACNET_SUBORDINATE_DATA test_subordinate_data = {
        1234,
        OBJECT_DEVICE,
        1234,
        "annotations-1234",
        BACNET_NODE_UNKNOWN,
        BACNET_RELATIONSHIP_DEFAULT,
        NULL
    };

    Structured_View_Init();
    Structured_View_Create(instance);
    status = Structured_View_Valid_Instance(instance);
    zassert_true(status, NULL);
    index = Structured_View_Instance_To_Index(instance);
    zassert_equal(index, 0, NULL);
    count = Structured_View_Count();
    zassert_true(count > 0, NULL);
    bacnet_object_properties_read_write_test(
        OBJECT_STRUCTURED_VIEW, instance, Structured_View_Property_Lists,
        Structured_View_Read_Property, NULL, skip_fail_property_list);
    bacnet_object_name_ascii_test(
        instance, Structured_View_Name_Set, Structured_View_Name_ASCII);
    /* there is no WriteProperty test for structured view - use get/set */
    status = Structured_View_Name_Set(instance, test_name);
    zassert_true(status, NULL);
    diff = strcmp(Structured_View_Name_ASCII(instance), test_name);
    zassert_equal(diff, 0, NULL);

    status = Structured_View_Description_Set(instance, test_description);
    zassert_true(status, NULL);
    diff = strcmp(Structured_View_Description(instance), test_description);
    zassert_equal(diff, 0, NULL);

    status = Structured_View_Node_Subtype_Set(instance, test_node_subtype);
    zassert_true(status, NULL);
    diff = strcmp(Structured_View_Node_Subtype(instance), test_node_subtype);
    zassert_equal(diff, 0, NULL);

    status = Structured_View_Node_Type_Set(instance, test_node_type);
    zassert_true(status, NULL);
    zassert_equal(Structured_View_Node_Type(instance), test_node_type, NULL);

    status = Structured_View_Default_Subordinate_Relationship_Set(
        instance, test_relationship);
    zassert_true(status, NULL);
    zassert_equal(
        Structured_View_Default_Subordinate_Relationship(instance),
        test_relationship, NULL);

    status = Structured_View_Represents_Set(instance, &test_represents);
    zassert_true(status, NULL);
    diff = memcmp(
        Structured_View_Represents(instance), &test_represents,
        sizeof(test_represents));
    zassert_equal(diff, 0, NULL);

    Structured_View_Subordinate_List_Set(instance, &test_subordinate_data);
    diff = memcmp(
        Structured_View_Subordinate_List(instance), &test_subordinate_data,
        sizeof(test_subordinate_data));
    zassert_equal(diff, 0, NULL);
}
/**
 * @}
 */

#if defined(CONFIG_ZTEST_NEW_API)
ZTEST_SUITE(tests_object_structured_view, NULL, NULL, NULL, NULL, NULL);
#else
void test_main(void)
{
    ztest_test_suite(
        tests_object_structured_view,
        ztest_unit_test(test_object_structured_view));

    ztest_run_test_suite(tests_object_structured_view);
}
#endif
