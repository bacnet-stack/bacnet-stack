/**************************************************************************
*
* Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
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

/* WRITEPROP: command line tool that writes a property to a BACnet device. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> /* for time */
#include <errno.h>
#include "bactext.h"
#include "iam.h"
#include "arf.h"
#include "tsm.h"
#include "address.h"
#include "config.h"
#include "bacdef.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "net.h"
#include "datalink.h"
#include "whois.h"
/* some demo stuff needed */
#include "filename.h"
#include "handlers.h"
#include "client.h"
#include "txbuf.h"

// buffer used for receive
static uint8_t Rx_Buf[MAX_MPDU] = {0};

/* global variables used in this file */
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;
static uint32_t Target_Object_Instance = BACNET_MAX_INSTANCE;
static BACNET_OBJECT_TYPE Target_Object_Type = OBJECT_ANALOG_INPUT;
static BACNET_PROPERTY_ID Target_Object_Property = PROP_ACKED_TRANSITIONS;
/* array index value or BACNET_ARRAY_ALL */
static int32_t Target_Object_Property_Index = BACNET_ARRAY_ALL;
static BACNET_APPLICATION_TAG Target_Object_Property_Tag = BACNET_APPLICATION_TAG_NULL;
static BACNET_APPLICATION_DATA_VALUE Target_Object_Property_Value = {0};
/* 0 if not set, 1..16 if set */
static uint8_t Target_Object_Property_Priority = 0;

static BACNET_ADDRESS Target_Address;
static bool Error_Detected = false;

static void MyErrorHandler(
  BACNET_ADDRESS *src,
  uint8_t invoke_id,
  BACNET_ERROR_CLASS error_class,
  BACNET_ERROR_CODE error_code)
{
  /* FIXME: verify src and invoke id */
  (void)src;
  (void)invoke_id;
  printf("\r\nBACnet Error!\r\n");
  printf("Error Class: %s\r\n",
    bactext_error_class_name(error_class));
  printf("Error Code: %s\r\n",
    bactext_error_code_name(error_code));
  Error_Detected = true;
}

void MyAbortHandler(
  BACNET_ADDRESS *src,
  uint8_t invoke_id,
  uint8_t abort_reason)
{
  /* FIXME: verify src and invoke id */
  (void)src;
  (void)invoke_id;
  printf("\r\nBACnet Abort!\r\n");
  printf("Abort Reason: %s\r\n",
    bactext_abort_reason_name(abort_reason));
  Error_Detected = true;
}

void MyRejectHandler(
  BACNET_ADDRESS *src,
  uint8_t invoke_id,
  uint8_t reject_reason)
{
  /* FIXME: verify src and invoke id */
  (void)src;
  (void)invoke_id;
  printf("\r\nBACnet Reject!\r\n");
  printf("Reject Reason: %s\r\n",
    bactext_reject_reason_name(reject_reason));
  Error_Detected = true;
}

static void Init_Service_Handlers(void)
{
  /* we need to handle who-is 
     to support dynamic device binding to us */
  apdu_set_unconfirmed_handler(
    SERVICE_UNCONFIRMED_WHO_IS,
    handler_who_is);
  /* handle i-am to support binding to other devices */
  apdu_set_unconfirmed_handler(
    SERVICE_UNCONFIRMED_I_AM,
    handler_i_am_bind);
  /* set the handler for all the services we don't implement
     It is required to send the proper reject message... */
  apdu_set_unrecognized_service_handler_handler(
    handler_unrecognized_service);
  /* we must implement read property - it's required! */
  apdu_set_confirmed_handler(
    SERVICE_CONFIRMED_READ_PROPERTY,
    handler_read_property);
  /* handle any errors coming back */
  apdu_set_error_handler(
    SERVICE_CONFIRMED_READ_PROPERTY,
    MyErrorHandler);
  apdu_set_abort_handler(
    MyAbortHandler);
  apdu_set_reject_handler(
    MyRejectHandler);
}

int main(int argc, char *argv[])
{
  BACNET_ADDRESS src = {0};  // address where message came from
  uint16_t pdu_len = 0;
  unsigned timeout = 100; // milliseconds
  unsigned max_apdu = 0;
  time_t elapsed_seconds = 0;
  time_t last_seconds = 0; 
  time_t current_seconds = 0;
  time_t timeout_seconds = 0;
  uint8_t invoke_id = 0;
  bool found = false;
  char *value_string = NULL;
  bool status = false;
  
  if (argc < 7)
  {
    /* note: priority 16 and 0 should produce the same end results... */
    printf("%s device-instance object-type object-instance "
      "property tag value [priority] [index]\r\n",
      filename_remove_path(argv[0]));
    return 0;
  }
  /* decode the command line parameters */
  Target_Device_Object_Instance = strtol(argv[1],NULL,0);
  Target_Object_Type = strtol(argv[2],NULL,0);
  Target_Object_Instance = strtol(argv[3],NULL,0);
  Target_Object_Property = strtol(argv[4],NULL,0);
  Target_Object_Property_Tag = strtol(argv[5],NULL,0);
  value_string = argv[6];
  /* optional priority */
  if (argc > 7)
    Target_Object_Property_Index = strtol(argv[7],NULL,0);
  /* optional index */
  if (argc > 8)
    Target_Object_Property_Index = strtol(argv[8],NULL,0);
    
  if (Target_Device_Object_Instance >= BACNET_MAX_INSTANCE)
  {
    fprintf(stderr,"device-instance=%u - it must be less than %u\r\n",
      Target_Device_Object_Instance,BACNET_MAX_INSTANCE);
    return 1;
  }
  if (Target_Object_Type > MAX_BACNET_OBJECT_TYPE)
  {
    fprintf(stderr,"object-type=%u - it must be less than %u\r\n",
      Target_Object_Type,MAX_BACNET_OBJECT_TYPE+1);
    return 1;
  }
  if (Target_Object_Instance > BACNET_MAX_INSTANCE)
  {
    fprintf(stderr,"object-instance=%u - it must be less than %u\r\n",
      Target_Object_Instance,BACNET_MAX_INSTANCE+1);
    return 1;
  }
  if (Target_Object_Property > MAX_BACNET_PROPERTY_ID)
  {
    fprintf(stderr,"object-type=%u - it must be less than %u\r\n",
      Target_Object_Property,MAX_BACNET_PROPERTY_ID+1);
    return 1;
  }
  if (Target_Object_Property_Tag >= MAX_BACNET_APPLICATION_TAG)
  {
    fprintf(stderr,"tag=%u - it must be less than %u\r\n",
      Target_Object_Property_Tag,MAX_BACNET_APPLICATION_TAG);
    return 1;
  }
  status = bacapp_parse_application_data(
    Target_Object_Property_Tag,
    value_string,
    &Target_Object_Property_Value);
  if (!status)
  {
    /* FIXME: show the expected entry format for the tag */
    fprintf(stderr,"unable to parse the tag value\r\n");
    return 1;
  }
  
  /* setup my info */
  Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
  address_init();
  Init_Service_Handlers();
  /* configure standard BACnet/IP port */
  bip_set_interface("eth0"); /* for linux */
  bip_set_port(0xBAC0);
  if (!bip_init())
    return 1;
  /* configure the timeout values */
  last_seconds = time(NULL);
  timeout_seconds = (Device_APDU_Timeout() / 1000) *
    Device_Number_Of_APDU_Retries();
  /* try to bind with the device */
  Send_WhoIs(Target_Device_Object_Instance,Target_Device_Object_Instance);
  /* loop forever */
  for (;;)
  {
    /* increment timer - exit if timed out */
    current_seconds = time(NULL);

    /* returns 0 bytes on timeout */
    pdu_len = bip_receive(
      &src,
      &Rx_Buf[0],
      MAX_MPDU,
      timeout);

    /* process */
    if (pdu_len)
    {
      npdu_handler(
        &src,
        &Rx_Buf[0],
        pdu_len);
    }
    /* at least one second has passed */
    if (current_seconds != last_seconds)
      tsm_timer_milliseconds(((current_seconds - last_seconds) * 1000));
    if (Error_Detected)
      break;
    if (I_Am_Request)
    {
      I_Am_Request = false;
      iam_send(&Handler_Transmit_Buffer[0]);
    }
    else
    {
      /* wait until the device is bound, or timeout and quit */
      found = address_bind_request(
        Target_Device_Object_Instance,
        &max_apdu,
        &Target_Address);
      if (found)
      {
        if (invoke_id == 0) 
        {
          invoke_id = Send_Write_Property_Request(
            Target_Device_Object_Instance,
            Target_Object_Type,
            Target_Object_Instance,
            Target_Object_Property,
            &Target_Object_Property_Value,
            Target_Object_Property_Priority,
            Target_Object_Property_Index);
        }
        else if (tsm_invoke_id_free(invoke_id))
          break;
      }
      else
      {
        /* increment timer - exit if timed out */
        elapsed_seconds += (current_seconds - last_seconds);
        if (elapsed_seconds > timeout_seconds)
          break;
      }
    }
    /* keep track of time for next check */
    last_seconds = current_seconds;
  }

  return 0;
}
