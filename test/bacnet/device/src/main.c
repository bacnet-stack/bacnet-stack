/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include <ztest.h>
#include <bacnet/bacstr.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h> /* for memmove */
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/bacenum.h"
#include "bacnet/bacapp.h"
#include "bacnet/config.h" /* the custom stuff */
#include "bacnet/datetime.h"
#include "bacnet/apdu.h"
#include "bacnet/wp.h" /* WriteProperty handling */
#include "bacnet/rp.h" /* ReadProperty handling */
#include "bacnet/dcc.h" /* DeviceCommunicationControl handling */
#include "bacnet/version.h"
#include "bacnet/basic/object/device.h" /* me */
#include "bacnet/basic/services.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/binding/address.h"
/* include the device object */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/acc.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/ao.h"
#include "bacnet/basic/object/av.h"
#include "bacnet/basic/object/bi.h"
#include "bacnet/basic/object/bo.h"
#include "bacnet/basic/object/bv.h"
#include "bacnet/basic/object/channel.h"
#include "bacnet/basic/object/command.h"
#include "bacnet/basic/object/csv.h"
#include "bacnet/basic/object/iv.h"
#include "bacnet/basic/object/lc.h"
#include "bacnet/basic/object/lsp.h"
#include "bacnet/basic/object/ms-input.h"
#include "bacnet/basic/object/mso.h"
#include "bacnet/basic/object/msv.h"


static void testDevice(void)
{
  const char *dev_name = "Patricia";

  zassert_true(Device_Set_Object_Instance_Number(0),
    "Device_Set_Object_Instance_Number(0) failed");
  zassert_true(Device_Object_Instance_Number()==0, 
    "Failed to set device object instance number to 0.");

  zassert_true(Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE),
    "Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE) failed");
  zassert_true(Device_Object_Instance_Number()==BACNET_MAX_INSTANCE, 
    "Failed to set device object instance number to BACNET_MAX_INSTANCE");

  zassert_true(Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE / 2), 
    "Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE / 2) failed");
  zassert_true(Device_Object_Instance_Number()==(BACNET_MAX_INSTANCE / 2), 
    "Failed to set device object instance number to BACNET_MAX_INSTANCE / 2");

  zassert_false(Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE + 1), 
    "Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE + 1) uncaught");
  zassert_false(Device_Object_Instance_Number()==(BACNET_MAX_INSTANCE + 1), 
    "Set device object instance number to illegal value BACNET_MAX_INSTANCE + 1");

  zassert_false(Device_Set_System_Status(STATUS_NON_OPERATIONAL, true),
    "Device_Set_System_Status() failed");
  zassert_true(Device_System_Status() == STATUS_NON_OPERATIONAL,
    "Failed to set device status to STATUS_NON_OPERATIONAL");

  zassert_true(Device_Vendor_Identifier() == BACNET_VENDOR_ID,
    "Incorrect BACNET_VENDOR_ID");

  zassert_true(Device_Set_Model_Name(dev_name, strlen(dev_name)),
    "Device_Set_Model_Name() failed");
  zassert_false( strcmp(Device_Model_Name(), dev_name) ,
    "Failed to set device model name");
}

void test_main(void)
{
    ztest_test_suite(device_tests,
     ztest_unit_test(testDevice)
     );

    ztest_run_test_suite(device_tests);
}
