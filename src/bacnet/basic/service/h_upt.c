/**************************************************************************
 *
 * Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/apdu.h"
#include "bacnet/npdu.h"
#include "bacnet/abort.h"
#include "bacnet/ptransfer.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"

/** @file h_upt.c  Handles Unconfirmed Private Transfer requests. */

void private_transfer_print_data(BACNET_PRIVATE_TRANSFER_DATA *private_data)
{
#ifdef BACAPP_PRINT_ENABLED
    BACNET_OBJECT_PROPERTY_VALUE object_value; /* for bacapp printing */
#endif
    BACNET_APPLICATION_DATA_VALUE value; /* for decode value data */
    int len = 0;
    uint8_t *application_data;
    int application_data_len;
    bool first_value = true;
#if PRINT_ENABLED
    bool print_brace = false;
#endif

    if (private_data) {
#if PRINT_ENABLED
        printf("PrivateTransfer:vendorID=%u\r\n",
            (unsigned)private_data->vendorID);
        printf("PrivateTransfer:serviceNumber=%lu\r\n",
            (unsigned long)private_data->serviceNumber);
#endif
        application_data = private_data->serviceParameters;
        application_data_len = private_data->serviceParametersLen;
        for (;;) {
            len = bacapp_decode_application_data(
                application_data, (uint8_t)application_data_len, &value);
            if (first_value && (len < application_data_len)) {
                first_value = false;
#if PRINT_ENABLED
                fprintf(stdout, "{");
                print_brace = true;
#endif
            }
            /* private transfer doesn't provide any clues */
#ifdef BACAPP_PRINT_ENABLED
            object_value.object_type = MAX_BACNET_OBJECT_TYPE;
            object_value.object_instance = BACNET_MAX_INSTANCE;
            object_value.object_property = MAX_BACNET_PROPERTY_ID;
            object_value.array_index = BACNET_ARRAY_ALL;
            object_value.value = &value;
            bacapp_print_value(stdout, &object_value);
#endif
            if (len > 0) {
                if (len < application_data_len) {
                    application_data += len;
                    application_data_len -= len;
                    /* there's more! */
#if PRINT_ENABLED
                    fprintf(stdout, ",");
#endif
                } else {
                    break;
                }
            } else {
                break;
            }
        }
#if PRINT_ENABLED
        if (print_brace) {
            fprintf(stdout, "}");
        }
        fprintf(stdout, "\r\n");
#endif
    }
}

void handler_unconfirmed_private_transfer(
    uint8_t *service_request, uint16_t service_len, BACNET_ADDRESS *src)
{
    BACNET_PRIVATE_TRANSFER_DATA private_data;
    int len = 0;

    (void)src;
#if PRINT_ENABLED
    fprintf(stderr, "Received Unconfirmed Private Transfer Request!\n");
#endif
    len = ptransfer_decode_service_request(
        service_request, service_len, &private_data);
    if (len >= 0) {
        private_transfer_print_data(&private_data);
    }
}
