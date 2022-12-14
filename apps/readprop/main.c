/*************************************************************************
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

/* command line tool that sends a BACnet service, and displays the reply */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h> /* for time */
#ifdef __STDC_ISO_10646__
#include <locale.h>
#endif

#define PRINT_ENABLED 1

#include "bacnet/bacdef.h"
#include "bacnet/config.h"
#include "bacnet/bactext.h"
#include "bacnet/bacerror.h"
#include "bacnet/iam.h"
#include "bacnet/arf.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/binding/address.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/basic/object/device.h"
#include "bacport.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/whois.h"
#include "bacnet/version.h"
/* some demo stuff needed */
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/dlenv.h"
#include "bacnet/basic/object/netport.h"
#if defined(BACDL_BSC)
#include "bacnet/basic/object/sc_netport.h"
#include "bacnet/datalink/bsc/bsc-datalink.h"
#include "bacnet/datalink/bsc/bsc-event.h"
#endif

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* converted command line arguments */
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;
static uint32_t Target_Object_Instance = BACNET_MAX_INSTANCE;
static BACNET_OBJECT_TYPE Target_Object_Type = OBJECT_ANALOG_INPUT;
static BACNET_PROPERTY_ID Target_Object_Property = PROP_ACKED_TRANSITIONS;
static int32_t Target_Object_Index = BACNET_ARRAY_ALL;
/* the invoke id is needed to filter incoming messages */
static uint8_t Request_Invoke_ID = 0;
static BACNET_ADDRESS Target_Address;
static bool Error_Detected = false;

#if defined(BACDL_BSC)
static uint8_t *Ca_Certificate = NULL;
static uint8_t *Certificate = NULL;
static uint8_t *Key = NULL;

#define SC_NETPORT_BACFILE_START_INDEX    0

#if defined(MAX_BACFILES) && (MAX_BACFILES < SC_NETPORT_BACFILE_START_INDEX + 3)
#error "BACFILE must save at least 3 files"
#endif

#endif /* BACDL_BSC */

static void MyErrorHandler(BACNET_ADDRESS *src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf("BACnet Error: %s: %s\n",
            bactext_error_class_name((int)error_class),
            bactext_error_code_name((int)error_code));
        Error_Detected = true;
    }
}

static void MyAbortHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t abort_reason, bool server)
{
    (void)server;
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf(
            "BACnet Abort: %s\n", bactext_abort_reason_name((int)abort_reason));
        Error_Detected = true;
    }
}

static void MyRejectHandler(
    BACNET_ADDRESS *src, uint8_t invoke_id, uint8_t reject_reason)
{
    if (address_match(&Target_Address, src) &&
        (invoke_id == Request_Invoke_ID)) {
        printf("BACnet Reject: %s\n",
            bactext_reject_reason_name((int)reject_reason));
        Error_Detected = true;
    }
}

/** Handler for a ReadProperty ACK.
 * @ingroup DSRP
 * Doesn't actually do anything, except, for debugging, to
 * print out the ACK data of a matching request.
 *
 * @param service_request [in] The contents of the service request.
 * @param service_len [in] The length of the service_request.
 * @param src [in] BACNET_ADDRESS of the source of the message
 * @param service_data [in] The BACNET_CONFIRMED_SERVICE_DATA information
 *                          decoded from the APDU header of this message.
 */
static void My_Read_Property_Ack_Handler(uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data)
{
    int len = 0;
    BACNET_READ_PROPERTY_DATA data;

    if (address_match(&Target_Address, src) &&
        (service_data->invoke_id == Request_Invoke_ID)) {
        len =
            rp_ack_decode_service_request(service_request, service_len, &data);
        if (len < 0) {
            printf("<decode failed!>\n");
        } else {
            rp_ack_print_data(&data);
        }
    }
}

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
    /* handle the data coming back from confirmed requests */
    apdu_set_confirmed_ack_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, My_Read_Property_Ack_Handler);
    /* handle any errors coming back */
    apdu_set_error_handler(SERVICE_CONFIRMED_READ_PROPERTY, MyErrorHandler);
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

static void print_usage(char *filename)
{
    printf("Usage: %s device-instance object-type object-instance "
           "property [index]\n",
        filename);
#if defined(BACDL_BSC)
    printf("       [--dnet][--dadr][--mac][--sc]\n");
#else
    printf("       [--dnet][--dadr][--mac]\n");
#endif
    printf("       [--version][--help]\n");
}

static void print_help(char *filename)
{
    printf("Read a property from an object in a BACnet device\n"
           "and print the value.\n");
    printf("--mac A\n"
           "Optional BACnet mac address."
           "Valid ranges are from 00 to FF (hex) for MS/TP or ARCNET,\n"
           "or an IP string with optional port number like 10.1.2.3:47808\n"
           "or an Ethernet MAC in hex like 00:21:70:7e:32:bb\n"
           "\n"
           "--dnet N\n"
           "Optional BACnet network number N for directed requests.\n"
           "Valid range is from 0 to 65535 where 0 is the local connection\n"
           "and 65535 is network broadcast.\n"
           "\n"
           "--dadr A\n"
           "Optional BACnet mac address on the destination BACnet network "
           "number.\n"
           "Valid ranges are from 00 to FF (hex) for MS/TP or ARCNET,\n"
           "or an IP string with optional port number like 10.1.2.3:47808\n"
           "or an Ethernet MAC in hex like 00:21:70:7e:32:bb\n"
#if defined(BACDL_BSC)
           "\n"
           "--sc hub-url dest-url ca-cert cert key\n"
           "Use the BACnet/SC hub connection.\n"
           "hub-url - destination URL like wss://127.0.0.1:50000\n"
           "ca-cert - filename of CA certificate\n"
           "cert - filename of device certificate\n"
           "key - filename of device certificate key\n"
#endif
           "\n");
    printf("device-instance:\n"
           "BACnet Device Object Instance number that you are\n"
           "trying to communicate to.  This number will be used\n"
           "to try and bind with the device using Who-Is and\n"
           "I-Am services.  For example, if you were reading\n"
           "Device Object 123, the device-instance would be 123.\n"
           "\nobject-type:\n"
           "The object type is object that you are reading. It\n"
           "can be defined either as the object-type name string\n"
           "as defined in the BACnet specification, or as the\n"
           "integer value of the enumeration BACNET_OBJECT_TYPE\n"
           "in bacenum.h. For example if you were reading Analog\n"
           "Output 2, the object-type would be analog-output or 1.\n"
           "\nobject-instance:\n"
           "This is the object instance number of the object that\n"
           "you are reading.  For example, if you were reading\n"
           "Analog Output 2, the object-instance would be 2.\n"
           "\nproperty:\n"
           "The property of the object that you are reading. It\n"
           "can be defined either as the property name string as\n"
           "defined in the BACnet specification, or as an integer\n"
           "value of the enumeration BACNET_PROPERTY_ID in\n"
           "bacenum.h. For example, if you were reading the Present\n"
           "Value property, use present-value or 85 as the property.\n"
           "\nindex:\n"
           "This integer parameter is the index number of an array.\n"
           "If the property is an array, individual elements can\n"
           "be read.  If this parameter is missing and the property\n"
           "is an array, the entire array will be read.\n"
           "\nExample:\n"
           "If you want read the Present-Value of Analog Output 101\n"
           "in Device 123, you could send either of the following\n"
           "commands:\n"
           "%s 123 analog-output 101 present-value\n"
           "%s 123 1 101 85\n"
           "If you want read the Priority-Array of Analog Output 101\n"
           "in Device 123, you could send either of the following\n"
           "commands:\n"
           "%s 123 analog-output 101 priority-array\n"
           "%s 123 1 101 87\n",
        filename, filename, filename, filename);
}

#if defined(BACDL_BSC)

static uint32_t read_file(char *filename, uint8_t **buff)
{
    uint32_t size = 0;
    FILE *pFile;

    pFile = fopen(filename, "rb");
    if (pFile) {
        fseek(pFile, 0L, SEEK_END);
        size = ftell(pFile);
        fseek(pFile, 0, SEEK_SET);

        *buff = (uint8_t *)malloc(size);
        if (*buff != NULL) {
            if (fread(*buff, size, 1, pFile) == 0) {
                size = 0;
            }
        }
        fclose(pFile);
    }
    return *buff ? size : 0;
}

static void init_bsc(char *hub_url, char *filename_ca_cert, char *filename_cert,
    char *filename_key)
{
    uint32_t instance = 1;
    uint32_t size;

    Network_Port_Object_Instance_Number_Set(0, instance);

    size = read_file(filename_ca_cert, &Ca_Certificate);
    Network_Port_Issuer_Certificate_File_Set_From_Memory(instance, 0,
        Ca_Certificate, size, SC_NETPORT_BACFILE_START_INDEX);

    size = read_file(filename_cert, &Certificate);
    Network_Port_Operational_Certificate_File_Set_From_Memory(instance,
        Certificate, size, SC_NETPORT_BACFILE_START_INDEX + 1);

    size = read_file(filename_key, &Key);
    Network_Port_Certificate_Key_File_Set_From_Memory(instance,
        Key, size, SC_NETPORT_BACFILE_START_INDEX + 2);

    Network_Port_SC_Primary_Hub_URI_Set(instance, hub_url);
    Network_Port_SC_Failover_Hub_URI_Set(instance, hub_url);

    Network_Port_SC_Direct_Connect_Initiate_Enable_Set(instance, true);
    Network_Port_SC_Direct_Connect_Accept_Enable_Set(instance,  false);
    Network_Port_SC_Direct_Server_Port_Set(instance, 9999);
    //Network_Port_SC_Direct_Connect_Accept_URIs_Set(instance, hub_url);

    Network_Port_SC_Hub_Function_Enable_Set(instance, false);
}

#endif

int main(int argc, char *argv[])
{
    BACNET_ADDRESS src = { 0 }; /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 100; /* milliseconds */
    unsigned max_apdu = 0;
    time_t elapsed_seconds = 0;
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    time_t timeout_seconds = 0;
    bool found = false;
    long dnet = -1;
    BACNET_MAC_ADDRESS mac = { 0 };
    BACNET_MAC_ADDRESS adr = { 0 };
    BACNET_ADDRESS dest = { 0 };
    bool specific_address = false;
    int argi = 0;
    unsigned object_type = 0;
    unsigned object_property = 0;
    unsigned int target_args = 0;
    char *filename = NULL;

#if defined(BACDL_BSC)
    bool use_sc = false;
    char *hub_url = NULL;
    char *filename_ca_cert = NULL;
    char *filename_cert = NULL;
    char *filename_key = NULL;
#endif

    filename = filename_remove_path(argv[0]);
    for (argi = 1; argi < argc; argi++) {
        if (strcmp(argv[argi], "--help") == 0) {
            print_usage(filename);
            print_help(filename);
            return 0;
        }
        if (strcmp(argv[argi], "--version") == 0) {
            printf("%s %s\n", filename, BACNET_VERSION_TEXT);
            printf("Copyright (C) 2015 by Steve Karg and others.\n"
                   "This is free software; see the source for copying "
                   "conditions.\n"
                   "There is NO warranty; not even for MERCHANTABILITY or\n"
                   "FITNESS FOR A PARTICULAR PURPOSE.\n");
            return 0;
        }
        if (strcmp(argv[argi], "--mac") == 0) {
            if (++argi < argc) {
                if (address_mac_from_ascii(&mac, argv[argi])) {
                    specific_address = true;
                }
            }
        } else if (strcmp(argv[argi], "--dnet") == 0) {
            if (++argi < argc) {
                dnet = strtol(argv[argi], NULL, 0);
                if ((dnet >= 0) && (dnet <= BACNET_BROADCAST_NETWORK)) {
                    specific_address = true;
                }
            }
        } else if (strcmp(argv[argi], "--dadr") == 0) {
            if (++argi < argc) {
                if (address_mac_from_ascii(&adr, argv[argi])) {
                    specific_address = true;
                }
            }
#if defined(BACDL_BSC)
        } else if (strcmp(argv[argi], "--sc") == 0) {
            use_sc = true;
            if (++argi < argc) {
                hub_url = argv[argi];
            }
            if (++argi < argc) {
                filename_ca_cert = argv[argi];
            }
            if (++argi < argc) {
                filename_cert = argv[argi];
            }
            if (++argi < argc) {
                filename_key = argv[argi];
            }
#endif
        } else {
            if (target_args == 0) {
                Target_Device_Object_Instance = strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 1) {
                if (bactext_object_type_strtol(argv[argi], &object_type) ==
                    false) {
                    fprintf(stderr, "object-type=%s invalid\n", argv[argi]);
                    return 1;
                }
                Target_Object_Type = object_type;
                target_args++;
            } else if (target_args == 2) {
                Target_Object_Instance = strtol(argv[argi], NULL, 0);
                target_args++;
            } else if (target_args == 3) {
                if (bactext_property_strtol(argv[argi], &object_property) ==
                    false) {
                    fprintf(stderr, "property=%s invalid\n", argv[argi]);
                    return 1;
                }
                Target_Object_Property = object_property;
                target_args++;
            } else if (target_args == 4) {
                Target_Object_Index = strtol(argv[argi], NULL, 0);
                target_args++;
            } else {
                print_usage(filename);
                return 1;
            }
        }
    }
    if (target_args < 4) {
        print_usage(filename);
        return 0;
    }
    if (Target_Device_Object_Instance > BACNET_MAX_INSTANCE) {
        fprintf(stderr, "device-instance=%u - it must be less than %u\n",
            Target_Device_Object_Instance, BACNET_MAX_INSTANCE);
        return 1;
    }
    address_init();
    if (specific_address) {
        if (adr.len && mac.len) {
            memcpy(&dest.mac[0], &mac.adr[0], mac.len);
            dest.mac_len = mac.len;
            memcpy(&dest.adr[0], &adr.adr[0], adr.len);
            dest.len = adr.len;
            if ((dnet >= 0) && (dnet <= BACNET_BROADCAST_NETWORK)) {
                dest.net = dnet;
            } else {
                dest.net = BACNET_BROADCAST_NETWORK;
            }
        } else if (mac.len) {
            memcpy(&dest.mac[0], &mac.adr[0], mac.len);
            dest.mac_len = mac.len;
            dest.len = 0;
            if ((dnet >= 0) && (dnet <= BACNET_BROADCAST_NETWORK)) {
                dest.net = dnet;
            } else {
                dest.net = 0;
            }
        } else {
            if ((dnet >= 0) && (dnet <= BACNET_BROADCAST_NETWORK)) {
                dest.net = dnet;
            } else {
                dest.net = BACNET_BROADCAST_NETWORK;
            }
            dest.mac_len = 0;
            dest.len = 0;
        }
        address_add(Target_Device_Object_Instance, MAX_APDU, &dest);
    }
    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    Init_Service_Handlers();
#if defined(BACDL_BSC)
    if (use_sc) {
        init_bsc(hub_url, filename_ca_cert, filename_cert, filename_key);
    }
#endif
    dlenv_init();

#if defined(BACDL_BSC)
    while(bsc_hub_connection_status()==BVLC_SC_HUB_CONNECTION_ABSENT) {
        bsc_wait(1);
    }
#endif

#ifdef __STDC_ISO_10646__
    /* Internationalized programs must call setlocale()
     * to initiate a specific language operation.
     * This can be done by calling setlocale() as follows.
     * If your native locale doesn't use UTF-8 encoding
     * you need to replace the empty string with a
     * locale like "en_US.utf8" which is the same as the string
     * used in the enviromental variable "LANG=en_US.UTF-8".
     */
    setlocale(LC_ALL, "");
#endif
    atexit(datalink_cleanup);
    /* configure the timeout values */
    last_seconds = time(NULL);
    timeout_seconds = (apdu_timeout() / 1000) * apdu_retries();
    /* try to bind with the device */
    found = address_bind_request(
        Target_Device_Object_Instance, &max_apdu, &Target_Address);
    if (!found) {
        Send_WhoIs(
            Target_Device_Object_Instance, Target_Device_Object_Instance);
    }
    /* loop forever */
    for (;;) {
        /* increment timer - exit if timed out */
        current_seconds = time(NULL);

        /* at least one second has passed */
        if (current_seconds != last_seconds) {
            tsm_timer_milliseconds(
                (uint16_t)((current_seconds - last_seconds) * 1000));
        }
        if (Error_Detected) {
            break;
        }
        /* wait until the device is bound, or timeout and quit */
        if (!found) {
            found = address_bind_request(
                Target_Device_Object_Instance, &max_apdu, &Target_Address);
        }
        if (found) {
            if (Request_Invoke_ID == 0) {
                Request_Invoke_ID =
                    Send_Read_Property_Request(Target_Device_Object_Instance,
                        Target_Object_Type, Target_Object_Instance,
                        Target_Object_Property, Target_Object_Index);
            } else if (tsm_invoke_id_free(Request_Invoke_ID)) {
                break;
            } else if (tsm_invoke_id_failed(Request_Invoke_ID)) {
                fprintf(stderr, "\rError: TSM Timeout!\n");
                tsm_free_invoke_id(Request_Invoke_ID);
                Error_Detected = true;
                /* try again or abort? */
                break;
            }
        } else {
            /* increment timer - exit if timed out */
            elapsed_seconds += (current_seconds - last_seconds);
            if (elapsed_seconds > timeout_seconds) {
                printf("\rError: APDU Timeout!\n");
                Error_Detected = true;
                break;
            }
        }

        /* returns 0 bytes on timeout */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);

        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len);
        }

        /* keep track of time for next check */
        last_seconds = current_seconds;
    }

#if defined(BACDL_BSC)
    free(Ca_Certificate);
    free(Certificate);
    free(Key);
#endif

    if (Error_Detected) {
        return 1;
    }

    return 0;
}
