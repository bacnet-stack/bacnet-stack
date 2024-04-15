/**
 * @file
 * @author Steve Karg
 * @date 2022
 * @brief Application to acquire data from a target client
 *
 * SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bactext.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/sys/debug.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/client/bac-task.h"
#include "bacnet/basic/client/bac-data.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlenv.h"

#if BACNET_SVC_SERVER
#error "App requires server-only features disabled! Set BACNET_SVC_SERVER=0"
#endif

/* print with flush by default */
#define PRINTF debug_aprintf

/* current version of the BACnet stack */
static const char *BACnet_Version = BACNET_VERSION_TEXT;

static void print_usage(const char *filename)
{
    PRINTF("Usage: %s [device-instance]\n", filename);
    PRINTF("       [object-type] [object-instance]\n");
    PRINTF("       [--device][--print-seconds]\n");
    PRINTF("       [--version][--help]\n");
}

static void print_help(const char *filename)
{
    PRINTF("Simulate a BACnet server-client device.\n");
    PRINTF("device-instance:\n"
           "BACnet Device Object Instance number that you are\n"
           "trying to communicate to.  This number will be used\n"
           "to try and bind with the device using Who-Is and\n"
           "I-Am services.  For example, if you were reading\n"
           "Device Object 123, the device-instance would be 123.\n");
    PRINTF("\n");
    PRINTF("object-type:\n"
           "The object type is object that you are reading. It\n"
           "can be defined either as the object-type name string\n"
           "as defined in the BACnet specification, or as the\n"
           "integer value of the enumeration BACNET_OBJECT_TYPE\n"
           "in bacenum.h. For example if you were reading Analog\n"
           "Output 2, the object-type would be analog-output or 1.\n");
    PRINTF("\n");
    PRINTF("object-instance:\n"
           "This is the object instance number of the object that\n"
           "you are reading.  For example, if you were reading\n"
           "Analog Output 2, the object-instance would be 2.\n");
    PRINTF("\n");
    PRINTF("Example:\n"
           "If you want read the Present-Value of Analog Output 101\n"
           "in Device 123, you could send either of the following\n"
           "commands:\n"
           "%s 123 analog-output 101\n"
           "%s 123 1 101\n"
           "If you want read the Present-Value of Binary Input 1\n"
           "in Device 123, you could send either of the following\n"
           "commands:\n"
           "%s 123 binary-input 1\n"
           "%s 123 3 1\n",
        filename, filename, filename, filename);
}

/**
 * @brief Main function of server-client demo.
 * @param argc [in] Arg count.
 * @param argv [in] Takes one argument: the Device Instance #.
 * @return 0 on success.
 */
int main(int argc, char *argv[])
{
    int argi = 0;
    unsigned int target_args = 0;
    const char *filename = NULL;
    uint32_t device_id = BACNET_MAX_INSTANCE;
    struct mstimer print_value_timer = { 0 };
    float float_value = 0.0;
    bool bool_value = false;
    unsigned object_type = 0;
    uint32_t unsigned_value = 0;
    /* data from the command line */
    unsigned long print_seconds = 10;
    uint32_t target_device_object_instance = BACNET_MAX_INSTANCE;
    uint32_t target_object_instance = BACNET_MAX_INSTANCE;
    BACNET_OBJECT_TYPE target_object_type = OBJECT_ANALOG_INPUT;

    filename = filename_remove_path(argv[0]);
    for (argi = 1; argi < argc; argi++) {
        if (strcmp(argv[argi], "--help") == 0) {
            print_usage(filename);
            print_help(filename);
            return 0;
        }
        if (strcmp(argv[argi], "--version") == 0) {
            PRINTF("%s %s\n", filename, BACNET_VERSION_TEXT);
            PRINTF("Copyright (C) 2022 by Steve Karg and others.\n"
                   "This is free software; see the source for copying "
                   "conditions.\n"
                   "There is NO warranty; not even for MERCHANTABILITY or\n"
                   "FITNESS FOR A PARTICULAR PURPOSE.\n");
            return 0;
        }
        if (strcmp(argv[argi], "--device") == 0) {
            if (++argi < argc) {
                device_id = strtol(argv[argi], NULL, 0);
                if (device_id > BACNET_MAX_INSTANCE) {
                    fprintf(stderr, "device=%u - not greater than %u\n",
                        device_id, BACNET_MAX_INSTANCE);
                    return 1;
                }
            }
        } else if (strcmp(argv[argi], "--print-seconds") == 0) {
            if (++argi < argc) {
                print_seconds = strtol(argv[argi], NULL, 0);
            }
        } else {
            if (target_args == 0) {
                target_device_object_instance = strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 1) {
                if (bactext_object_type_strtol(argv[argi], &object_type) ==
                    false) {
                    fprintf(stderr, "object-type=%s invalid\n", argv[argi]);
                    return 1;
                }
                target_object_type = object_type;
                target_args++;
            } else if (target_args == 2) {
                target_object_instance = strtol(argv[argi], NULL, 0);
                target_args++;
            } else {
                print_usage(filename);
                return 1;
            }
        }
    }
    if (target_args < 2) {
        print_usage(filename);
        return 0;
    }
    Device_Set_Object_Instance_Number(device_id);
    if (target_device_object_instance > BACNET_MAX_INSTANCE) {
        fprintf(stderr, "device-instance=%u - not greater than %u\n",
            target_device_object_instance, BACNET_MAX_INSTANCE);
        return 1;
    }
    PRINTF("BACnet Server-Client Demo\n"
           "BACnet Stack Version %s\n"
           "BACnet Device ID: %u\n"
           "Max APDU: %d\n",
        BACnet_Version, Device_Object_Instance_Number(), MAX_APDU);
    fflush(stdout);
    dlenv_init();
    atexit(datalink_cleanup);
    bacnet_task_init();
    bacnet_data_poll_seconds_set(print_seconds);
    if (!bacnet_data_object_add(target_device_object_instance,
            target_object_type, target_object_instance)) {
        return 1;
    }
    mstimer_set(&print_value_timer, print_seconds * 1000);
    /* loop forever */
    for (;;) {
        bacnet_task();
        if (mstimer_expired(&print_value_timer)) {
            mstimer_reset(&print_value_timer);
            switch (target_object_type) {
                case OBJECT_ANALOG_INPUT:
                case OBJECT_ANALOG_OUTPUT:
                case OBJECT_ANALOG_VALUE:
                    if (bacnet_data_analog_present_value(
                            target_device_object_instance, target_object_type,
                            target_object_instance, &float_value)) {
                        PRINTF("Device %u %s-%u=%f\n",
                            (unsigned)target_device_object_instance,
                            bactext_object_type_name(target_object_type),
                            (unsigned)target_object_instance, float_value);
                    }
                    break;
                case OBJECT_BINARY_INPUT:
                case OBJECT_BINARY_OUTPUT:
                case OBJECT_BINARY_VALUE:
                    if (bacnet_data_binary_present_value(
                            target_device_object_instance, target_object_type,
                            target_object_instance, &bool_value)) {
                        PRINTF("Device %u %s-%u=%s\n",
                            (unsigned)target_device_object_instance,
                            bactext_object_type_name(target_object_type),
                            (unsigned)target_object_instance,
                            bool_value ? "active" : "inactive");
                    }
                    break;
                case OBJECT_MULTI_STATE_INPUT:
                case OBJECT_MULTI_STATE_OUTPUT:
                case OBJECT_MULTI_STATE_VALUE:
                    if (bacnet_data_multistate_present_value(
                            target_device_object_instance, target_object_type,
                            target_object_instance, &unsigned_value)) {
                        PRINTF("Device %u %s-%u=%u\n",
                            (unsigned)target_device_object_instance,
                            bactext_object_type_name(target_object_type),
                            (unsigned)target_object_instance,
                            (unsigned)unsigned_value);
                    }
                    break;
                default:
                    return 1;
                    break;
            }
        }
    }

    return 0;
}
