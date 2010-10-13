/*
 * $HeadURL: https://svn.eaton.com/svn/PCM-tk/trunk/PCM-tk/etn_source/tbd/gw_device.c $
 *---------------------------------------------------------------------
 * $Author: brennant $
 * $Date: Oct 13, 2010 $
 *---------------------------------------------------------------------
 * Copyright (c) 2010 Eaton Corporation, All Rights Reserved.
 *
 * This software is the confidential and proprietary information of
 * Eaton Corporation (Confidential Information).  You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered into
 * with Eaton Corporation.
 *
 * EATON CORPORATION MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
 * SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING
 * FITNESS FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. EATON
 * CORPORATION SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY
 * LICENSEE AS A RESULT OF USING, MODIFYING OR DISTRIBUTING THIS
 * SOFTWARE OR ITS DERIVATIVES.
 *---------------------------------------------------------------------
 */

/** @file gw_device.c  Functions that extend the Device object to support routing. */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>     /* for memmove */
#include <time.h>       /* for timezone, localtime */
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bacapp.h"
#include "config.h"     /* the custom stuff */
#include "apdu.h"
#include "wp.h" /* write property handling */
#include "rp.h" /* read property handling */
#include "version.h"
#include "device.h"     /* me */
#include "handlers.h"
#include "datalink.h"
#include "address.h"
/* include the objects */
#include "device.h"
#include "ai.h"
#include "ao.h"
#include "av.h"
#include "bi.h"
#include "bo.h"
#include "bv.h"
#include "lc.h"
#include "lsp.h"
#include "mso.h"
#include "ms-input.h"
#include "trendlog.h"
#if defined(BACFILE)
#include "bacfile.h"    /* object list dependency */
#endif
/* os specfic includes */
#include "timer.h"

#if defined(__BORLANDC__)
/* seems to not be defined in time.h as specified by The Open Group */
/* difference from UTC and local standard time  */
long int timezone;
#endif

/* local forward and external prototypes */
extern int Device_Read_Property_Local(
    BACNET_READ_PROPERTY_DATA * rpdata);
extern bool Device_Write_Property_Local(
    BACNET_WRITE_PROPERTY_DATA * wp_data);
int Routed_Device_Read_Property_Local(
    BACNET_READ_PROPERTY_DATA * rpdata);
bool Routed_Device_Write_Property_Local(
    BACNET_WRITE_PROPERTY_DATA * wp_data);


#if !BAC_ROUTING
#warning This file should not be included in the build unless BAC_ROUTING is enabled.
#endif

/****************************************************************************
 ************* BACnet Routing Functionality (Optional) **********************
 ****************************************************************************
 * It would be correct to view the routing functionality here as inheriting 
 * and extending the regular Device Object functionality.
 ****************************************************************************/

/** Model the gateway as the main Device, with (two) remote
 * Devices that are reached via its routing capabilities.
 */
DEVICE_OBJECT_DATA Devices[MAX_NUM_DEVICES];
/** Keep track of the number of managed devices, including the gateway */
uint16_t Num_Managed_Devices = 0;
/** Which Device entry are we currently managing.
 * Since we are not using actual class objects here, the best we can do is 
 * keep this local variable which notes which of the Devices the current 
 * request is addressing.  Should default to 0, the main gateway Device.
 */
uint16_t iCurrent_Device_Idx = 0;

/* void Routing_Device_Init(uint32_t first_object_instance) is
 * found in device.c
 */

/** Add a Device to our table of Devices[].
 * The first entry must be the gateway device.
 * @param Object_Instance [in] Set the new Device to this instance number.
 * @param sObject_Name [in] Use this Object Name for the Device.
 * @param sDescription [in] Set this Description for the Device.
 * @return The index of this instance in the Devices[] array,
 *         or -1 if there isn't enough room to add this Device.
 */
uint16_t Add_Routed_Device( 
		uint32_t Object_Instance, 
		const char * sObject_Name, 
		const char * sDescription )
{
	int i = Num_Managed_Devices;
	if ( i < MAX_NUM_DEVICES )
    {
    	DEVICE_OBJECT_DATA *pDev = &Devices[i];
    	Num_Managed_Devices++;
    	iCurrent_Device_Idx = i;
    	pDev->bacObj.mObject_Type = OBJECT_DEVICE;
    	pDev->bacObj.Object_Instance_Number = Object_Instance;
    	Routed_Device_Set_Object_Name( sObject_Name, strlen( sObject_Name ));
    	Routed_Device_Set_Description( sDescription, strlen( sDescription ));
    	pDev->Database_Revision = 0;		/* Reset/Initialize now */
    	return i;
    }
	else
		return -1;
}


/** Return the Device Object descriptive data for the indicated entry.
 * @param idx [in] Index into Devices[] array being requested.
 *                 0 is for the main, gateway Device entry.
 *                 -1 is a special case meaning "whichever iCurrent_Device_Idx
 *                 is currently set to"
 * @return Pointer to the requested Device Object data, or NULL if the idx 
 *         is for an invalid row entry (eg, after the last good Device).
 */
DEVICE_OBJECT_DATA * Get_Routed_Device_Object( 
		int idx )
{
	if ( idx == -1 )
		return &Devices[iCurrent_Device_Idx];
	else if ( (idx >= 0) && (idx < MAX_NUM_DEVICES) )
		return &Devices[idx];
	else
		return NULL;
}

/** See if the Gateway or Routed Device at the given idx matches
 * the given MAC address.
 * Has the desirable side-effect of setting iCurrent_Device_Idx to the
 * given idx if a match is found, for use in the subsequent routing handling 
 * functions here.
 * 
 * @param idx [in] Index into Devices[] array being requested.
 *                 0 is for the main, gateway Device entry.
 * @param address_len [in] Length of the mac_adress[] field.
 *         If 0, then this is a MAC broadcast.  Otherwise, size is determined
 *         by the DLL type (eg, 6 for BIP and 2 for MSTP).
 * @param mac_adress [in] The desired MAC address of a Device;
 * 
 * @return True if the MAC addresses match (or the address_len is 0, 
 *         meaning MAC broadcast, so it's an automatic match). 
 *         Else False if no match or invalid idx is given.
 */
bool Lookup_Routed_Device_Address( 
		int idx, 
		uint8_t address_len,       
		uint8_t * mac_adress )
{
	bool result = false;
	DEVICE_OBJECT_DATA *pDev = &Devices[idx];
	int i;

	if ( (idx >= 0) && (idx < MAX_NUM_DEVICES) ) {
		if ( address_len == 0 ) {
			/* Automatic match */
			iCurrent_Device_Idx = idx;
			result = true;
		} else {
		    for (i = 0; i < address_len; i++) {
		        if (pDev->bacDevAddr.mac[i] != mac_adress[i])
		            break;
		    }
		    if ( i == address_len ) {		/* Success! */
				iCurrent_Device_Idx = idx;
				result = true;
		    }
		}
	}
	return result;
}

/* methods to override the normal Device objection functions */

uint32_t Routed_Device_Index_To_Instance(
    unsigned index)
{
    index = index;
    return Devices[iCurrent_Device_Idx].bacObj.Object_Instance_Number;
}

/** See if the requested Object instance matches that for the currently 
 * indexed Device Object. 
 * iCurrent_Device_Idx must have been set to point to this Device Object  
 * before this function is called.
 * @param object_id [in] Object ID of the desired Device object.
 * 			If the wildcard value (BACNET_MAX_INSTANCE), always matches. 
 * @return True if Object ID matches the present Device, else False.
 */
bool Routed_Device_Valid_Object_Instance_Number(
    uint32_t object_id)
{
	bool bResult = false;
	DEVICE_OBJECT_DATA *pDev = &Devices[iCurrent_Device_Idx];
    /* BACnet allows for a wildcard instance number */
	if (object_id == BACNET_MAX_INSTANCE)
		bResult = true;
	if ( pDev->bacObj.Object_Instance_Number == object_id ) 
		bResult = true;
	return bResult;
}

char *Routed_Device_Name(
    uint32_t object_instance)
{
	DEVICE_OBJECT_DATA *pDev = &Devices[iCurrent_Device_Idx];
    if (object_instance == pDev->bacObj.Object_Instance_Number) {
        return pDev->bacObj.Object_Name;
    }

    return NULL;
}

/** Manages ReadProperty service for fields which are different for routed
 * Devices, or hands off to the default Device RP function for the rest.
 * @param rpdata [in] Structure which describes the property to be read.
 * @return The length of the apdu encoded, or BACNET_STATUS_ERROR for error or
 * BACNET_STATUS_ABORT for abort message.
 */
int Routed_Device_Read_Property_Local(
    BACNET_READ_PROPERTY_DATA * rpdata)
{
    int apdu_len = 0;   /* return value */
    BACNET_CHARACTER_STRING char_string;
    uint8_t *apdu = NULL;
	DEVICE_OBJECT_DATA *pDev = &Devices[iCurrent_Device_Idx];

    if ((rpdata == NULL) || (rpdata->application_data == NULL) ||
        (rpdata->application_data_len == 0)) {
        return 0;
    }
    apdu = rpdata->application_data;
    switch (rpdata->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len =
                encode_application_object_id(&apdu[0], OBJECT_DEVICE,
                		pDev->bacObj.Object_Instance_Number);
            break;
        case PROP_OBJECT_NAME:
            characterstring_init_ansi(&char_string, pDev->bacObj.Object_Name);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_DESCRIPTION:
            characterstring_init_ansi(&char_string, pDev->Description);
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
        case PROP_DATABASE_REVISION:
            apdu_len =
                encode_application_unsigned(&apdu[0], pDev->Database_Revision);
            break;
        default:
            apdu_len = Device_Read_Property_Local( rpdata);
            break;
    }

	return ( apdu_len );
}

bool Routed_Device_Write_Property_Local(
    BACNET_WRITE_PROPERTY_DATA * wp_data)
{
    bool status = false;        /* return value */
    int len = 0;
    BACNET_APPLICATION_DATA_VALUE value;

    /* decode the some of the request */
    len =
        bacapp_decode_application_data(wp_data->application_data,
        wp_data->application_data_len, &value);
    if (len < 0) {
        /* error while decoding - a value larger than we can handle */
        wp_data->error_class = ERROR_CLASS_PROPERTY;
        wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
        return false;
    }
    /* FIXME: len < application_data_len: more data? */
    switch (wp_data->object_property) {
        case PROP_OBJECT_IDENTIFIER:
            status =
                WPValidateArgType(&value, BACNET_APPLICATION_TAG_OBJECT_ID,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                if ((value.type.Object_Id.type == OBJECT_DEVICE) &&
                    (Routed_Device_Set_Object_Instance_Number(value.type.
                            Object_Id.instance))) {
                    /* FIXME: we could send an I-Am broadcast to let the world know */
                } else {
                    status = false;
                    wp_data->error_class = ERROR_CLASS_PROPERTY;
                    wp_data->error_code = ERROR_CODE_VALUE_OUT_OF_RANGE;
                }
            }
            break;
        case PROP_OBJECT_NAME:
            status =
                WPValidateString(&value, MAX_DEV_NAME_LEN, false,
                &wp_data->error_class, &wp_data->error_code);
            if (status) {
                Routed_Device_Set_Object_Name(characterstring_value(&value.
                        type.Character_String),
                    characterstring_length(&value.type.Character_String));
            }
            break;
        default:
            status = Device_Write_Property_Local( wp_data);
            break;
    }
    return status;
}

/* methods to manipulate the data */

/** Return the Object Instance number for our (single) Device Object.
 * This is a key function, widely invoked by the handler code, since
 * it provides "our" (ie, local) address.
 * @ingroup ObjIntf
 * @return The Instance number used in the BACNET_OBJECT_ID for the Device.
 */
uint32_t Routed_Device_Object_Instance_Number(
    void)
{
    return Devices[iCurrent_Device_Idx].bacObj.Object_Instance_Number;
}

bool Routed_Device_Set_Object_Instance_Number(
    uint32_t object_id)
{
    bool status = true; /* return value */

    if (object_id <= BACNET_MAX_INSTANCE) {
        /* Make the change and update the database revision */
    	Devices[iCurrent_Device_Idx].bacObj.Object_Instance_Number = object_id;
        Routed_Device_Inc_Database_Revision();
    } else
        status = false;

    return status;
}


/** Sets the Object Name for a routed Device (or the gateway).
 * Uses local variable iCurrent_Device_Idx to know which Device
 * is to be updated.
 * @param name [in] Text for the new Object Name.
 * @param length [in] Length of name[] text.
 * @return True if succeed in updating Object Name, else False.
 */
bool Routed_Device_Set_Object_Name(
    const char *name,
    size_t length)
{
    bool status = false;        /*return value */
	DEVICE_OBJECT_DATA *pDev = &Devices[iCurrent_Device_Idx];

    if (length < MAX_DEV_NAME_LEN) {
        /* Make the change and update the database revision */
        memmove(pDev->bacObj.Object_Name, name, length);
        pDev->bacObj.Object_Name[length] = 0;
        Routed_Device_Inc_Database_Revision();
        status = true;
    }

    return status;
}

bool Routed_Device_Set_Description(
    const char *name,
    size_t length)
{
    bool status = false;        /*return value */
	DEVICE_OBJECT_DATA *pDev = &Devices[iCurrent_Device_Idx];

    if (length < MAX_DEV_DESC_LEN) {
        memmove(pDev->Description, name, length);
        pDev->Description[length] = 0;
        status = true;
    }

    return status;
}


/* 
 * Shortcut for incrementing database revision as this is potentially
 * the most common operation if changing object names and ids is
 * implemented.
 */
void Routed_Device_Inc_Database_Revision(
    void)
{
	DEVICE_OBJECT_DATA *pDev = &Devices[iCurrent_Device_Idx];
	pDev->Database_Revision++;
}


