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

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include "bacdef.h"
#include "bacenum.h"
#include "config.h" // the custom stuff

static BACNET_DEVICE_STATUS System_Status = STATUS_OPERATIONAL;
static const char *Vendor_Name = "ASHRAE";
static uint16_t Vendor_Identifier = 0;
static const char *Model_Name = "GNU";
static const char *Firmware_Revision = "1.0";
static const char *Application_Software_Version = "1.0";
//static char *Location = "USA";
static const char *Description = "server";
//static uint8_t Protocol_Version = 1; - constant, not settable
//static uint8_t Protocol_Revision = 4; - constant, not settable
//Protocol_Services_Supported
//Protocol_Object_Types_Supported
//Object_List
//static uint16_t Max_APDU_Length_Accepted = MAX_APDU; - constant
//static BACNET_SEGMENTATION Segmentation_Supported = SEGMENTATION_NONE;
//static uint8_t Max_Segments_Accepted = 0;
//VT_Classes_Supported
//Active_VT_Sessions
//Local_Time - rely on OS, if there is one
//Local_Date - rely on OS, if there is one
//UTC_Offset - rely on OS, if there is one
//Daylight_Savings_Status - rely on OS, if there is one
//APDU_Segment_Timeout
static uint16_t APDU_Timeout = 3000;
static uint8_t Number_Of_APDU_Retries = 3;
//List_Of_Session_Keys
//Time_Synchronization_Recipients
//Max_Master - rely on MS/TP subsystem, if there is one
//Max_Info_Frames - rely on MS/TP subsystem, if there is one
//Device_Address_Binding - required, but relies on binding cache
static uint8_t Database_Revision = 0;
//Configuration_Files
//Last_Restore_Time
//Backup_Failure_Timeout
//Active_COV_Subscriptions
//Slave_Proxy_Enable
//Manual_Slave_Address_Binding
//Auto_Slave_Discovery
//Slave_Address_Binding
//Profile_Name

// methods to manipulate the data
// FIXME: add APDU encode methods for each?
BACNET_DEVICE_STATUS Device_System_Status(void)
{
  return System_Status;
}

void Device_Set_System_Status(BACNET_DEVICE_STATUS status)
{
  // FIXME: bounds check?
  System_Status = status;
}

const char *Device_Vendor_Name(void)
{
  return Vendor_Name;
}

void Device_Set_Vendor_Name(const char *name)
{
  Vendor_Name = name;
}

uint16_t Device_Vendor_Identifier(void)
{
  return Vendor_Identifier;
}

void Device_Set_Vendor_Identifier(uint16_t vendor_id)
{
  Vendor_Identifier = vendor_id;
}

const char *Device_Model_Name(void)
{
  return Model_Name;
}

void Device_Set_Model_Name(const char *name)
{
  Model_Name = name;
}

const char *Device_Firmware_Revision(void)
{
  return Firmware_Revision;
}

void Device_Set_Firmware_Revision(const char *name)
{
  Firmware_Revision = name;
}

const char *Device_Application_Software_Version(void)
{
  return Application_Software_Version;
}

void Device_Set_Application_Software_Version(const char *name)
{
  Application_Software_Version = name;
}

const char *Device_Description(void)
{
  return Description;
}

void Device_Set_Description(const char *name)
{
  Description = name;
}

uint8_t Device_Protocol_Version(void)
{
  return 1;
}

uint8_t Device_Protocol_Revision(void)
{
  return 4;
}

uint16_t Device_Max_APDU_Length_Accepted(void)
{
  return MAX_APDU;
}

BACNET_SEGMENTATION Device_Segmentation_Supported(void)
{
  return SEGMENTATION_NONE;
}

//FIXME: This need some bit string encodings...
//Protocol_Services_Supported
//Protocol_Object_Types_Supported
//FIXME: Probably return one at a time, or be supported by
// an object module, or encode the APDU here.
//Object_List


uint16_t Device_APDU_Timeout(void)
{
  return APDU_Timeout;
}

void Device_Set_APDU_Timeout(uint16_t timeout)
{
  APDU_Timeout = timeout;
}

uint8_t Device_Number_Of_APDU_Retries(void)
{
  return Number_Of_APDU_Retries;
}

void Device_Set_Number_Of_APDU_Retries(uint8_t retries)
{
  Number_Of_APDU_Retries = retries;
}

uint8_t Device_Database_Revision(void)
{
  return Database_Revision;
}

void Device_Set_Database_Revision(uint8_t revision)
{
  Database_Revision = revision;
}

