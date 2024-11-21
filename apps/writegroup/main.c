/**
 * @file
 * @brief command line tool that sends a BACnet WriteGroup-Request message
 * to the network
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date October 2024
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h> /* for time */
#include <errno.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactext.h"
#include "bacnet/iam.h"
#include "bacnet/cov.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/channel_value.h"
#include "bacnet/write_group.h"
/* some demo stuff needed */
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"

static BACNET_WRITE_GROUP_DATA Write_Group_Data;

static void Init_Service_Handlers(void)
{
    Device_Init(NULL);
    /* we need to handle who-is
       to support dynamic device binding to us */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    /* handle i-am to support binding to other devices */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_bind);
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
}

static void print_usage(const char *filename)
{
    printf("Sends a BACnet WriteGroup-Reqeust to the network.\n");
    printf("\n");
    printf(
        "Usage: %s group-number priority <inhibit|delay>\n"
        "change-value [change-value]\n",
        filename);
    printf("\n");
    printf("group-number:\n"
           "parameter in the range 1-4294967295 that represents\n"
           " the control group to be affected by this request.\n");
    printf("\n");
    printf("priority:\n"
           "This Write_Priority parameter is an unsigned integer\n"
           "in the range 1..16 that represents the priority for writing\n"
           "that shall apply to any channel value changes that result\n"
           "in writes to properties of BACnet objects.\n");
    printf("\n");
    printf("change-value:\n"
           "This parameter shall specify a BACnetGroupChannelValue\n"
           "consisting of channel number, overridingPriority, value\n"
           "tuples representing each channel number whose value is\n"
           "to be updated.");
    printf("Since List_Of_Object_Property_References can include\n"
           "object properties of different data types, the value\n"
           "written to Present_Value may be coerced to another datatype.\n"
           "The rules governing how these coercions occur are\n"
           "defined in the BACnet standard.\n");
    printf("\n");
    printf("change-value: channel number\n");
    printf("Channel numbers shall range from 0 to 65535\n"
           "where the channel number corresponds directly to the\n"
           "Channel_Number property of a Channel object.");
    printf("\n");
    printf("change-value: overridingPriority\n"
           "The optional overridingPriority allows specific values\n"
           "to be written with some priority other than that specified\n"
           "by Write_Priority property. If overridingPriority 0 is given,\n"
           "no priority is sent.\n");
    printf("\n");
    printf("change-value: value\n"
           "BACnetChannelValue values that are any primitive application\n"
           "datatype or BACnetLightingCommand or BACnetColorCommand or\n"
           "BACnetXYColor constructed datatypes. The NULL value represents\n"
           "'relinquish control' as with commandable object properties.\n");
    printf("\n");
    printf("The numeric values are parsed in the following manner:\n"
           "null=Null, true or false=Boolean,\n"
           "numeric with negative sign=Signed Integer,\n"
           "numeric with decimal point=Real or Double\n"
           "Ltuple=BACnetLightingCommand\n"
           "Ctuple=BACnetColorCommand\n"
           "Xtuple=BACnetXYColor\n");
    printf("\n");
    printf(
        "Example:\n"
        "If you want generate a WriteGroup-Request,\n"
        "you could send one of the following command:\n"
        "%s 1 2 inhibit 3 0 100.0 4 0 null 5 0 -100 6 0 true 7 0 10\n"
        "where 1=group-number, 2=priority, 3=channel-number,\n"
        "0=overridingPriority, 5=channel-number, 6=channel-number,\n"
        "7=channel-number\n",
        filename);
}

/**
 * @brief Cleanup the resources used by the application
 */
static void cleanup(void)
{
    unsigned count;

    count = bacnet_write_group_change_list_count(&Write_Group_Data);
    while (count) {
        BACNET_GROUP_CHANNEL_VALUE *value;
        value =
            bacnet_write_group_change_list_element(&Write_Group_Data, count);
        if (count == 0) {
            /* index=0 change-value is static head, not dynamic */
            value->next = NULL;
        } else {
            free(value);
        }
        count--;
    }
}

/**
 * @brief Send a WriteGroup-Request to the network
 * @param argc [in] The number of command line arguments
 * @param argv [in] The command line arguments
 * @return 0 on success, 1 on failure
 */
int main(int argc, char *argv[])
{
    BACNET_ADDRESS dest = { 0 /*broadcast*/ };
    BACNET_WRITE_GROUP_DATA *data;
    BACNET_GROUP_CHANNEL_VALUE *value;
    int argi = 0;
    int len = 0;

    if (argc < 4) {
        print_usage(filename_remove_path(argv[0]));
        return 0;
    }
    data = &Write_Group_Data;
    /* decode the command line parameters */
    data->group_number = strtol(argv[1], NULL, 0);
    data->write_priority = strtol(argv[2], NULL, 0);
    if (bacnet_stricmp(argv[3], "inhibit") == 0) {
        data->inhibit_delay = WRITE_GROUP_INHIBIT_DELAY_TRUE;
    } else if (bacnet_stricmp(argv[3], "delay") == 0) {
        data->inhibit_delay = WRITE_GROUP_INHIBIT_DELAY_FALSE;
    } else {
        data->inhibit_delay = WRITE_GROUP_INHIBIT_DELAY_FALSE;
    }
    value = &data->change_list;
    for (argi = 4; argi < argc; argi++) {
        if (!value) {
            value = calloc(1, sizeof(BACNET_GROUP_CHANNEL_VALUE));
            bacnet_write_group_change_list_append(data, value);
        }
        if (value) {
            value->channel = strtol(argv[argi], NULL, 0);
            argi++;
            value->overriding_priority = strtol(argv[argi], NULL, 0);
            argi++;
            if (!bacnet_channel_value_from_ascii(&value->value, argv[argi])) {
                value->value.tag = BACNET_APPLICATION_TAG_NULL;
            }
            printf(
                "WriteGroup-Request added channel %u "
                "with priority %u value=%s tag=%s\n",
                value->channel, value->overriding_priority, argv[argi],
                bactext_application_tag_name(value->value.tag));
            value = value->next;
        }
    }
    atexit(cleanup);
    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    Init_Service_Handlers();
    dlenv_init();
    atexit(datalink_cleanup);
    len = Send_Write_Group(&dest, data);
    if (len <= 0) {
        fprintf(
            stderr, "Failed to Send WriteGroup-Request (%s)!\n",
            strerror(errno));
    } else {
        printf("Send WriteGroup-Request successful!\n");
    }

    return 0;
}
