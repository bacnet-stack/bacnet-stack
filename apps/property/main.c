/**
 * @file
 * @author Steve Karg
 * @date October 2022
 * @brief simple program to parse BACnet value data and print decoded meanings
 *
 * @section LICENSE
 *
 * Copyright (C) 2022 Steve Karg
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to:
 * The Free Software Foundation, Inc.
 * 59 Temple Place - Suite 330
 * Boston, MA  02111-1307
 * USA.
 *
 * As a special exception, if other files instantiate templates or
 * use macros or inline functions from this file, or you compile
 * this file and link it with other works to produce a work based
 * on this file, this file does not by itself cause the resulting
 * work to be covered by the GNU General Public License. However
 * the source code for this file must still be made available in
 * accordance with section (3) of the GNU General Public License.
 *
 * This exception does not invalidate any other reasons why a work
 * based on this file might be covered by the GNU General Public
 * License.
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include "bacnet/bactext.h"
#include "bacnet/bytes.h"
#include "bacnet/version.h"
#include "bacnet/datetime.h"
/* basic timer */
#include "bacnet/basic/sys/mstimer.h"
/* specific service includes */
#include "bacnet/basic/services.h"

/* buffer needed by property value */
static BACNET_OCTET_STRING Octet_String;
/* flags needed for options */
static bool ASCII_Decimal = false;
/* optional data for specific decoding */
static uint32_t Target_Object_Instance = BACNET_MAX_INSTANCE;
static BACNET_OBJECT_TYPE Target_Object_Type = OBJECT_ANALOG_INPUT;
static BACNET_PROPERTY_ID Target_Object_Property = PROP_ACKED_TRANSITIONS;
static int32_t Target_Object_Index = BACNET_ARRAY_ALL;

/**
 * @brief Takes the arguments passed by the main function and sets flags
 *  if it matches one of the predefined args.
 * @param argc (IN) number of arguments.
 * @param argv (IN) an array of arguments in string form.
 */
static void Parse_Arguments(int argc, char *argv[])
{
    int argi = 0;
    bool status = false;

    for (argi = 1; argi < argc; argi++) {
        if (argv[argi][0] == '-') {
            /* dash arguments */
            switch (argv[argi][1]) {
                case 'x':
                case 'X':
                    ASCII_Decimal = false;
                    break;
                case 'd':
                case 'D':
                    ASCII_Decimal = true;
                    break;
                case '-':
                    if (strcmp(argv[argi], "--instance") == 0) {
                        if (++argi < argc) {
                            Target_Object_Instance =
                                strtol(argv[argi], NULL, 0);
                            if (Target_Object_Instance < 0) {
                                fprintf(stderr,
                                    "--instance=%s - it must positive\n",
                                    argv[argi]);
                                exit(1);
                            } else if (Target_Object_Instance >
                                BACNET_MAX_INSTANCE) {
                                fprintf(stderr,
                                    "--instance=%s - it must be less than %u\n",
                                    argv[argi], BACNET_MAX_INSTANCE);
                                exit(1);
                            }

                        }
                    } else if (strcmp(argv[argi], "--object-type") == 0) {
                        if (++argi < argc) {
                            status = bactext_object_type_strtol(
                                argv[argi], &Target_Object_Type);
                            if (!status) {
                                fprintf(stderr, "--object-type=%s invalid\n",
                                    argv[argi]);
                                exit(1);
                            }
                        }
                    } else if (strcmp(argv[argi], "--property") == 0) {
                        if (++argi < argc) {
                            status = bactext_property_strtol(argv[argi],
                                &Target_Object_Property);
                            if (!status) {
                                fprintf(stderr, "--property=%s invalid\n",
                                    argv[argi]);
                                exit(1);
                            }
                        }
                    } else if (strcmp(argv[argi], "--index") == 0) {
                        if (++argi < argc) {
                            Target_Object_Index = strtol(argv[argi], NULL, 0);
                        }
                    }
                    break;
                default:
                    break;
            }
        } else {
            octetstring_init_ascii_hex(&Octet_String, argv[argi]);
        }
    }
}

/**
 * @brief print the application encoded property value
 * @param application_data encoded application data
 * @param application_data_len size of the encoded application data
 */
static void print_property_value(
    uint8_t *application_data,
    int application_data_len)
{
    BACNET_OBJECT_PROPERTY_VALUE object_value = {};
    BACNET_APPLICATION_DATA_VALUE value = {};
    int len = 0;
    bool first_value = true;
    bool print_brace = false;

    if (application_data) {
        /* value? need to loop until all of the len is gone... */
        while (len < application_data_len) {
            len = bacapp_decode_application_data(
                application_data, (unsigned)application_data_len, &value);
            if (first_value && (len < application_data_len)) {
                first_value = false;
                fprintf(stdout, "{");
                print_brace = true;
            }
            object_value.object_type = Target_Object_Type;
            object_value.object_instance = Target_Object_Instance;
            object_value.object_property = Target_Object_Property;
            object_value.array_index = Target_Object_Index;
            object_value.value = &value;
            bacapp_print_value(stdout, &object_value);
            if (len > 0) {
                if (len < application_data_len) {
                    application_data += len;
                    application_data_len -= len;
                    /* there's more! */
                    fprintf(stdout, ",");
                } else {
                    break;
                }
            } else {
                break;
            }
        }
        if (print_brace) {
            fprintf(stdout, "}");
        }
        fprintf(stdout, "\r\n");
    }
}

/**************************************************************************
* Description: The starting point of the C program
* Returns: none
* Notes: called from crt.s module
**************************************************************************/
int main(int argc, char *argv[])
{
    /* initialize our interface */
    if ((argc > 1) && (strcmp(argv[1], "--help") == 0)) {
        printf(
            "bacprop [options] <75 07 00 4c 4f 4c 43 50 32>\r\n"
            "options:\r\n"
            "[-x] interprete the arguments as ascii hex (default)\r\n"
            "[-d] interprete the argument as ascii decimal\r\n");
        return 0;
    }
    if ((argc > 1) && (strcmp(argv[1], "--version") == 0)) {
        printf("bacprop %s\r\n", BACNET_VERSION_TEXT);
        printf(
            "Copyright (C) 2022 by Steve Karg\r\n"
            "This is free software; see the source for copying conditions.\r\n"
            "There is NO warranty; not even for MERCHANTABILITY or\r\n"
            "FITNESS FOR A PARTICULAR PURPOSE.\r\n");
        return 0;
    }
    Parse_Arguments(argc, argv);
    print_property_value(
        octetstring_value(&Octet_String),
        octetstring_length(&Octet_String));

    return 1;
}
