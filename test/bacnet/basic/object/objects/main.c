/**
 * @file
 * @author Steve Karg
 * @date April 2020
 * @brief Test file for a basic BBMD for BVLC IPv4 handler
 *
 * @section LICENSE
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "bacnet/basic/sys/keylist.h"
#include "bacnet/basic/object/objects.h"
#include "ctest.h"

/* test the object creation and deletion */
void testBACnetObjectsCompare(
    Test *pTest, OBJECT_DEVICE_T *pDevice, uint32_t device_id)
{
    ct_test(pTest, pDevice != NULL);
    if (pDevice) {
        ct_test(pTest, pDevice->Object_List != NULL);
        ct_test(pTest, pDevice->Object_Identifier.instance == device_id);
        ct_test(pTest, pDevice->Object_Identifier.type == OBJECT_DEVICE);
        ct_test(pTest, pDevice->Object_Type == OBJECT_DEVICE);
    }
}

void testBACnetObjects(Test *pTest)
{
    uint32_t device_id = 0;
    unsigned test_point = 0;
    const unsigned max_test_points = 20;
    OBJECT_DEVICE_T *pDevice;
    bool status = false;

    for (test_point = 0; test_point < max_test_points; test_point++) {
        device_id = test_point * (BACNET_MAX_INSTANCE / max_test_points);
        pDevice = objects_device_new(device_id);
        testBACnetObjectsCompare(pTest, pDevice, device_id);
        pDevice = objects_device_by_instance(device_id);
        testBACnetObjectsCompare(pTest, pDevice, device_id);
    }
    ct_test(pTest, max_test_points == objects_device_count());
    for (test_point = 0; test_point < max_test_points; test_point++) {
        device_id = test_point * (BACNET_MAX_INSTANCE / max_test_points);
        pDevice = objects_device_by_instance(device_id);
        testBACnetObjectsCompare(pTest, pDevice, device_id);
    }
    for (test_point = 0; test_point < max_test_points; test_point++) {
        device_id = test_point * (BACNET_MAX_INSTANCE / max_test_points);
        pDevice = objects_device_data(test_point);
        testBACnetObjectsCompare(pTest, pDevice, objects_device_id(test_point));
    }
    for (test_point = 0; test_point < max_test_points; test_point++) {
        status = objects_device_delete(0);
        ct_test(pTest, status);
    }
}

int main(void)
{
    Test *pTest;
    bool rc;

    pTest = ct_create("BACnet Objects", NULL);

    /* individual tests */
    rc = ct_addTestFunction(pTest, testBACnetObjects);
    assert(rc);

    ct_setStream(pTest, stdout);
    ct_run(pTest);
    (void)ct_report(pTest);

    ct_destroy(pTest);

    return 0;
}
