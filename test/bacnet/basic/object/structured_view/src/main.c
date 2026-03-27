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

static void Structured_View_Subordinate_List_Member_Same(
    BACNET_SUBORDINATE_DATA *list_member_a,
    BACNET_SUBORDINATE_DATA *list_member_b)
{
    zassert_equal(
        list_member_a->Device_Instance, list_member_b->Device_Instance, NULL);
    zassert_equal(list_member_a->Object_Type, list_member_b->Object_Type, NULL);
    zassert_equal(
        list_member_a->Object_Instance, list_member_b->Object_Instance, NULL);
    zassert_equal(list_member_a->Node_Type, list_member_b->Node_Type, NULL);
    zassert_equal(
        list_member_a->Relationship, list_member_b->Relationship, NULL);
    zassert_equal(
        strcmp(list_member_a->Annotation, list_member_b->Annotation), 0, NULL);
}

/**
 * @brief Test Structured_View_Subordinate_List_Element_Same
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_structured_view, test_subordinate_list_element_same)
#else
static void test_subordinate_list_element_same(void)
#endif
{
    bool status = false;
    BACNET_SUBORDINATE_DATA element_a = { 0,
                                          OBJECT_ANALOG_INPUT,
                                          1,
                                          "annotation-a",
                                          BACNET_NODE_MEMBER,
                                          BACNET_RELATIONSHIP_CONTAINS,
                                          NULL };
    BACNET_SUBORDINATE_DATA element_b = { 0,
                                          OBJECT_ANALOG_INPUT,
                                          1,
                                          "annotation-a",
                                          BACNET_NODE_MEMBER,
                                          BACNET_RELATIONSHIP_CONTAINS,
                                          NULL };
    BACNET_SUBORDINATE_DATA element_c = { 1,
                                          OBJECT_BINARY_INPUT,
                                          2,
                                          "annotation-c",
                                          BACNET_NODE_COLLECTION,
                                          BACNET_RELATIONSHIP_DEFAULT,
                                          NULL };

    /* identical elements */
    status =
        Structured_View_Subordinate_List_Element_Same(&element_a, &element_b);
    zassert_true(status, NULL);

    /* different elements */
    status =
        Structured_View_Subordinate_List_Element_Same(&element_a, &element_c);
    zassert_false(status, NULL);

    /* NULL element1 */
    status = Structured_View_Subordinate_List_Element_Same(NULL, &element_b);
    zassert_false(status, NULL);

    /* NULL element2 */
    status = Structured_View_Subordinate_List_Element_Same(&element_a, NULL);
    zassert_false(status, NULL);

    /* both NULL - returns true (both match "nothing") */
    status = Structured_View_Subordinate_List_Element_Same(NULL, NULL);
    zassert_true(status, NULL);

    /* NULL vs non-NULL annotation */
    element_b.Annotation = NULL;
    status =
        Structured_View_Subordinate_List_Element_Same(&element_a, &element_b);
    zassert_false(status, NULL);
}

/**
 * @brief Test Structured_View_Subordinate_List_Element_Exist,
 *   Structured_View_Subordinate_List_Element_Add, and
 *   Structured_View_Subordinate_List_Element_Remove
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(
    tests_object_structured_view,
    test_subordinate_list_element_add_remove_exist)
#else
static void test_subordinate_list_element_add_remove_exist(void)
#endif
{
    const uint32_t instance = 456;
    BACNET_ARRAY_INDEX array_index = 0;
    BACNET_SUBORDINATE_DATA element_a = {
        0,       OBJECT_ACCUMULATOR, 10,
        "watts", BACNET_NODE_MEMBER, BACNET_RELATIONSHIP_CONTAINS,
        NULL
    };
    BACNET_SUBORDINATE_DATA element_b = {
        0,        OBJECT_LOAD_CONTROL, 20,
        "demand", BACNET_NODE_MEMBER,  BACNET_RELATIONSHIP_CONTAINS,
        NULL
    };
    BACNET_SUBORDINATE_DATA element_not_added = {
        99,          OBJECT_BINARY_INPUT, 99,
        "not-added", BACNET_NODE_UNKNOWN, BACNET_RELATIONSHIP_DEFAULT,
        NULL
    };

    Structured_View_Init();
    Structured_View_Create(instance);

    /* --- Structured_View_Subordinate_List_Element_Add --- */

    /* add first element - should succeed and return index 0 */
    array_index =
        Structured_View_Subordinate_List_Element_Add(instance, &element_a);
    zassert_not_equal(array_index, BACNET_ARRAY_ALL, NULL);

    /* add second element - should succeed */
    array_index =
        Structured_View_Subordinate_List_Element_Add(instance, &element_b);
    zassert_not_equal(array_index, BACNET_ARRAY_ALL, NULL);

    /* adding a duplicate should return the existing index, not BACNET_ARRAY_ALL
     */
    array_index =
        Structured_View_Subordinate_List_Element_Add(instance, &element_a);
    zassert_equal(array_index, 0, NULL);

    /* count should be 2 after adding two unique elements */
    zassert_equal(Structured_View_Subordinate_List_Count(instance), 2, NULL);

    /* --- Structured_View_Subordinate_List_Element_Exist --- */

    /* element_a should be found at index 0 */
    array_index =
        Structured_View_Subordinate_List_Element_Exist(instance, &element_a);
    zassert_equal(array_index, 0, NULL);

    /* element_b should be found at index 1 */
    array_index =
        Structured_View_Subordinate_List_Element_Exist(instance, &element_b);
    zassert_equal(array_index, 1, NULL);

    /* element not in the list should return BACNET_ARRAY_ALL */
    array_index = Structured_View_Subordinate_List_Element_Exist(
        instance, &element_not_added);
    zassert_equal(array_index, BACNET_ARRAY_ALL, NULL);

    /* invalid instance should return BACNET_ARRAY_ALL */
    array_index = Structured_View_Subordinate_List_Element_Exist(
        instance + 1000, &element_a);
    zassert_equal(array_index, BACNET_ARRAY_ALL, NULL);

    /* --- Structured_View_Subordinate_List_Element_Remove --- */

    /* remove element_a */
    array_index =
        Structured_View_Subordinate_List_Element_Remove(instance, &element_a);
    zassert_not_equal(array_index, BACNET_ARRAY_ALL, NULL);

    /* element_a should no longer be found */
    array_index =
        Structured_View_Subordinate_List_Element_Exist(instance, &element_a);
    zassert_equal(array_index, BACNET_ARRAY_ALL, NULL);

    /* count should now be 1 */
    zassert_equal(Structured_View_Subordinate_List_Count(instance), 1, NULL);

    /* removing an element that doesn't exist should return BACNET_ARRAY_ALL */
    array_index = Structured_View_Subordinate_List_Element_Remove(
        instance, &element_not_added);
    zassert_equal(array_index, BACNET_ARRAY_ALL, NULL);

    Structured_View_Delete(instance);
    Structured_View_Cleanup();
}

/**
 * @brief Test Structured_View_Subordinate_List_Purge
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_structured_view, test_subordinate_list_purge)
#else
static void test_subordinate_list_purge(void)
#endif
{
    const uint32_t instance = 789;
    bool status = false;
    BACNET_SUBORDINATE_DATA test_subordinate_data[] = {
        { 0, OBJECT_ACCUMULATOR, 1, "watts", BACNET_NODE_MEMBER,
          BACNET_RELATIONSHIP_CONTAINS, NULL },
        { 0, OBJECT_LOAD_CONTROL, 2, "demand", BACNET_NODE_MEMBER,
          BACNET_RELATIONSHIP_CONTAINS, NULL },
        { 0, OBJECT_CHANNEL, 3, "scene", BACNET_NODE_COLLECTION,
          BACNET_RELATIONSHIP_CONTAINS, NULL },
    };

    Structured_View_Init();
    Structured_View_Create(instance);

    /* populate the list */
    Structured_View_Subordinate_List_Link_Array(
        test_subordinate_data, ARRAY_SIZE(test_subordinate_data));
    Structured_View_Subordinate_List_Set(instance, &test_subordinate_data[0]);
    zassert_equal(
        Structured_View_Subordinate_List_Count(instance),
        ARRAY_SIZE(test_subordinate_data), NULL);

    /* purge - all elements should be gone */
    status = Structured_View_Subordinate_List_Purge(instance);
    zassert_true(status, NULL);
    zassert_equal(Structured_View_Subordinate_List_Count(instance), 0, NULL);

    /* purge on invalid instance returns false */
    status = Structured_View_Subordinate_List_Purge(instance + 1000);
    zassert_false(status, NULL);

    /* re-populate after purge to confirm the list is reusable */
    Structured_View_Subordinate_List_Link_Array(
        test_subordinate_data, ARRAY_SIZE(test_subordinate_data));
    Structured_View_Subordinate_List_Set(instance, &test_subordinate_data[0]);
    zassert_equal(
        Structured_View_Subordinate_List_Count(instance),
        ARRAY_SIZE(test_subordinate_data), NULL);

    Structured_View_Delete(instance);
    Structured_View_Cleanup();
}

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
    uint32_t test_instance = 0;
    const int32_t skip_fail_property_list[] = { -1 };
    const int32_t *writable_properties;
    size_t test_size = 0;
    const char *test_name = "name-1234";
    const char *test_description = "description-1234";
    const char *test_node_subtype = "node-subtype-1234";
    char *sample_context = "context";
    BACNET_NODE_TYPE test_node_type = BACNET_NODE_UNKNOWN;
    BACNET_RELATIONSHIP test_relationship = BACNET_RELATIONSHIP_DEFAULT;
    BACNET_SUBORDINATE_DATA *test_list_member = NULL;
    BACNET_DEVICE_OBJECT_REFERENCE test_represents = {
        { OBJECT_DEVICE, 1234 }, { OBJECT_DEVICE, 1234 }
    };
    BACNET_SUBORDINATE_DATA test_subordinate_data[] = {
        { 0, OBJECT_ACCUMULATOR, 1, "watt-hours", BACNET_NODE_COLLECTION,
          BACNET_RELATIONSHIP_CONTAINS, NULL },
        { 0, OBJECT_LOAD_CONTROL, 1, "demand-response", BACNET_NODE_COLLECTION,
          BACNET_RELATIONSHIP_CONTAINS, NULL },
        { 0, OBJECT_CHANNEL, 1, "scene", BACNET_NODE_COLLECTION,
          BACNET_RELATIONSHIP_CONTAINS, NULL },
        { 0, OBJECT_LIGHTING_OUTPUT, 1, "light", BACNET_NODE_COLLECTION,
          BACNET_RELATIONSHIP_CONTAINS, NULL },
        { 0, OBJECT_BINARY_LIGHTING_OUTPUT, 1, "relay", BACNET_NODE_COLLECTION,
          BACNET_RELATIONSHIP_CONTAINS, NULL },
        { 0, OBJECT_COLOR, 1, "color", BACNET_NODE_COLLECTION,
          BACNET_RELATIONSHIP_CONTAINS, NULL },
        { 0, OBJECT_COLOR_TEMPERATURE, 1, "color-temperature",
          BACNET_NODE_COLLECTION, BACNET_RELATIONSHIP_CONTAINS, NULL },
    };

    Structured_View_Init();
    Structured_View_Create(instance);
    status = Structured_View_Valid_Instance(instance);
    zassert_true(status, NULL);
    index = Structured_View_Instance_To_Index(instance);
    zassert_equal(index, 0, NULL);
    test_instance = Structured_View_Index_To_Instance(index);
    zassert_equal(test_instance, instance, NULL);
    count = Structured_View_Count();
    zassert_true(count > 0, NULL);
    Structured_View_Subordinate_List_Link_Array(
        test_subordinate_data, ARRAY_SIZE(test_subordinate_data));
    Structured_View_Subordinate_List_Set(instance, &test_subordinate_data[0]);
    count = Structured_View_Subordinate_List_Count(instance);
    zassert_equal(count, ARRAY_SIZE(test_subordinate_data), NULL);
    for (index = 0; index < count; index++) {
        test_list_member =
            Structured_View_Subordinate_List_Member(instance, index);
        zassert_not_null(test_list_member, NULL);
        Structured_View_Subordinate_List_Member_Same(
            test_list_member, &test_subordinate_data[index]);
    }
    bacnet_object_properties_read_write_test(
        OBJECT_STRUCTURED_VIEW, instance, Structured_View_Property_Lists,
        Structured_View_Read_Property, Structured_View_Write_Property,
        skip_fail_property_list);
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

    /* WriteProperty test */
    Structured_View_Writable_Property_List(instance, &writable_properties);
    zassert_not_null(writable_properties, NULL);

    /* property specific API */
    test_list_member = Structured_View_Subordinate_List_Member(instance, 0);
    Structured_View_Subordinate_List_Member_Same(
        test_list_member, &test_subordinate_data[0]);

    /* context API */
    Structured_View_Context_Set(instance, sample_context);
    zassert_true(sample_context == Structured_View_Context_Get(instance), NULL);
    zassert_true(NULL == Structured_View_Context_Get(instance + 1), NULL);

    test_size = Structured_View_Size();
    zassert_true(test_size > 0, NULL);
    printf("Structured_View_Size: %zu bytes\n", test_size);

    Structured_View_Delete(instance);
    status = Structured_View_Valid_Instance(instance);
    zassert_false(status, NULL);

    Structured_View_Cleanup();
}
/**
 * @brief Test Structured_View_Property_Lists
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_structured_view, test_property_lists)
#else
static void test_property_lists(void)
#endif
{
    const int32_t *required = NULL;
    const int32_t *optional = NULL;
    const int32_t *proprietary = NULL;

    /* all three pointers populated */
    Structured_View_Property_Lists(&required, &optional, &proprietary);
    zassert_not_null(required, NULL);
    zassert_not_null(optional, NULL);
    zassert_not_null(proprietary, NULL);

    /* required list must contain PROP_OBJECT_IDENTIFIER (-1 terminated) */
    {
        const int32_t *p = required;
        bool found_oid = false;
        bool found_name = false;
        bool found_node_type = false;
        bool found_sublist = false;
        while (*p != -1) {
            if (*p == PROP_OBJECT_IDENTIFIER) {
                found_oid = true;
            }
            if (*p == PROP_OBJECT_NAME) {
                found_name = true;
            }
            if (*p == PROP_NODE_TYPE) {
                found_node_type = true;
            }
            if (*p == PROP_SUBORDINATE_LIST) {
                found_sublist = true;
            }
            p++;
        }
        zassert_true(found_oid, NULL);
        zassert_true(found_name, NULL);
        zassert_true(found_node_type, NULL);
        zassert_true(found_sublist, NULL);
    }

    /* NULL pointers - must not crash */
    Structured_View_Property_Lists(NULL, NULL, NULL);
    Structured_View_Property_Lists(&required, NULL, NULL);
    Structured_View_Property_Lists(NULL, &optional, NULL);
    Structured_View_Property_Lists(NULL, NULL, &proprietary);
}

/**
 * @brief Test Structured_View_Subordinate_List (raw head-pointer accessor)
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_structured_view, test_subordinate_list)
#else
static void test_subordinate_list(void)
#endif
{
    const uint32_t instance = 321;
    BACNET_SUBORDINATE_DATA *head = NULL;
    BACNET_SUBORDINATE_DATA list[] = {
        { 0, OBJECT_ANALOG_INPUT, 1, "ai-1", BACNET_NODE_MEMBER,
          BACNET_RELATIONSHIP_CONTAINS, NULL },
        { 0, OBJECT_ANALOG_OUTPUT, 2, "ao-2", BACNET_NODE_MEMBER,
          BACNET_RELATIONSHIP_CONTAINS, NULL },
    };

    Structured_View_Init();
    Structured_View_Create(instance);

    /* before population the list is empty */
    zassert_equal(Structured_View_Subordinate_List_Count(instance), 0, NULL);

    Structured_View_Subordinate_List_Link_Array(list, ARRAY_SIZE(list));
    Structured_View_Subordinate_List_Set(instance, &list[0]);

    /* Structured_View_Subordinate_List returns the head of the internal list */
    head = Structured_View_Subordinate_List(instance);
    zassert_not_null(head, NULL);
    zassert_equal(head->Object_Type, OBJECT_ANALOG_INPUT, NULL);
    zassert_equal(head->Object_Instance, 1, NULL);

    /* invalid instance returns NULL */
    zassert_is_null(Structured_View_Subordinate_List(instance + 9999), NULL);

    Structured_View_Delete(instance);
    Structured_View_Cleanup();
}

/**
 * @brief Test the four subordinate element encode functions
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_structured_view, test_subordinate_encode_functions)
#else
static void test_subordinate_encode_functions(void)
#endif
{
    const uint32_t instance = 654;
    uint8_t apdu[256];
    int len = 0;
    BACNET_SUBORDINATE_DATA list[] = {
        { 1234, OBJECT_ANALOG_INPUT, 7, "energy", BACNET_NODE_MEMBER,
          BACNET_RELATIONSHIP_CONTAINS, NULL },
        { 0, OBJECT_BINARY_INPUT, 3, "status", BACNET_NODE_COLLECTION,
          BACNET_RELATIONSHIP_DEFAULT, NULL },
    };

    Structured_View_Init();
    Structured_View_Create(instance);
    Structured_View_Subordinate_List_Link_Array(list, ARRAY_SIZE(list));
    Structured_View_Subordinate_List_Set(instance, &list[0]);

    /* --- Structured_View_Subordinate_List_Element_Encode --- */

    /* valid index 0 - should produce a positive length */
    len = Structured_View_Subordinate_List_Element_Encode(instance, 0, apdu);
    zassert_true(len > 0, NULL);

    /* valid index 1 */
    len = Structured_View_Subordinate_List_Element_Encode(instance, 1, apdu);
    zassert_true(len > 0, NULL);

    /* out-of-range index returns BACNET_STATUS_ERROR */
    len = Structured_View_Subordinate_List_Element_Encode(instance, 99, apdu);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);

    /* NULL apdu (length-only query) for index 0 */
    len = Structured_View_Subordinate_List_Element_Encode(instance, 0, NULL);
    zassert_true(len > 0, NULL);

    /* --- Structured_View_Subordinate_Annotations_Element_Encode --- */

    len = Structured_View_Subordinate_Annotations_Element_Encode(
        instance, 0, apdu);
    zassert_true(len > 0, NULL);

    len = Structured_View_Subordinate_Annotations_Element_Encode(
        instance, 1, apdu);
    zassert_true(len > 0, NULL);

    /* out-of-range */
    len = Structured_View_Subordinate_Annotations_Element_Encode(
        instance, 99, apdu);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);

    /* --- Structured_View_Subordinate_Node_Types_Element_Encode --- */

    len = Structured_View_Subordinate_Node_Types_Element_Encode(
        instance, 0, apdu);
    zassert_true(len > 0, NULL);

    len = Structured_View_Subordinate_Node_Types_Element_Encode(
        instance, 1, apdu);
    zassert_true(len > 0, NULL);

    /* out-of-range */
    len = Structured_View_Subordinate_Node_Types_Element_Encode(
        instance, 99, apdu);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);

    /* --- Structured_View_Subordinate_Relationships_Element_Encode --- */

    len = Structured_View_Subordinate_Relationships_Element_Encode(
        instance, 0, apdu);
    zassert_true(len > 0, NULL);

    len = Structured_View_Subordinate_Relationships_Element_Encode(
        instance, 1, apdu);
    zassert_true(len > 0, NULL);

    /* out-of-range */
    len = Structured_View_Subordinate_Relationships_Element_Encode(
        instance, 99, apdu);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);

    Structured_View_Delete(instance);
    Structured_View_Cleanup();
}

/**
 * @brief Test encode functions with invalid instance
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_structured_view, test_encode_functions_invalid_instance)
#else
static void test_encode_functions_invalid_instance(void)
#endif
{
    const uint32_t invalid_instance = 9999;
    uint8_t apdu[256];
    int len = 0;

    Structured_View_Init();

    /* All encode functions should return BACNET_STATUS_ERROR for invalid
     * instance */

    /* --- Subordinate_List_Element_Encode --- */
    len = Structured_View_Subordinate_List_Element_Encode(
        invalid_instance, 0, apdu);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);

    len = Structured_View_Subordinate_List_Element_Encode(
        invalid_instance, 0, NULL);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);

    /* --- Subordinate_Annotations_Element_Encode --- */
    len = Structured_View_Subordinate_Annotations_Element_Encode(
        invalid_instance, 0, apdu);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);

    len = Structured_View_Subordinate_Annotations_Element_Encode(
        invalid_instance, 0, NULL);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);

    /* --- Subordinate_Node_Types_Element_Encode --- */
    len = Structured_View_Subordinate_Node_Types_Element_Encode(
        invalid_instance, 0, apdu);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);

    len = Structured_View_Subordinate_Node_Types_Element_Encode(
        invalid_instance, 0, NULL);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);

    /* --- Subordinate_Relationships_Element_Encode --- */
    len = Structured_View_Subordinate_Relationships_Element_Encode(
        invalid_instance, 0, apdu);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);

    len = Structured_View_Subordinate_Relationships_Element_Encode(
        invalid_instance, 0, NULL);
    zassert_equal(len, BACNET_STATUS_ERROR, NULL);

    Structured_View_Cleanup();
}

/**
 * @brief Test NULL annotation handling in subordinate list elements
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_structured_view, test_subordinate_null_annotations)
#else
static void test_subordinate_null_annotations(void)
#endif
{
    const uint32_t instance = 555;
    BACNET_ARRAY_INDEX array_index = 0;
    BACNET_SUBORDINATE_DATA element_with_annotation = {
        0,
        OBJECT_ACCUMULATOR,
        10,
        "has-annotation",
        BACNET_NODE_MEMBER,
        BACNET_RELATIONSHIP_CONTAINS,
        NULL
    };
    BACNET_SUBORDINATE_DATA element_null_annotation = {
        0,    OBJECT_ACCUMULATOR, 10,
        NULL, BACNET_NODE_MEMBER, BACNET_RELATIONSHIP_CONTAINS,
        NULL
    };
    BACNET_SUBORDINATE_DATA *test_member = NULL;

    Structured_View_Init();
    Structured_View_Create(instance);

    /* Add element with annotation */
    array_index = Structured_View_Subordinate_List_Element_Add(
        instance, &element_with_annotation);
    zassert_not_equal(array_index, BACNET_ARRAY_ALL, NULL);

    /* Retrieve and verify annotation is preserved */
    test_member = Structured_View_Subordinate_List_Member(instance, 0);
    zassert_not_null(test_member, NULL);
    zassert_not_null(test_member->Annotation, NULL);

    /* Add element with NULL annotation */
    array_index = Structured_View_Subordinate_List_Element_Add(
        instance, &element_null_annotation);
    /* Should succeed with different annotation */
    zassert_not_equal(array_index, BACNET_ARRAY_ALL, NULL);

    /* Verify NULL annotation is preserved */
    test_member = Structured_View_Subordinate_List_Member(instance, 1);
    zassert_not_null(test_member, NULL);
    zassert_is_null(test_member->Annotation, NULL);

    zassert_equal(Structured_View_Subordinate_List_Count(instance), 2, NULL);

    Structured_View_Delete(instance);
    Structured_View_Cleanup();
}

/**
 * @brief Test boundary conditions for subordinate list operations
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_structured_view, test_subordinate_list_boundaries)
#else
static void test_subordinate_list_boundaries(void)
#endif
{
    const uint32_t instance = 888;
    BACNET_ARRAY_INDEX array_index = 0;
    BACNET_SUBORDINATE_DATA element = {
        0,      OBJECT_ACCUMULATOR, 1,
        "test", BACNET_NODE_MEMBER, BACNET_RELATIONSHIP_CONTAINS,
        NULL
    };
    BACNET_SUBORDINATE_DATA *test_member = NULL;

    Structured_View_Init();
    Structured_View_Create(instance);

    /* Test accessing beyond list boundaries */
    test_member = Structured_View_Subordinate_List_Member(instance, 0);
    zassert_is_null(test_member, NULL); /* Empty list */

    test_member = Structured_View_Subordinate_List_Member(instance, 100);
    zassert_is_null(test_member, NULL); /* Out of bounds */

    /* Add one element */
    array_index =
        Structured_View_Subordinate_List_Element_Add(instance, &element);
    zassert_not_equal(array_index, BACNET_ARRAY_ALL, NULL);

    /* Access valid index */
    test_member = Structured_View_Subordinate_List_Member(instance, 0);
    zassert_not_null(test_member, NULL);

    /* Access index just beyond valid range */
    test_member = Structured_View_Subordinate_List_Member(instance, 1);
    zassert_is_null(test_member, NULL);

    /* Test count after operations */
    zassert_equal(Structured_View_Subordinate_List_Count(instance), 1, NULL);

    /* Remove element and verify count */
    array_index =
        Structured_View_Subordinate_List_Element_Remove(instance, &element);
    zassert_not_equal(array_index, BACNET_STATUS_ERROR, NULL);
    zassert_equal(Structured_View_Subordinate_List_Count(instance), 0, NULL);

    Structured_View_Delete(instance);
    Structured_View_Cleanup();
}

/**
 * @brief Test sequential add-remove-add cycles
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_structured_view, test_sequential_add_remove_cycles)
#else
static void test_sequential_add_remove_cycles(void)
#endif
{
    const uint32_t instance = 777;
    BACNET_ARRAY_INDEX array_index = 0;
    BACNET_SUBORDINATE_DATA element1 = {
        0,           OBJECT_ACCUMULATOR, 1,
        "element-1", BACNET_NODE_MEMBER, BACNET_RELATIONSHIP_CONTAINS,
        NULL
    };
    BACNET_SUBORDINATE_DATA element2 = {
        0,           OBJECT_LOAD_CONTROL, 2,
        "element-2", BACNET_NODE_MEMBER,  BACNET_RELATIONSHIP_CONTAINS,
        NULL
    };

    Structured_View_Init();
    Structured_View_Create(instance);

    /* Add element (should succeed) */
    array_index =
        Structured_View_Subordinate_List_Element_Add(instance, &element1);
    zassert_not_equal(array_index, BACNET_ARRAY_ALL, NULL);
    unsigned count1 = Structured_View_Subordinate_List_Count(instance);
    zassert_equal(count1, 1, NULL);

    /* Remove element (should succeed) */
    array_index =
        Structured_View_Subordinate_List_Element_Remove(instance, &element1);
    zassert_not_equal(array_index, BACNET_ARRAY_ALL, NULL);

    /* Add another element */
    array_index =
        Structured_View_Subordinate_List_Element_Add(instance, &element2);
    zassert_not_equal(array_index, BACNET_ARRAY_ALL, NULL);
    unsigned count2 = Structured_View_Subordinate_List_Count(instance);
    zassert_equal(count2, 1, NULL);

    Structured_View_Delete(instance);
    Structured_View_Cleanup();
}

/**
 * @brief Test public API functions handle invalid instances gracefully
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_structured_view, test_invalid_instance_validation)
#else
static void test_invalid_instance_validation(void)
#endif
{
    const uint32_t invalid_instance = 5000;
    char test_string[] = "test";
    BACNET_DEVICE_OBJECT_REFERENCE test_ref = { { OBJECT_DEVICE, 1 },
                                                { OBJECT_DEVICE, 1 } };

    Structured_View_Init();

    /* Valid instance check should return false for invalid instance */
    bool is_valid = Structured_View_Valid_Instance(invalid_instance);
    zassert_false(is_valid, NULL);

    /* Operations on invalid instance should handle gracefully (not crash) */
    /* These calls should not crash or cause undefined behavior */
    (void)Structured_View_Subordinate_List_Count(invalid_instance);
    (void)Structured_View_Subordinate_List_Member(invalid_instance, 0);
    (void)Structured_View_Name_ASCII(invalid_instance);
    (void)Structured_View_Description(invalid_instance);
    (void)Structured_View_Node_Subtype(invalid_instance);
    (void)Structured_View_Node_Type(invalid_instance);
    (void)Structured_View_Default_Subordinate_Relationship(invalid_instance);
    (void)Structured_View_Represents(invalid_instance);
    (void)Structured_View_Context_Get(invalid_instance);

    /* Write operations should fail for invalid instance */
    bool result = Structured_View_Name_Set(invalid_instance, test_string);
    zassert_false(result, NULL);

    result = Structured_View_Description_Set(invalid_instance, test_string);
    zassert_false(result, NULL);

    result = Structured_View_Node_Subtype_Set(invalid_instance, test_string);
    zassert_false(result, NULL);

    result =
        Structured_View_Node_Type_Set(invalid_instance, BACNET_NODE_MEMBER);
    zassert_false(result, NULL);

    result = Structured_View_Default_Subordinate_Relationship_Set(
        invalid_instance, BACNET_RELATIONSHIP_CONTAINS);
    zassert_false(result, NULL);

    result = Structured_View_Represents_Set(invalid_instance, &test_ref);
    zassert_false(result, NULL);

    result = Structured_View_Subordinate_List_Purge(invalid_instance);
    zassert_false(result, NULL);

    Structured_View_Cleanup();
}

/**
 * @brief Test state consistency after purge and re-populate
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_structured_view, test_state_consistency_after_purge)
#else
static void test_state_consistency_after_purge(void)
#endif
{
    const uint32_t instance = 999;
    bool status = false;
    BACNET_ARRAY_INDEX array_index = 0;
    BACNET_SUBORDINATE_DATA test_data[] = {
        { 0, OBJECT_ACCUMULATOR, 1, "acc-1", BACNET_NODE_MEMBER,
          BACNET_RELATIONSHIP_CONTAINS, NULL },
        { 0, OBJECT_LOAD_CONTROL, 2, "lc-2", BACNET_NODE_MEMBER,
          BACNET_RELATIONSHIP_CONTAINS, NULL },
        { 0, OBJECT_CHANNEL, 3, "ch-3", BACNET_NODE_COLLECTION,
          BACNET_RELATIONSHIP_CONTAINS, NULL },
    };

    Structured_View_Init();
    Structured_View_Create(instance);

    /* Populate list */
    Structured_View_Subordinate_List_Link_Array(
        test_data, ARRAY_SIZE(test_data));
    Structured_View_Subordinate_List_Set(instance, &test_data[0]);
    zassert_equal(
        Structured_View_Subordinate_List_Count(instance), ARRAY_SIZE(test_data),
        NULL);

    /* Verify element exists */
    array_index =
        Structured_View_Subordinate_List_Element_Exist(instance, &test_data[0]);
    zassert_equal(array_index, 0, NULL);

    /* Purge list */
    status = Structured_View_Subordinate_List_Purge(instance);
    zassert_true(status, NULL);
    zassert_equal(Structured_View_Subordinate_List_Count(instance), 0, NULL);

    /* Verify element no longer exists after purge */
    array_index =
        Structured_View_Subordinate_List_Element_Exist(instance, &test_data[0]);
    zassert_equal(array_index, BACNET_ARRAY_ALL, NULL);

    /* Re-populate with same data */
    Structured_View_Subordinate_List_Link_Array(
        test_data, ARRAY_SIZE(test_data));
    Structured_View_Subordinate_List_Set(instance, &test_data[0]);
    zassert_equal(
        Structured_View_Subordinate_List_Count(instance), ARRAY_SIZE(test_data),
        NULL);

    /* Verify elements are found again */
    array_index =
        Structured_View_Subordinate_List_Element_Exist(instance, &test_data[0]);
    zassert_equal(array_index, 0, NULL);

    array_index =
        Structured_View_Subordinate_List_Element_Exist(instance, &test_data[1]);
    zassert_equal(array_index, 1, NULL);

    /* Purge again */
    status = Structured_View_Subordinate_List_Purge(instance);
    zassert_true(status, NULL);
    zassert_equal(Structured_View_Subordinate_List_Count(instance), 0, NULL);

    Structured_View_Delete(instance);
    Structured_View_Cleanup();
}

/**
 * @brief Test Structured_View_Subordinate_List_Resize via Write_Property
 * array_index==0 on PROP_SUBORDINATE_LIST and related array properties.
 */
#if defined(CONFIG_ZTEST_NEW_API)
ZTEST(tests_object_structured_view, test_subordinate_list_resize)
#else
static void test_subordinate_list_resize(void)
#endif
{
    const uint32_t instance = 1111;
    bool status = false;
    BACNET_WRITE_PROPERTY_DATA wp_data = { 0 };

    Structured_View_Init();
    Structured_View_Create(instance);

    wp_data.object_type = OBJECT_STRUCTURED_VIEW;
    wp_data.object_instance = instance;
    wp_data.priority = BACNET_NO_PRIORITY;
    wp_data.array_index = 0; /* index 0 means "write the array size" */

    /* --- grow from 0 to 3 via PROP_SUBORDINATE_LIST --- */
    wp_data.object_property = PROP_SUBORDINATE_LIST;
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, 3);
    status = Structured_View_Write_Property(&wp_data);
    zassert_true(status, NULL);
    zassert_equal(Structured_View_Subordinate_List_Count(instance), 3, NULL);

    /* --- grow from 3 to 5 --- */
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, 5);
    status = Structured_View_Write_Property(&wp_data);
    zassert_true(status, NULL);
    zassert_equal(Structured_View_Subordinate_List_Count(instance), 5, NULL);

    /* --- same size (no change) --- */
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, 5);
    status = Structured_View_Write_Property(&wp_data);
    zassert_true(status, NULL);
    zassert_equal(Structured_View_Subordinate_List_Count(instance), 5, NULL);

    /* --- shrink from 5 to 2 --- */
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, 2);
    status = Structured_View_Write_Property(&wp_data);
    zassert_true(status, NULL);
    zassert_equal(Structured_View_Subordinate_List_Count(instance), 2, NULL);

    /* --- shrink to zero --- */
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, 0);
    status = Structured_View_Write_Property(&wp_data);
    zassert_true(status, NULL);
    zassert_equal(Structured_View_Subordinate_List_Count(instance), 0, NULL);

    /* --- resize via PROP_SUBORDINATE_ANNOTATIONS independently --- */
    /* first grow the list so the annotation resize has something to work with
     */
    wp_data.object_property = PROP_SUBORDINATE_LIST;
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, 4);
    status = Structured_View_Write_Property(&wp_data);
    zassert_true(status, NULL);

    wp_data.object_property = PROP_SUBORDINATE_ANNOTATIONS;
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, 2);
    status = Structured_View_Write_Property(&wp_data);
    zassert_true(status, NULL);
    zassert_equal(Structured_View_Subordinate_List_Count(instance), 2, NULL);

    /* --- resize via PROP_SUBORDINATE_NODE_TYPES --- */
    wp_data.object_property = PROP_SUBORDINATE_NODE_TYPES;
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, 3);
    status = Structured_View_Write_Property(&wp_data);
    zassert_true(status, NULL);
    zassert_equal(Structured_View_Subordinate_List_Count(instance), 3, NULL);

    /* --- resize via PROP_SUBORDINATE_RELATIONSHIPS --- */
    wp_data.object_property = PROP_SUBORDINATE_RELATIONSHIPS;
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, 1);
    status = Structured_View_Write_Property(&wp_data);
    zassert_true(status, NULL);
    zassert_equal(Structured_View_Subordinate_List_Count(instance), 1, NULL);

    /* --- invalid instance returns false --- */
    wp_data.object_instance = instance + 9999;
    wp_data.object_property = PROP_SUBORDINATE_LIST;
    wp_data.application_data_len =
        encode_application_unsigned(wp_data.application_data, 2);
    status = Structured_View_Write_Property(&wp_data);
    zassert_false(status, NULL);

    Structured_View_Delete(instance);
    Structured_View_Cleanup();
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
        ztest_unit_test(test_object_structured_view),
        ztest_unit_test(test_subordinate_list_element_same),
        ztest_unit_test(test_subordinate_list_element_add_remove_exist),
        ztest_unit_test(test_subordinate_list_purge),
        ztest_unit_test(test_property_lists),
        ztest_unit_test(test_subordinate_list),
        ztest_unit_test(test_subordinate_encode_functions),
        ztest_unit_test(test_encode_functions_invalid_instance),
        ztest_unit_test(test_subordinate_null_annotations),
        ztest_unit_test(test_subordinate_list_boundaries),
        ztest_unit_test(test_sequential_add_remove_cycles),
        ztest_unit_test(test_invalid_instance_validation),
        ztest_unit_test(test_state_consistency_after_purge),
        ztest_unit_test(test_subordinate_list_resize));

    ztest_run_test_suite(tests_object_structured_view);
}
#endif
