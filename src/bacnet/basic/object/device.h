/**************************************************************************
*
* Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
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
*
*********************************************************************/

/** @file device.h Defines functions for handling all BACnet objects belonging
 *                 to a BACnet device, as well as Device-specific properties. */

#ifndef DEVICE_H
#define DEVICE_H

#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacenum.h"
#include "bacnet/wp.h"
#include "bacnet/rd.h"
#include "bacnet/rp.h"
#include "bacnet/rpm.h"
#include "bacnet/readrange.h"

/** Called so a BACnet object can perform any necessary initialization.
 * @ingroup ObjHelpers
 */
typedef void (
    *object_init_function) (
    void);

/** Counts the number of objects of this type.
 * @ingroup ObjHelpers
 * @return Count of implemented objects of this type.
 */
typedef unsigned (
    *object_count_function) (
    void);

/** Maps an object index position to its corresponding BACnet object instance number.
 * @ingroup ObjHelpers
 * @param index [in] The index of the object, in the array of objects of its type.
 * @return The BACnet object instance number to be used in a BACNET_OBJECT_ID.
 */
typedef uint32_t(
    *object_index_to_instance_function)
        (
    unsigned index);

/** Provides the BACnet Object_Name for a given object instance of this type.
 * @ingroup ObjHelpers
 * @param object_instance [in] The object instance number to be looked up.
 * @param object_name [in,out] Pointer to a character_string structure that
 *         will hold a copy of the object name if this is a valid object_instance.
 * @return True if the object_instance is valid and object_name has been
 *         filled with a copy of the Object's name.
 */
typedef bool(
    *object_name_function)
        (
    uint32_t object_instance,
    BACNET_CHARACTER_STRING * object_name);

/** Look in the table of objects of this type, and see if this is a valid
 *  instance number.
 * @ingroup ObjHelpers
 * @param [in] The object instance number to be looked up.
 * @return True if the object instance refers to a valid object of this type.
 */
typedef bool(
    *object_valid_instance_function) (
    uint32_t object_instance);

/** Helper function to step through an array of objects and find either the
 * first one or the next one of a given type. Used to step through an array
 * of objects which is not necessarily contiguious for each type i.e. the
 * index for the 'n'th object of a given type is not necessarily 'n'.
 * @ingroup ObjHelpers
 * @param [in] The index of the current object or a value of ~0 to indicate
 * start at the beginning.
 * @return The index of the next object of the required type or ~0 (all bits
 * == 1) to indicate no more objects found.
 */
typedef unsigned (
    *object_iterate_function) (
    unsigned current_index);

/** Look in the table of objects of this type, and get the COV Value List.
 * @ingroup ObjHelpers
 * @param [in] The object instance number to be looked up.
 * @param [out] The value list
 * @return True if the object instance supports this feature, and has changed.
 */
typedef bool(
    *object_value_list_function) (
    uint32_t object_instance,
    BACNET_PROPERTY_VALUE * value_list);

/** Look in the table of objects for this instance to see if value changed.
 * @ingroup ObjHelpers
 * @param [in] The object instance number to be looked up.
 * @return True if the object instance has changed.
 */
typedef bool(
    *object_cov_function) (
    uint32_t object_instance);

/** Look in the table of objects for this instance to clear the changed flag.
 * @ingroup ObjHelpers
 * @param [in] The object instance number to be looked up.
 */
typedef void (
    *object_cov_clear_function) (
    uint32_t object_instance);

/** Intrinsic Reporting funcionality.
 * @ingroup ObjHelpers
 * @param [in] Object instance.
 */
typedef void (
    *object_intrinsic_reporting_function) (
    uint32_t object_instance);


/** Defines the group of object helper functions for any supported Object.
 * @ingroup ObjHelpers
 * Each Object must provide some implementation of each of these helpers
 * in order to properly support the handlers.  Eg, the ReadProperty handler
 * handler_read_property() relies on the instance of Object_Read_Property
 * for each Object type, or configure the function as NULL.
 * In both appearance and operation, this group of functions acts like
 * they are member functions of a C++ Object base class.
 */
typedef struct object_functions {
    BACNET_OBJECT_TYPE Object_Type;
    object_init_function Object_Init;
    object_count_function Object_Count;
    object_index_to_instance_function Object_Index_To_Instance;
    object_valid_instance_function Object_Valid_Instance;
    object_name_function Object_Name;
    read_property_function Object_Read_Property;
    write_property_function Object_Write_Property;
    rpm_property_lists_function Object_RPM_List;
    rr_info_function Object_RR_Info;
    object_iterate_function Object_Iterator;
    object_value_list_function Object_Value_List;
    object_cov_function Object_COV;
    object_cov_clear_function Object_COV_Clear;
    object_intrinsic_reporting_function Object_Intrinsic_Reporting;
} object_functions_t;

/* String Lengths - excluding any nul terminator */
#define MAX_DEV_NAME_LEN 32
#define MAX_DEV_LOC_LEN  64
#define MAX_DEV_MOD_LEN  32
#define MAX_DEV_VER_LEN  16
#define MAX_DEV_DESC_LEN 64

/** Structure to define the Object Properties common to all Objects. */
typedef struct commonBacObj_s {

    /** The BACnet type of this object (ie, what class is this object from?).
     * This property, of type BACnetObjectType, indicates membership in a
     * particular object type class. Each inherited class will be of one type.
     */
    BACNET_OBJECT_TYPE mObject_Type;

    /** The instance number for this class instance. */
    uint32_t Object_Instance_Number;

    /** Object Name; must be unique.
     * This property, of type CharacterString, shall represent a name for
     * the object that is unique within the BACnet Device that maintains it.
     */
    char Object_Name[MAX_DEV_NAME_LEN];

} COMMON_BAC_OBJECT;


/** Structure to define the Properties of Device Objects which distinguish
 *  one instance from another.
 *  This structure only defines fields for properties that are unique to
 *  a given Device object.  The rest may be fixed in device.c or hard-coded
 *  into the read-property encoding.
 *  This may be useful for implementations which manage multiple Devices,
 *  eg, a Gateway.
 */
typedef struct devObj_s {
    /** The BACnet Device Address for this device; ->len depends on DLL type. */
    BACNET_ADDRESS bacDevAddr;

    /** Structure for the Object Properties common to all Objects. */
    COMMON_BAC_OBJECT bacObj;

    /** Device Description. */
    char Description[MAX_DEV_DESC_LEN];

    /** The upcounter that shows if the Device ID or object structure has changed. */
    uint32_t Database_Revision;
} DEVICE_OBJECT_DATA;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    BACNET_STACK_EXPORT
    void Device_Init(
        object_functions_t * object_table);

    BACNET_STACK_EXPORT
    bool Device_Reinitialize(
        BACNET_REINITIALIZE_DEVICE_DATA * rd_data);
    BACNET_STACK_EXPORT
    bool Device_Reinitialize_State_Set(BACNET_REINITIALIZED_STATE state);
    BACNET_STACK_EXPORT
    BACNET_REINITIALIZED_STATE Device_Reinitialized_State(
        void);

    BACNET_STACK_EXPORT
    rr_info_function Device_Objects_RR_Info(
        BACNET_OBJECT_TYPE object_type);

    BACNET_STACK_EXPORT
    void Device_getCurrentDateTime(
        BACNET_DATE_TIME * DateTime);

    BACNET_STACK_EXPORT
    int32_t Device_UTC_Offset(void);
    BACNET_STACK_EXPORT
    void Device_UTC_Offset_Set(int16_t offset);

    BACNET_STACK_EXPORT
    bool Device_Daylight_Savings_Status(void);
    bool Device_Align_Intervals(void);
    BACNET_STACK_EXPORT
    bool Device_Align_Intervals_Set(bool flag);
    BACNET_STACK_EXPORT
    uint32_t Device_Time_Sync_Interval(void);
    BACNET_STACK_EXPORT
    bool Device_Time_Sync_Interval_Set(uint32_t value);
    BACNET_STACK_EXPORT
    uint32_t Device_Interval_Offset(void);
    BACNET_STACK_EXPORT
    bool Device_Interval_Offset_Set(uint32_t value);

    BACNET_STACK_EXPORT
    void Device_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    BACNET_STACK_EXPORT
    void Device_Objects_Property_List(
        BACNET_OBJECT_TYPE object_type,
        uint32_t object_instance,
        struct special_property_list_t *pPropertyList);
    /* functions to support COV */
    BACNET_STACK_EXPORT
    bool Device_Encode_Value_List(
        BACNET_OBJECT_TYPE object_type,
        uint32_t object_instance,
        BACNET_PROPERTY_VALUE * value_list);
    bool Device_Value_List_Supported(
        BACNET_OBJECT_TYPE object_type);
    BACNET_STACK_EXPORT
    bool Device_COV(
        BACNET_OBJECT_TYPE object_type,
        uint32_t object_instance);
    BACNET_STACK_EXPORT
    void Device_COV_Clear(
        BACNET_OBJECT_TYPE object_type,
        uint32_t object_instance);

    BACNET_STACK_EXPORT
    uint32_t Device_Object_Instance_Number(
        void);
    BACNET_STACK_EXPORT
    bool Device_Set_Object_Instance_Number(
        uint32_t object_id);
    BACNET_STACK_EXPORT
    bool Device_Valid_Object_Instance_Number(
        uint32_t object_id);
    BACNET_STACK_EXPORT
    unsigned Device_Object_List_Count(
        void);
    BACNET_STACK_EXPORT
    bool Device_Object_List_Identifier(
        uint32_t array_index,
        BACNET_OBJECT_TYPE *object_type,
        uint32_t * instance);

    BACNET_STACK_EXPORT
    unsigned Device_Count(
        void);
    BACNET_STACK_EXPORT
    uint32_t Device_Index_To_Instance(
        unsigned index);

    BACNET_STACK_EXPORT
    bool Device_Object_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    BACNET_STACK_EXPORT
    bool Device_Set_Object_Name(
        BACNET_CHARACTER_STRING * object_name);
    /* Copy a child object name, given its ID. */
    BACNET_STACK_EXPORT
    bool Device_Object_Name_Copy(
        BACNET_OBJECT_TYPE object_type,
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    BACNET_STACK_EXPORT
    bool Device_Object_Name_ANSI_Init(const char * object_name);
    BACNET_STACK_EXPORT
    char * Device_Object_Name_ANSI(void);

    BACNET_STACK_EXPORT
    BACNET_DEVICE_STATUS Device_System_Status(
        void);
    BACNET_STACK_EXPORT
    int Device_Set_System_Status(
        BACNET_DEVICE_STATUS status,
        bool local);

    BACNET_STACK_EXPORT
    const char *Device_Vendor_Name(
        void);

    BACNET_STACK_EXPORT
    uint16_t Device_Vendor_Identifier(
        void);
    BACNET_STACK_EXPORT
    void Device_Set_Vendor_Identifier(
        uint16_t vendor_id);

    BACNET_STACK_EXPORT
    const char *Device_Model_Name(
        void);
    BACNET_STACK_EXPORT
    bool Device_Set_Model_Name(
        const char *name,
        size_t length);

    BACNET_STACK_EXPORT
    const char *Device_Firmware_Revision(
        void);

    BACNET_STACK_EXPORT
    const char *Device_Application_Software_Version(
        void);
    BACNET_STACK_EXPORT
    bool Device_Set_Application_Software_Version(
        const char *name,
        size_t length);

    BACNET_STACK_EXPORT
    const char *Device_Description(
        void);
    BACNET_STACK_EXPORT
    bool Device_Set_Description(
        const char *name,
        size_t length);

    BACNET_STACK_EXPORT
    const char *Device_Location(
        void);
    BACNET_STACK_EXPORT
    bool Device_Set_Location(
        const char *name,
        size_t length);

    /* some stack-centric constant values - no set methods */
    BACNET_STACK_EXPORT
    uint8_t Device_Protocol_Version(
        void);
    BACNET_STACK_EXPORT
    uint8_t Device_Protocol_Revision(
        void);
    BACNET_STACK_EXPORT
    BACNET_SEGMENTATION Device_Segmentation_Supported(
        void);

    BACNET_STACK_EXPORT
    uint32_t Device_Database_Revision(
        void);
    BACNET_STACK_EXPORT
    void Device_Set_Database_Revision(
        uint32_t revision);
    BACNET_STACK_EXPORT
    void Device_Inc_Database_Revision(
        void);

    BACNET_STACK_EXPORT
    bool Device_Valid_Object_Name(
        BACNET_CHARACTER_STRING * object_name,
        BACNET_OBJECT_TYPE *object_type,
        uint32_t * object_instance);
    BACNET_STACK_EXPORT
    bool Device_Valid_Object_Id(
        BACNET_OBJECT_TYPE object_type,
        uint32_t object_instance);

    BACNET_STACK_EXPORT
    int Device_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);
    BACNET_STACK_EXPORT
    bool Device_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    BACNET_STACK_EXPORT
    bool DeviceGetRRInfo(
        BACNET_READ_RANGE_DATA * pRequest,      /* Info on the request */
        RR_PROP_INFO * pInfo);  /* Where to put the information */

    BACNET_STACK_EXPORT
    int Device_Read_Property_Local(
        BACNET_READ_PROPERTY_DATA * rpdata);
    BACNET_STACK_EXPORT
    bool Device_Write_Property_Local(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

#if defined(INTRINSIC_REPORTING)
    BACNET_STACK_EXPORT
    void Device_local_reporting(
        void);
#endif

/* Prototypes for Routing functionality in the Device Object.
 * Enable by defining BAC_ROUTING in config.h and including gw_device.c
 * in the build (lib/Makefile).
 */
    BACNET_STACK_EXPORT
    void Routing_Device_Init(
        uint32_t first_object_instance);

    BACNET_STACK_EXPORT
    uint16_t Add_Routed_Device(
        uint32_t Object_Instance,
        BACNET_CHARACTER_STRING * Object_Name,
        const char *Description);
    BACNET_STACK_EXPORT
    DEVICE_OBJECT_DATA *Get_Routed_Device_Object(
        int idx);
    BACNET_STACK_EXPORT
    BACNET_ADDRESS *Get_Routed_Device_Address(
        int idx);

    BACNET_STACK_EXPORT
    void routed_get_my_address(
        BACNET_ADDRESS * my_address);

    BACNET_STACK_EXPORT
    bool Routed_Device_Address_Lookup(
        int idx,
        uint8_t address_len,
        uint8_t * mac_adress);
    BACNET_STACK_EXPORT
    bool Routed_Device_GetNext(
        BACNET_ADDRESS * dest,
        int *DNET_list,
        int *cursor);
    BACNET_STACK_EXPORT
    bool Routed_Device_Is_Valid_Network(
        uint16_t dest_net,
        int *DNET_list);

    BACNET_STACK_EXPORT
    uint32_t Routed_Device_Index_To_Instance(
        unsigned index);
    BACNET_STACK_EXPORT
    bool Routed_Device_Valid_Object_Instance_Number(
        uint32_t object_id);
    BACNET_STACK_EXPORT
    bool Routed_Device_Name(
        uint32_t object_instance,
        BACNET_CHARACTER_STRING * object_name);
    BACNET_STACK_EXPORT
    uint32_t Routed_Device_Object_Instance_Number(
        void);
    BACNET_STACK_EXPORT
    bool Routed_Device_Set_Object_Instance_Number(
        uint32_t object_id);
    BACNET_STACK_EXPORT
    bool Routed_Device_Set_Object_Name(
        uint8_t encoding,
        const char *value,
        size_t length);
    BACNET_STACK_EXPORT
    bool Routed_Device_Set_Description(
        const char *name,
        size_t length);
    BACNET_STACK_EXPORT
    void Routed_Device_Inc_Database_Revision(
        void);
    BACNET_STACK_EXPORT
    int Routed_Device_Service_Approval(
        BACNET_CONFIRMED_SERVICE service,
        int service_argument,
        uint8_t * apdu_buff,
        uint8_t invoke_id);



#ifdef __cplusplus
}
#endif /* __cplusplus */
/** @defgroup ObjFrmwk Object Framework
 * The modules in this section describe the BACnet-stack's framework for
 * BACnet-defined Objects (Device, Analog Input, etc). There are two submodules
 * to describe this arrangement:
 *  - The "object helper functions" which provide C++-like common functionality
 *    to all supported object types.
 *  - The interface between the implemented Objects and the BAC-stack services,
 *    specifically the handlers, which are mediated through function calls to
 *    the Device object.
    *//** @defgroup ObjHelpers Object Helper Functions
 * @ingroup ObjFrmwk
 * This section describes the function templates for the helper functions that
 * provide common object support.
    *//** @defgroup ObjIntf Handler-to-Object Interface Functions
 * @ingroup ObjFrmwk
 * This section describes the fairly limited set of functions that link the
 * BAC-stack handlers to the BACnet Object instances.  All of these calls are
 * situated in the Device Object, which "knows" how to reach its child Objects.
 *
 * Most of these calls have a common operation:
 *  -# Call Device_Objects_Find_Functions( for the desired Object_Type )
 *   - Gets a pointer to the object_functions for this Type of Object.
 *  -# Call the Object's Object_Valid_Instance( for the desired object_instance )
 *     to make sure there is such an instance.
 *  -# Call the Object helper function needed by the handler,
 *     eg Object_Read_Property() for the RP handler.
 *
 */
#endif
