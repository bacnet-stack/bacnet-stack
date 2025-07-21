/**
 * @file
 * @brief command line fuzz (data scrambling) interface for security testing
 * @author Anthony Delorenzo <anthony@crystalpeaksecurity.com>
 * @date 2023
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <assert.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactext.h"
#include "bacnet/bacerror.h"
#include "bacnet/iam.h"
#include "bacnet/list_element.h"
#include "bacnet/arf.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/version.h"
/* some demo modules we use */
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/services.h"
/* port agnostic file */
#include "bacport.h"
/* our datalink layers */
#include "bacnet/datalink/dlmstp.h"
#include "bacnet/datalink/bip.h"
#include "bacnet/datalink/bvlc.h"
#include "bacnet/basic/bbmd/h_bbmd.h"

// Pull in all of this...
#include "../router-mstp/main.c"

static void Init_Service_Handlers(void)
{
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_WHO_IS, handler_who_is_unicast);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, handler_write_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_RANGE, handler_read_range);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_REINITIALIZE_DEVICE, handler_reinitialize_device);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_UTC_TIME_SYNCHRONIZATION, handler_timesync_utc);
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_TIME_SYNCHRONIZATION, handler_timesync);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_SUBSCRIBE_COV, handler_cov_subscribe);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_add);
}

/*
 * FIXME: This is a hack to get things linking correctly
 */
extern int cov_subscribe(void)
{
    return 0;
}
extern int Device_Value_List_Supported(void)
{
    return 0;
}
extern int Encode_RR_payload(void)
{
    return 0;
}
extern int Device_Objects_RR_Info(void)
{
    return 0;
}
extern int Device_Write_Property(void)
{
    return 0;
}
extern int Device_Reinitialize(void)
{
    return 0;
}

extern bool Device_COV(BACNET_OBJECT_TYPE object_type, uint32_t object_instance)
{
    return false;
}
extern void
Device_COV_Clear(BACNET_OBJECT_TYPE object_type, uint32_t object_instance)
{
    return;
}

extern bool Device_Encode_Value_List(
    BACNET_OBJECT_TYPE object_type,
    uint32_t object_instance,
    BACNET_PROPERTY_VALUE *value_list)
{
    return false;
}

extern int Device_Add_List_Element(BACNET_LIST_ELEMENT_DATA *list_element)
{
    return BACNET_STATUS_ERROR;
}

extern int Device_Remove_List_Element(BACNET_LIST_ELEMENT_DATA *list_element)
{
    return BACNET_STATUS_ERROR;
}

extern bool Device_Write_Property_Local(BACNET_WRITE_PROPERTY_DATA *wp_data)
{
    return false;
}

int LLVMFuzzerTestOneInput(uint8_t *data, size_t size)
{
    BACNET_ADDRESS src = { 0 };

    Init_Service_Handlers();

    /* process fuzz input*/
    if (size > 0 && size <= 0xffff) {
        my_routing_npdu_handler(BIP_Net, &src, data, (uint16_t)size);
    }

    return 0;
}
