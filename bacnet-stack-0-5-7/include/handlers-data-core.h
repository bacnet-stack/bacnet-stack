#ifndef __HANDLERS_DATA_CORE_H_
#define __HANDLERS_DATA_CORE_H_

#include "handlers-data.h"
#include "rpm.h"
#include "rp.h"
#include "wp.h"
#include "cov-core.h"
#include "getevent.h"
#include "mydata.h"
#include "device.h"

/* String Lengths - excluding any nul terminator */
#define MAX_DEV_NAME_LEN 32
#define MAX_DEV_LOC_LEN  64
#define MAX_DEV_MOD_LEN  32
#define MAX_DEV_VER_LEN  16
#define MAX_DEV_DESC_LEN 512    /*64 */

/*/ Data for the various bacnet handlers, to prevent */
/*/ multiple global static variables. */
struct bacnet_handlers_data {
    BACNET_MY_COV_SUBSCRIPTION COV_Subscriptions[MAX_COV_SUBCRIPTIONS];

    /* rpm_property_lists_function RPM_Lists[MAX_BACNET_OBJECT_TYPE]; */

    /*read_property_function Read_Property[MAX_BACNET_OBJECT_TYPE]; */

    /*object_valid_instance_function Valid_Instance[MAX_BACNET_OBJECT_TYPE]; */

    get_event_info_function Get_Event_Info[MAX_BACNET_OBJECT_TYPE];

    write_property_function Write_Property[MAX_BACNET_OBJECT_TYPE];

    DATABLOCK PT_MyData[MY_MAX_BLOCK];

    /* DEVICE --------------------------------------------------------------- */
    uint32_t DEVICE_Object_Instance_Number;     /*= 300000; */
    char DEVICE_My_Object_Name[MAX_DEV_NAME_LEN + 1];
    BACNET_DEVICE_STATUS DEVICE_System_Status;  /* = STATUS_OPERATIONAL; */
    uint16_t DEVICE_Vendor_Identifier;  /* = BACNET_VENDOR_ID; */
    char DEVICE_Model_Name[MAX_DEV_MOD_LEN + 1];        
    char DEVICE_Application_Software_Version[MAX_DEV_VER_LEN + 1];
    char DEVICE_Location[MAX_DEV_LOC_LEN + 1]; 
    char DEVICE_Description[MAX_DEV_DESC_LEN + 1];
    uint32_t DEVICE_Database_Revision;  /* = 0; */
};

#endif /*__HANDLERS_DATA_CORE_H_ */
