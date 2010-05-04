/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/

/** @file device.h Defines functions for handling all BACnet objects belonging
 *                 to a BACnet device, as well as Device-specific properties. */

#ifndef DEVICE_H
#define DEVICE_H

#include <stdbool.h>
#include <stdint.h>
#include "bacdef.h"
#include "bacenum.h"
#include "wp.h"
#include "rd.h"
#include "rp.h"
#include "rpm.h"
#include "readrange.h"

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
 * @param [in] The object instance number to be looked up.
 * @return Pointer to a string containing the unique Object_Name.  This string
 *         is temporary and should be copied upon the return.  It is
 *         allocated by the system and does not need to be freed.
 */
typedef char *(
    *object_name_function)
     (
    uint32_t object_instance);

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


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void Device_Init(
        void);

    bool Device_Reinitialize(
        BACNET_REINITIALIZE_DEVICE_DATA * rd_data);

    BACNET_REINITIALIZED_STATE Device_Reinitialized_State(
        void);

    rr_info_function Device_Objects_RR_Info(
        BACNET_OBJECT_TYPE object_type);

    void Device_Property_Lists(
        const int **pRequired,
        const int **pOptional,
        const int **pProprietary);
    void Device_Objects_Property_List(
        BACNET_OBJECT_TYPE object_type,
        struct special_property_list_t *pPropertyList);

    uint32_t Device_Object_Instance_Number(
        void);
    bool Device_Set_Object_Instance_Number(
        uint32_t object_id);
    bool Device_Valid_Object_Instance_Number(
        uint32_t object_id);
    unsigned Device_Object_List_Count(
        void);
    bool Device_Object_List_Identifier(
        unsigned array_index,
        int *object_type,
        uint32_t * instance);

    unsigned Device_Count(
        void);
    uint32_t Device_Index_To_Instance(
        unsigned index);
    char *Device_Name(
        uint32_t object_instance);

    BACNET_DEVICE_STATUS Device_System_Status(
        void);
    int Device_Set_System_Status(
        BACNET_DEVICE_STATUS status,
        bool local);

    const char *Device_Vendor_Name(
        void);

    uint16_t Device_Vendor_Identifier(
        void);

    const char *Device_Model_Name(
        void);
    bool Device_Set_Model_Name(
        const char *name,
        size_t length);

    const char *Device_Firmware_Revision(
        void);

    const char *Device_Application_Software_Version(
        void);
    bool Device_Set_Application_Software_Version(
        const char *name,
        size_t length);

    bool Device_Set_Object_Name(
        const char *name,
        size_t length);
    const char *Device_Object_Name(
        void);

    const char *Device_Description(
        void);
    bool Device_Set_Description(
        const char *name,
        size_t length);

    const char *Device_Location(
        void);
    bool Device_Set_Location(
        const char *name,
        size_t length);

    /* some stack-centric constant values - no set methods */
    uint8_t Device_Protocol_Version(
        void);
    uint8_t Device_Protocol_Revision(
        void);
    BACNET_SEGMENTATION Device_Segmentation_Supported(
        void);

    uint32_t Device_Database_Revision(
        void);
    void Device_Set_Database_Revision(
        uint32_t revision);
    void Device_Inc_Database_Revision(
        void);

    bool Device_Valid_Object_Name(
        const char *object_name,
        int *object_type,
        uint32_t * object_instance);
    char *Device_Valid_Object_Id(
        int object_type,
        uint32_t object_instance);

    int Device_Read_Property(
        BACNET_READ_PROPERTY_DATA * rpdata);
    bool Device_Write_Property(
        BACNET_WRITE_PROPERTY_DATA * wp_data);

    bool DeviceGetRRInfo(
        BACNET_READ_RANGE_DATA * pRequest,      /* Info on the request */
        RR_PROP_INFO * pInfo);  /* Where to put the information */


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
