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
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include "config.h"
#include "address.h"
#include "bacdef.h"
#include "handlers.h"
#include "client.h"
#include "dlenv.h"
#include "bacdcode.h"
#include "npdu.h"
#include "apdu.h"
#include "iam.h"
#include "tsm.h"
#include "device.h"
#include "bacfile.h"
#include "datalink.h"
#include "dcc.h"
#include "net.h"
#include "txbuf.h"
#include "lc.h"
#include "version.h"
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
#include "bacfile.h"

/* This is an example server application using the BACnet Stack */

/* buffers used for receiving */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

static void Init_Object(
    BACNET_OBJECT_TYPE object_type,
    rpm_property_lists_function rpm_list_function,
    read_property_function rp_function,
    object_valid_instance_function object_valid_function,
    write_property_function wp_function,
    object_count_function count_function,
    object_index_to_instance_function index_function,
    object_name_function name_function)
{
    handler_read_property_object_set(
        object_type,
        rp_function,
		object_valid_function);
    handler_write_property_object_set(
        object_type,
        wp_function);
	handler_read_property_multiple_list_set(
        object_type,
		rpm_list_function);
    Device_Object_Function_Set(
        object_type,
        count_function,
        index_function,
        name_function);
}

static void Init_Objects(void)
{
    Device_Init();
    Init_Object(
        OBJECT_DEVICE,
        Device_Property_Lists,
        Device_Encode_Property_APDU,
		Device_Valid_Object_Instance_Number,
        Device_Write_Property,
        NULL,
        NULL,
        NULL);

    Analog_Input_Init();
    Init_Object(
        OBJECT_ANALOG_INPUT,
        Analog_Input_Property_Lists,
        Analog_Input_Encode_Property_APDU,
		Analog_Input_Valid_Instance,
        NULL,
        Analog_Input_Count,
        Analog_Input_Index_To_Instance,
        Analog_Input_Name);
    
    Analog_Output_Init();
    Init_Object(
        OBJECT_ANALOG_OUTPUT,
        Analog_Output_Property_Lists,
        Analog_Output_Encode_Property_APDU,
		Analog_Output_Valid_Instance,
        Analog_Output_Write_Property,
        Analog_Output_Count,
        Analog_Output_Index_To_Instance,
        Analog_Output_Name);

    Analog_Value_Init();
    Init_Object(
        OBJECT_ANALOG_VALUE,
        Analog_Value_Property_Lists,
        Analog_Value_Encode_Property_APDU,
		Analog_Value_Valid_Instance,
        Analog_Value_Write_Property,
        Analog_Value_Count,
        Analog_Value_Index_To_Instance,
        Analog_Value_Name);

    Binary_Input_Init();
    Init_Object(
        OBJECT_BINARY_INPUT,
        Binary_Input_Property_Lists,
        Binary_Input_Encode_Property_APDU,
		Binary_Input_Valid_Instance,
        NULL,
        Binary_Input_Count,
        Binary_Input_Index_To_Instance,
        Binary_Input_Name);
    
    Binary_Output_Init();
    Init_Object(
        OBJECT_BINARY_OUTPUT,
        Binary_Output_Property_Lists,
        Binary_Output_Encode_Property_APDU,
		Binary_Output_Valid_Instance,
        Binary_Output_Write_Property,
        Binary_Output_Count,
        Binary_Output_Index_To_Instance,
        Binary_Output_Name);

    Binary_Value_Init();
    Init_Object(
        OBJECT_BINARY_VALUE,
        Binary_Value_Property_Lists,
        Binary_Value_Encode_Property_APDU,
		Binary_Value_Valid_Instance,
        Binary_Value_Write_Property,
        Binary_Value_Count,
        Binary_Value_Index_To_Instance,
        Binary_Value_Name);

    Life_Safety_Point_Init();
    Init_Object(
        OBJECT_LIFE_SAFETY_POINT,
        Life_Safety_Point_Property_Lists,
        Life_Safety_Point_Encode_Property_APDU,
		Life_Safety_Point_Valid_Instance,
        Life_Safety_Point_Write_Property,
        Life_Safety_Point_Count,
        Life_Safety_Point_Index_To_Instance,
        Life_Safety_Point_Name);

    Load_Control_Init();
    Init_Object(
        OBJECT_LOAD_CONTROL,
        Load_Control_Property_Lists,
        Load_Control_Encode_Property_APDU,
		Load_Control_Valid_Instance,
        Load_Control_Write_Property,
        Load_Control_Count,
        Load_Control_Index_To_Instance,
        Load_Control_Name);

    Multistate_Output_Init();
    Init_Object(
        OBJECT_LIFE_SAFETY_POINT,
        Multistate_Output_Property_Lists,
        Multistate_Output_Encode_Property_APDU,
		Multistate_Output_Valid_Instance,
        Multistate_Output_Write_Property,
        Multistate_Output_Count,
        Multistate_Output_Index_To_Instance,
        Multistate_Output_Name);

#if defined(BACFILE)
    bacfile_init();
    Init_Object(
        OBJECT_FILE,
        BACfile_Property_Lists,
        bacfile_encode_property_apdu,
		bacfile_valid_instance,
        bacfile_write_property,
        bacfile_count,
        bacfile_index_to_instance,
        bacfile_name);
#endif
}

static void Init_Service_Handlers(
    void)
{
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
    /* set the handler for all the services we don't implement */
    /* It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler
        (handler_unrecognized_service);
    /* Set the handlers for any confirmed services that we support. */
    /* We must implement read property - it's required! */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROP_MULTIPLE,
        handler_read_property_multiple);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROPERTY,
        handler_write_property);
#if defined(BACFILE)
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_ATOMIC_READ_FILE,
        handler_atomic_read_file);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_ATOMIC_WRITE_FILE,
        handler_atomic_write_file);
#endif
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_REINITIALIZE_DEVICE,
        handler_reinitialize_device);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION,
        handler_timesync_utc);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION,
        handler_timesync);
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_SUBSCRIBE_COV,
        handler_cov_subscribe);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_COV_NOTIFICATION,
        handler_ucov_notification);
    /* handle communication so we can shutup when asked */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
}

static void cleanup(
    void)
{
    datalink_cleanup();
}

int main(
    int argc,
    char *argv[])
{
    BACNET_ADDRESS src = {
        0
    };  /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 1000;    /* milliseconds */
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    uint32_t elapsed_seconds = 0;
    uint32_t elapsed_milliseconds = 0;

    /* allow the device ID to be set */
    if (argc > 1)
        Device_Set_Object_Instance_Number(strtol(argv[1], NULL, 0));
    printf("BACnet Server Demo\n" "BACnet Stack Version %s\n"
        "BACnet Device ID: %u\n" "Max APDU: %d\n", BACnet_Version,
        Device_Object_Instance_Number(), MAX_APDU);
    Init_Objects();
    Init_Service_Handlers();
    dlenv_init();
    atexit(cleanup);
    /* configure the timeout values */
    last_seconds = time(NULL);
    /* broadcast an I-Am on startup */
    Send_I_Am(&Handler_Transmit_Buffer[0]);
    /* loop forever */
    for (;;) {
        /* input */
        current_seconds = time(NULL);

        /* returns 0 bytes on timeout */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);

        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }
        /* at least one second has passed */
        elapsed_seconds = current_seconds - last_seconds;
        if (elapsed_seconds) {
            last_seconds = current_seconds;
            dcc_timer_seconds(elapsed_seconds);
#if defined(BACDL_BIP) && BBMD_ENABLED
            bvlc_maintenance_timer(elapsed_seconds);
#endif
            Load_Control_State_Machine_Handler();
            elapsed_milliseconds = elapsed_seconds * 1000;
            handler_cov_task(elapsed_seconds);
            tsm_timer_milliseconds(elapsed_milliseconds);
        }
        /* output */

        /* blink LEDs, Turn on or off outputs, etc */
    }
}
