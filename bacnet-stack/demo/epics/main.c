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

/** @file epics/main.c  Command line tool to build a list of Objects and
 *                      Properties that can be used with VTS3 EPICS files. */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>       /* for time */
#include <errno.h>
#include <assert.h>
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
#include "rp.h"
/* some demo stuff needed */
#include "filename.h"
#include "handlers.h"
#include "client.h"
#include "txbuf.h"
#include "dlenv.h"
#include "keylist.h"
#include "bacepics.h"


/* (Doxygen note: The next two lines pull all the following Javadoc 
 *  into the BACEPICS module.) */
/** @addtogroup BACEPICS 
 * @{ */

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* target information converted from command line */
static uint32_t Target_Device_Object_Instance = BACNET_MAX_INSTANCE;
static BACNET_ADDRESS Target_Address;
/* any errors are picked up in main loop */
static bool Error_Detected = false;
static EPICS_STATES myState = INITIAL_BINDING;

/* any valid RP or RPM data returned is put here */
/* Now using one structure for both RP and RPM data:
 * typedef struct BACnet_RP_Service_Data_t {
 *   bool new_data;
 *   BACNET_CONFIRMED_SERVICE_ACK_DATA service_data;
 *   BACNET_READ_PROPERTY_DATA data;
 *   } BACNET_RP_SERVICE_DATA;
 * static BACNET_RP_SERVICE_DATA Read_Property_Data;
 */

typedef struct BACnet_RPM_Service_Data_t {
    bool new_data;
    BACNET_CONFIRMED_SERVICE_ACK_DATA service_data;
    BACNET_READ_ACCESS_DATA *rpm_data;
} BACNET_RPM_SERVICE_DATA;
static BACNET_RPM_SERVICE_DATA Read_Property_Multiple_Data;

/* We get the length of the object list,
   and then get the objects one at a time */
static uint32_t Object_List_Length = 0;
static int32_t Object_List_Index = 0;
/* object that we are currently printing */
static OS_Keylist Object_List;

/* When we need to process an Object's properties one at a time,
 * then we build and use this list */
#define MAX_PROPS 100		/* Supersized so it always is big enough. */
static uint32_t Property_List_Length = 0;
static uint32_t Property_List_Index = 0;
static int32_t Property_List[MAX_PROPS + 2];
/* This normally points to Property_List. */
static const int *pPropList = NULL;

/* When we have to walk through an array of things, like ObjectIDs or
 * Subordinate_Annotations, one RP call at a time, use these for indexing. */
static uint32_t Walked_List_Length = 0;
static uint32_t Walked_List_Index = 0;
static bool Using_Walked_List = false;

static bool ShowValues = false;		/* Show value instead of '?' */

#if !defined(PRINT_ERRORS)
#define PRINT_ERRORS 1
#endif

static void MyErrorHandler(
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    BACNET_ERROR_CLASS error_class,
    BACNET_ERROR_CODE error_code)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
#if PRINT_ERRORS
    printf("BACnet Error: %s: %s\r\n", bactext_error_class_name(error_class),
        bactext_error_code_name(error_code));
#else
    (void) error_class;
    (void) error_code;
#endif
    Error_Detected = true;
}

void MyAbortHandler(
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    uint8_t abort_reason,
    bool server)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
    (void) server;
#if PRINT_ERRORS
    /* It is normal for this to fail, so don't print. */
    if ( myState != GET_ALL_RESPONSE )
    	printf("BACnet Abort: %s\r\n", bactext_abort_reason_name(abort_reason));
#else
    (void) abort_reason;
#endif
    Error_Detected = true;
}

void MyRejectHandler(
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    uint8_t reject_reason)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
#if PRINT_ERRORS
    printf("BACnet Reject: %s\r\n", bactext_reject_reason_name(reject_reason));
#else
    (void) reject_reason;
#endif
    Error_Detected = true;
}

void MyReadPropertyAckHandler(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA * service_data)
{
    int len = 0;
    BACNET_READ_ACCESS_DATA *rp_data;

    (void) src;
    rp_data = calloc(1, sizeof(BACNET_READ_ACCESS_DATA));
    if (rp_data) {
        len =
            rp_ack_fully_decode_service_request(service_request, service_len,
											    rp_data);
    }
    if (len > 0) {
        memmove(&Read_Property_Multiple_Data.service_data, service_data,
        		sizeof(BACNET_CONFIRMED_SERVICE_ACK_DATA));
        Read_Property_Multiple_Data.rpm_data = rp_data;
        Read_Property_Multiple_Data.new_data = true;
    }
    else
    {
    	if ( len < 0 )		/* Eg, failed due to no segmentation */
    		Error_Detected = true;
    	free( rp_data );
    }
}

void MyReadPropertyMultipleAckHandler(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA * service_data)
{
    int len = 0;
    BACNET_READ_ACCESS_DATA *rpm_data;

    (void) src;
    rpm_data = calloc(1, sizeof(BACNET_READ_ACCESS_DATA));
    if (rpm_data) {
        len =
            rpm_ack_decode_service_request(service_request, service_len,
											rpm_data);
    }
    if (len > 0) {
        memmove(&Read_Property_Multiple_Data.service_data, service_data,
        		sizeof(BACNET_CONFIRMED_SERVICE_ACK_DATA));
        Read_Property_Multiple_Data.rpm_data = rpm_data;
        Read_Property_Multiple_Data.new_data = true;
        /* Will process and free the RPM data later */
    }
    else
    {
    	if ( len < 0 )		/* Eg, failed due to no segmentation */
    		Error_Detected = true;
    	free( rpm_data );
    }
 }

static void Init_Service_Handlers(
    void)
{
    Device_Init();
    /* we need to handle who-is
       to support dynamic device binding to us */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    /* handle i-am to support binding to other devices */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, handler_i_am_bind);
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler
        (handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
								handler_read_property);
    /* handle the data coming back from confirmed requests */
    apdu_set_confirmed_ack_handler(SERVICE_CONFIRMED_READ_PROPERTY,
									MyReadPropertyAckHandler);
    apdu_set_confirmed_ack_handler(SERVICE_CONFIRMED_READ_PROP_MULTIPLE,
									MyReadPropertyMultipleAckHandler);
    /* handle any errors coming back */
    apdu_set_error_handler(SERVICE_CONFIRMED_READ_PROPERTY, MyErrorHandler);
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}


/** Provide a nicer output for Supported Services and Object Types bitfields.
 * We have to override the library's normal bitfield print because the
 * EPICS format wants just T and F, and we want to provide (as comments)
 * the names of the active types.
 * These bitfields use opening and closing parentheses instead of braces.
 * We also limit the output to 4 bit fields per line.
 * 
 * @param stream [in] Normally stdout
 * @param value [in] The structure holding this property's value (union) and type.
 * @param property [in] Which property we are printing.
 * @return True if success.  Or otherwise.
 */
 
 bool PrettyPrintPropertyValue(
    FILE * stream,
    BACNET_APPLICATION_DATA_VALUE * value,
    BACNET_PROPERTY_ID property)
{
    bool status = true; /*return value */
    size_t len = 0, i = 0, j = 0;

    if ( (value != NULL) && (value->tag == BACNET_APPLICATION_TAG_BIT_STRING) &&
         ( (property == PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED) || 
           (property == PROP_PROTOCOL_SERVICES_SUPPORTED) ) )
     {
        len = bitstring_bits_used(&value->type.Bit_String);
        fprintf(stream, "( \r\n        ");
        for (i = 0; i < len; i++) {
            fprintf(stream, "%s",
                bitstring_bit(&value->type.Bit_String,
                    (uint8_t) i) ? "T" : "F");
            if (i < len - 1)
                fprintf(stream, ",");
            else
                fprintf(stream, " ");
            /* Tried with 8 per line, but with the comments, got way too long. */
            if ( (i == (len-1) ) || ( (i % 4) == 3 ) )       // line break every 4
            {
                fprintf(stream, "   -- ");		// EPICS comments begin with "--"
                /* Now rerun the same 4 bits, but print labels for true ones */
                for ( j = i - (i%4); j <= i; j++)
                {
                    if ( bitstring_bit(&value->type.Bit_String, (uint8_t) j) )
                    {
                        if (property == PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED) 
                            fprintf( stream, " %s,", bactext_object_type_name(j) );
                        /* PROP_PROTOCOL_SERVICES_SUPPORTED */
                        else
                        {
                            bool bIsConfirmed;
                            size_t newIndex;
                            if ( apdu_service_supported_to_index( j,
                                            &newIndex, &bIsConfirmed ) )
                            { 
                                if ( bIsConfirmed )   
                                    fprintf( stream, " %s,", 
                                         bactext_confirmed_service_name(newIndex) );
                            
                                else 
                                    fprintf( stream, " %s,", 
                                        bactext_unconfirmed_service_name( 
                                                        (newIndex) ) );
                               
                            }
                        }
                     }
                    else    /* not supported */
                        fprintf( stream, "," );
                }
                fprintf(stream, "\r\n        ");
           }
         }
        fprintf(stream, ") \r\n");
     }
     else if ( value != NULL )
     {
         assert( false );	/* How did I get here?  Fix your code. */
         /* Meanwhile, a fallback plan */
         status = bacapp_print_value(stdout, value, property);
     }
     else
         fprintf(stream, "? \r\n");

    return status;
}


/** Print out the value(s) for one Property.
 * This function may be called repeatedly for one property if we are walking
 * through a list (Using_Walked_List is True) to show just one value of the
 * array per call.
 *
 * @param rpm_property [in] Points to structure holding the Property,
 *                          Value, and Error information.
 */
void PrintReadPropertyData(
		BACNET_PROPERTY_REFERENCE *rpm_property)
{
    BACNET_APPLICATION_DATA_VALUE *value, *old_value;
    bool print_brace = false;
    KEY object_list_element;

    if (rpm_property == NULL )
    {
    	fprintf( stdout, "    -- Null Property data \r\n" );
    	return;
    }
    value = rpm_property->value;
    if ( value == NULL )
    {
    	/* Then we print the error information */
        fprintf(stdout, "?  -- BACnet Error: %s: %s\r\n",
            bactext_error_class_name((int) rpm_property->error.error_class),
            bactext_error_code_name((int) rpm_property->error.error_code));
    	return;
    }

#if 0
        if (data->array_index == BACNET_ARRAY_ALL)
            fprintf(stderr, "%s #%u %s\n",
                bactext_object_type_name(data->object_type),
                data->object_instance,
                bactext_property_name(data->object_property));
        else
            fprintf(stderr, "%s #%u %s[%d]\n",
                bactext_object_type_name(data->object_type),
                data->object_instance,
                bactext_property_name(data->object_property),
                data->array_index);
#endif
	if( ( value != NULL ) && ( value->next != NULL ) )
	{
		/* Then this is an array of values; open brace */
		fprintf(stdout, "{ ");
		print_brace = true;		/* remember to close it */
	}

	if ( !Using_Walked_List )
		Walked_List_Index = Walked_List_Length = 0;	/* In case we need this. */
	/* value(s) loop until there is no "next" ... */
	while ( value != NULL )
	{
		switch( rpm_property->propertyIdentifier )
		{
		case PROP_OBJECT_LIST:
		case PROP_STRUCTURED_OBJECT_LIST:
		case PROP_SUBORDINATE_LIST:
			if ( Using_Walked_List )
			{
				if ( (rpm_property->propertyArrayIndex == 0) &&
				     (value->tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) )
				{
					/* Grab the value of the Object List length - don't print it! */
					Walked_List_Length = value->type.Unsigned_Int;
					if ( rpm_property->propertyIdentifier == PROP_OBJECT_LIST)
						Object_List_Length = value->type.Unsigned_Int;
					break;
				}
				else
					assert( Walked_List_Index == rpm_property->propertyArrayIndex);
			}
			else
				Walked_List_Index++;
			if ( Walked_List_Index == 1 )
			{
				/* Open the list of Objects (opening brace may be already printed) */
				if( value->next == NULL )
					fprintf(stdout, "{ \r\n        ");
				else
					fprintf(stdout, "\r\n        ");
			}
			if ( value->tag != BACNET_APPLICATION_TAG_OBJECT_ID ) {
				assert( false );		/* Something not right here */
				break;
			}
			else if ( rpm_property->propertyIdentifier == PROP_OBJECT_LIST)
			{
				/* Store the object list so we can interrogate
				   each object. */
				object_list_element =
					KEY_ENCODE(value->type.Object_Id.type,
								value->type.Object_Id.instance);
				/* We don't have anything to put in the data pointer
				 * yet, so just leave it null.  The key is Key here. */
				Keylist_Data_Add( Object_List, object_list_element, NULL );
			}
			else if ( rpm_property->propertyIdentifier == PROP_SUBORDINATE_LIST)
			{
				/* TODO: handle Sequence of { Device ObjID, Object ID }, */
			}
			bacapp_print_value(stdout, value, rpm_property->propertyIdentifier );
			if ( ( Walked_List_Index < Walked_List_Length ) ||
				 ( value->next != NULL ) )
			{
				/* There are more. */
				fprintf(stdout, ",");
				if (!(Walked_List_Index % 4))
					fprintf(stdout, "\r\n        ");
			} else {
				fprintf(stdout, " } \r\n");
			}
			break;

		case PROP_PROTOCOL_OBJECT_TYPES_SUPPORTED:
		case PROP_PROTOCOL_SERVICES_SUPPORTED:
			PrettyPrintPropertyValue(stdout, value, rpm_property->propertyIdentifier);
			break;

		default:
			/* Some properties are presented just as '?' in an EPICS;
			 * screen these out here, unless ShowValues is true.  */
			switch( rpm_property->propertyIdentifier )
			{
			case PROP_DEVICE_ADDRESS_BINDING:
			case PROP_DAYLIGHT_SAVINGS_STATUS:
			case PROP_LOCAL_DATE:
			case PROP_LOCAL_TIME:
			case PROP_PRESENT_VALUE:
			case PROP_PRIORITY_ARRAY:
			case PROP_RELIABILITY:
			case PROP_UTC_OFFSET:
			case PROP_DATABASE_REVISION:
				if ( !ShowValues )
				{
					fprintf(stdout, "?");
					break;
				}
				/* Else, fall through and print value: */
			default:
				bacapp_print_value(stdout, value, rpm_property->propertyIdentifier);
				break;
			}
			if ( value->next != NULL ) {
				/* there's more! */
				fprintf(stdout, ",");
			}
			else
			{
				if (print_brace) {
					/* Closing brace for this multi-valued array */
					fprintf(stdout, " }");
				}
			fprintf(stdout, "\r\n");
			}
			break;
		}

		old_value = value;
		value = value->next;	/* next or NULL */
		free( old_value );
	}		/* End while loop */

}

/** Send an RP request to read one property from the current Object.
 * Singly process large arrays too, like the Device Object's Object_List.
 * If GET_LIST_OF_ALL_RESPONSE failed, we will fall back to using just
 * the list of known Required properties for this type of object.
 *
 * @param device_instance [in] Our target device's instance.
 * @param pMyObject [in] The current Object's type and instance numbers.
 * @return The invokeID of the message sent, or 0 if reached the end
 *         of the property list.
 */
static uint8_t Read_Properties(
    uint32_t device_instance,
    BACNET_OBJECT_ID *pMyObject )
{
    uint8_t invoke_id = 0;
    struct special_property_list_t PropertyListStruct;

    if ( Property_List_Length == 0 )
    {
    	/* If we failed to get the Properties with RPM, just settle for what we
    	 * know is the fixed list of Required (only) properties.
    	 * In practice, this should only happen for simple devices that don't
    	 * implement RPM or have really limited MAX_APDU size.
    	 */
    	Device_Objects_Property_List( pMyObject->type, &PropertyListStruct);
		pPropList = PropertyListStruct.Required.pList;
    	if ( pPropList != NULL )
    	{
    		Property_List_Length = PropertyListStruct.Required.count;
    	}
    	else
    	{
    		fprintf( stdout, "    -- No Properties available for %s \r\n",
    				bactext_object_type_name( pMyObject->type ) );
    	}
    }
    else
    	pPropList = Property_List;

    if ( (pPropList != NULL ) && ( pPropList[Property_List_Index] != -1) )
    {
    	int prop = pPropList[Property_List_Index];
		if ( Using_Walked_List )
        {
            if (Walked_List_Length == 0) {
                printf("    %s: ", bactext_property_name( prop ) );
                invoke_id =
                    Send_Read_Property_Request(device_instance,
								pMyObject->type, pMyObject->instance,
								prop, 0);
            } else {
                invoke_id =
                    Send_Read_Property_Request(device_instance,
								pMyObject->type, pMyObject->instance,
								prop, Walked_List_Index);
            }
        } else {
            printf("    %s: ", bactext_property_name( prop ) );
            invoke_id =
                Send_Read_Property_Request(device_instance,
						pMyObject->type, pMyObject->instance,
						prop, BACNET_ARRAY_ALL);
        }
    }

    return invoke_id;
}

/** Process the RPM list, either printing out on success or building a
 *  properties list for later use.
 *  Also need to free the data in the list.
 * @param rpm_data [in] The list of RPM data received.
 * @param myState [in] The current state.
 * @return The next state of the EPICS state machine, normally NEXT_OBJECT
 *         if the RPM got good data, or GET_PROPERTY_REQUEST if we have to
 *         singly process the list of Properties.
 */
EPICS_STATES ProcessRPMData(
							 BACNET_READ_ACCESS_DATA * rpm_data,
							 EPICS_STATES myState )
{
    BACNET_READ_ACCESS_DATA *old_rpm_data;
    BACNET_PROPERTY_REFERENCE *rpm_property;
    BACNET_PROPERTY_REFERENCE *old_rpm_property;
    BACNET_APPLICATION_DATA_VALUE *value;
    BACNET_APPLICATION_DATA_VALUE *old_value;
    bool bSuccess = true;
    EPICS_STATES nextState = myState;		/* assume no change */
    /* Some flags to keep the output "pretty" -
     * wait and put these object lists at the end */
    bool bHasObjectList = false;
    bool bHasStructuredViewList = false;

    while (rpm_data)
    {
        rpm_property = rpm_data->listOfProperties;
        while (rpm_property) {
            /* For the GET_LIST_OF_ALL_RESPONSE case,
             * just keep what property this was */
            if ( myState == GET_LIST_OF_ALL_RESPONSE )
            {
            	switch ( rpm_property->propertyIdentifier )
            	{
            	case PROP_OBJECT_LIST:
            		bHasObjectList = true;	/* Will append below */
            		break;
            	case PROP_STRUCTURED_OBJECT_LIST:
            		bHasStructuredViewList = true;
            		break;
            	default:
					Property_List[ Property_List_Index++ ] =
							rpm_property->propertyIdentifier;
					Property_List_Length++;
            		break;
            	}
            	/* Free up the value(s) */
                value = rpm_property->value;
                while (value) {
                    old_value = value;
                    value = value->next;
                    free(old_value);
                }
            }
            else
            {
                printf("    %s: ", bactext_property_name(
										rpm_property->propertyIdentifier) );
            	PrintReadPropertyData( rpm_property );
            }
            old_rpm_property = rpm_property;
            rpm_property = rpm_property->next;
            free(old_rpm_property);
        }
        old_rpm_data = rpm_data;
        rpm_data = rpm_data->next;
        free(old_rpm_data);
    }

    /* Now determine the next state */
    if ( bSuccess && ( myState == GET_ALL_RESPONSE) )
    	nextState = NEXT_OBJECT;
    else if ( bSuccess )	/* and GET_LIST_OF_ALL_RESPONSE */
    {
    	/* Now append the properties we waited on. */
    	if ( bHasStructuredViewList ) {
			Property_List[ Property_List_Index++ ] = PROP_STRUCTURED_OBJECT_LIST;
			Property_List_Length++;
    	}
    	if ( bHasObjectList ) {
			Property_List[ Property_List_Index++ ] = PROP_OBJECT_LIST;
			Property_List_Length++;
    	}
    	/* Now insert the -1 list terminator, but don't count it. */
		Property_List[ Property_List_Index ] = -1;
		assert ( Property_List_Length < MAX_PROPS );
    	Property_List_Index = 0;		/* Will start at top of the list */
    	nextState = GET_PROPERTY_REQUEST;
    }
    return nextState;
}

void PrintUsage()
{
	printf("bacepics -- Generates Object and Property List for EPICS \r\n" );
	printf("Usage: \r\n" );
    printf("  bacepics [-v] device-instance \r\n" );
    printf("    Use the -v option to show values instead of '?' \r\n\r\n" );
    printf("Insert the output in your EPICS file as the last section: \r\n");
    printf("\"List of Objects in test device:\"  \r\n");
    printf("before the final statement: \r\n");
    printf("\"End of BACnet Protocol Implementation Conformance Statement\" \r\n");
    exit(0);
}

int CheckCommandLineArgs(
	int argc,
    char *argv[] )
{
	int i;
	bool bFoundTarget = false;
    /* FIXME: handle multi homed systems - use an argument passed to the datalink_init() */

    /* print help if not enough arguments */
    if (argc < 2) {
    	fprintf(stderr, "Must provide a device-instance \r\n\r\n" );
    	PrintUsage();		/* Will exit */
    }
    for ( i = 1; i < argc; i++ )
    {
    	char *anArg = argv[i];
    	if ( anArg[0] == '-' )
    	{
    		if ( anArg[1] == 'v' )
    			ShowValues = true;
    		else
    			PrintUsage();
    	}
    	else
    	{
    	    /* decode the Target Device Instance parameter */
    	    Target_Device_Object_Instance = strtol( anArg, NULL, 0);
    	    if (Target_Device_Object_Instance > BACNET_MAX_INSTANCE) {
    	        fprintf(stderr, "device-instance=%u - it must be less than %u\r\n",
    	            Target_Device_Object_Instance, BACNET_MAX_INSTANCE + 1);
    			PrintUsage();
    	    }
    	    bFoundTarget = true;
    	}
    }
    if ( !bFoundTarget ) {
    	fprintf(stderr, "Must provide a device-instance \r\n\r\n" );
    	PrintUsage();		/* Will exit */
    }

    return 0;		/* All OK if we reach here */
}

/** Main function of the bacepics program.
 *
 * @see Device_Set_Object_Instance_Number, Keylist_Create, address_init, 
 *      dlenv_init, address_bind_request, Send_WhoIs, 
 *      tsm_timer_milliseconds, datalink_receive, npdu_handler,
 *      Send_Read_Property_Multiple_Request,  
 *      
 *  
 * @param argc [in] Arg count.
 * @param argv [in] Takes one or two arguments: an optional -v "Show Values"
 *                  switch, and the Device Instance #.
 * @return 0 on success.
 */
int main(
    int argc,
    char *argv[])
{
    BACNET_ADDRESS src = {
        0
    };  /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 100;     /* milliseconds */
    unsigned max_apdu = 0;
    time_t elapsed_seconds = 0;
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    time_t timeout_seconds = 0;
    uint8_t invoke_id = 0;
    bool found = false;
    BACNET_OBJECT_ID myObject;
    uint8_t buffer[MAX_PDU] = { 0 };
    BACNET_READ_ACCESS_DATA *rpm_object;
    BACNET_PROPERTY_REFERENCE *rpm_property;
    KEY nextKey;

    CheckCommandLineArgs( argc, argv );		/* Won't return if there is an issue. */

    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    Object_List = Keylist_Create();
    address_init();
    Init_Service_Handlers();
    dlenv_init();
    /* configure the timeout values */
    current_seconds = time(NULL);
    timeout_seconds = (apdu_timeout() / 1000) * apdu_retries();
    /* try to bind with the device */
    found =
        address_bind_request(Target_Device_Object_Instance, &max_apdu,
        &Target_Address);
    if (!found) {
        Send_WhoIs(Target_Device_Object_Instance,
            Target_Device_Object_Instance);
    }
    printf("List of Objects in test device:\r\n");
    /* Print Opening brace, then kick off the Device Object */
    printf("{ \r\n");
    printf("  { \r\n");		/* And opening brace for the first object */
    myObject.type = OBJECT_DEVICE;
    myObject.instance = Target_Device_Object_Instance;
    myState = INITIAL_BINDING;
    do {
        /* increment timer - will exit if timed out */
        last_seconds = current_seconds;
        current_seconds = time(NULL);
        /* Has at least one second passed ? */
        if (current_seconds != last_seconds) {
            tsm_timer_milliseconds(((current_seconds - last_seconds) * 1000));
        }

        /* OK to proceed; see what we are up to now */
    	switch ( myState )
    	{
    	case INITIAL_BINDING:
            /* returns 0 bytes on timeout */
            pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);

            /* process; normally is some initial error */
            if (pdu_len) {
                npdu_handler(&src, &Rx_Buf[0], pdu_len);
            }
            /* will wait until the device is bound, or timeout and quit */
            found = address_bind_request(Target_Device_Object_Instance,
											&max_apdu, &Target_Address);
            if ( !found )
            {
                /* increment timer - exit if timed out */
                elapsed_seconds += (current_seconds - last_seconds);
                if (elapsed_seconds > timeout_seconds) {
                    printf("\rError: APDU Timeout!\r\n");
                    break;
                }
                /* else, loop back and try again */
                continue;
            }
            else
                myState = GET_ALL_REQUEST;
            break;

    	case GET_ALL_REQUEST:
    	case GET_LIST_OF_ALL_REQUEST:		/* differs in ArrayIndex only */
    		Error_Detected = false;
    		Property_List_Index = Property_List_Length = 0;
            /* Update times; aids single-step debugging */
            last_seconds = current_seconds;
     		rpm_object = calloc(1, sizeof(BACNET_READ_ACCESS_DATA));
    		assert( rpm_object );
    		rpm_object->object_type = myObject.type;
       		rpm_object->object_instance = myObject.instance;
            rpm_property = calloc(1, sizeof(BACNET_PROPERTY_REFERENCE));
            rpm_object->listOfProperties = rpm_property;
            assert( rpm_property );
            rpm_property->propertyIdentifier = PROP_ALL;
            if ( myState == GET_LIST_OF_ALL_REQUEST )
            	rpm_property->propertyArrayIndex = 0; /* Get count of arrays */
            else
            	rpm_property->propertyArrayIndex = -1; /* optional: None */
            invoke_id =
                Send_Read_Property_Multiple_Request( buffer, MAX_PDU,
                		Target_Device_Object_Instance, rpm_object );
            if ( invoke_id > 0 )
            {
                if ( myState == GET_LIST_OF_ALL_REQUEST )
                	myState = GET_LIST_OF_ALL_RESPONSE;
                else
                	myState = GET_ALL_RESPONSE;
            }
            break;

    	case GET_ALL_RESPONSE:
    	case GET_LIST_OF_ALL_RESPONSE:
            /* returns 0 bytes on timeout */
            pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);

            /* process */
            if (pdu_len) {
                npdu_handler(&src, &Rx_Buf[0], pdu_len);
            }

			if ((Read_Property_Multiple_Data.new_data) &&
				(invoke_id == Read_Property_Multiple_Data.service_data.invoke_id)) {
				Read_Property_Multiple_Data.new_data = false;
				myState = ProcessRPMData( Read_Property_Multiple_Data.rpm_data,
											myState );
				if (tsm_invoke_id_free(invoke_id)) {
					invoke_id = 0;
				}
				else {
					assert( false );		/* How can this be? */
					invoke_id = 0;
				}
			} else if (tsm_invoke_id_free(invoke_id)) {
				invoke_id = 0;
				if (Error_Detected)		/* The normal case for Device Object */
				{
					/* Try again, just to get a list of properties. */
					if ( myState == GET_ALL_RESPONSE )
						myState = GET_LIST_OF_ALL_REQUEST;
					/* Else it may be that RPM is not implemented. */
					else
						myState = GET_PROPERTY_REQUEST;
				}
				else
					myState = GET_ALL_REQUEST;		/* Let's try again */
			} else if (tsm_invoke_id_failed(invoke_id)) {
				fprintf(stderr, "\rError: TSM Timeout!\r\n");
				tsm_free_invoke_id(invoke_id);
				invoke_id = 0;
				myState = GET_ALL_REQUEST;		/* Let's try again */
			} else if (Error_Detected) {
				/* Don't think we'll ever actually reach this point. */
				invoke_id = 0;
				myState = NEXT_OBJECT;		/* Give up and move on to the next.*/
			}
    		break;

    	/* Process the next single property in our list,
    	 * if we couldn't GET_ALL at once above. */
    	case GET_PROPERTY_REQUEST:
    		Error_Detected = false;
            /* Update times; aids single-step debugging */
            last_seconds = current_seconds;
            invoke_id = Read_Properties(Target_Device_Object_Instance, &myObject );
            if (invoke_id == 0) {
                /* Reached the end of the list. */
				myState = NEXT_OBJECT;		/* Move on to the next.*/
            }
            else
				myState = GET_PROPERTY_RESPONSE;
            break;

    	case GET_PROPERTY_RESPONSE:
            /* returns 0 bytes on timeout */
            pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);

            /* process */
            if (pdu_len) {
                npdu_handler(&src, &Rx_Buf[0], pdu_len);
            }

            if ( (Read_Property_Multiple_Data.new_data) &&
                 (invoke_id == Read_Property_Multiple_Data.service_data.invoke_id))
			{
            	Read_Property_Multiple_Data.new_data = false;
				PrintReadPropertyData( Read_Property_Multiple_Data.rpm_data->listOfProperties );
				if (tsm_invoke_id_free(invoke_id)) {
					invoke_id = 0;
				}
				else {
					assert( false );		/* How can this be? */
					invoke_id = 0;
				}
				/* Advance the property (or Array List) index */
				if ( Using_Walked_List )
				{
                    Walked_List_Index++;
                    if (Walked_List_Index > Walked_List_Length) {
                        /* go on to next property */
                        Property_List_Index++;
                        Using_Walked_List = false;
                    }
				}
				else
                    Property_List_Index++;
				if ( pPropList[Property_List_Index] == PROP_OBJECT_LIST )
				{
					if ( !Using_Walked_List )		/* Just switched */
					{
                        Using_Walked_List = true;
                        Walked_List_Index = Walked_List_Length = 0;
					}
				}
				myState = GET_PROPERTY_REQUEST;		/* Go fetch next Property */
			}
            else if (tsm_invoke_id_free(invoke_id)) {
				invoke_id = 0;
				if (Error_Detected)
				{
					/* OK, skip this one and try the next property. */
					fprintf( stdout, "    -- Failed to get %s \r\n",
							 bactext_property_name(pPropList[Property_List_Index]) );
					Property_List_Index++;
				}
				myState = GET_PROPERTY_REQUEST;
			} else if (tsm_invoke_id_failed(invoke_id)) {
				fprintf(stderr, "\rError: TSM Timeout!\r\n");
				tsm_free_invoke_id(invoke_id);
				invoke_id = 0;
				myState = GET_PROPERTY_REQUEST;		/* Let's try again, same Property */
			} else if (Error_Detected) {
				/* Don't think we'll ever actually reach this point. */
				invoke_id = 0;
				myState = NEXT_OBJECT;		/* Give up and move on to the next.*/
			}
    		break;

    	case NEXT_OBJECT:
    	    if ( myObject.type == OBJECT_DEVICE )
    	    {
    	        printf("  -- Found %d Objects \r\n", Keylist_Count( Object_List ) );
    	    	Object_List_Index = -1;		/* start over (will be incr to 0) */
    	    }
    	    /* Advance to the next object, as long as it's not the Device object */
			do
			{
				Object_List_Index++;
				nextKey = Keylist_Key( Object_List, Object_List_Index );
				/* If done with all Objects, signal end of this while loop */
				if ( ( nextKey == 0 ) || ( Object_List_Index >= Object_List_Length ) )
				{
		    		/* Closing brace for the last Object */
		    	    printf("  } \r\n");
					myObject.type = MAX_BACNET_OBJECT_TYPE;
				}
				else
				{
		    		/* Closing brace for the previous Object */
		    	    printf("  }, \r\n");
					myObject.type = KEY_DECODE_TYPE( nextKey );
					myObject.instance = KEY_DECODE_ID( nextKey );
		    		/* Opening brace for the new Object */
		    	    printf("  { \r\n");
		    	    /* Test code:
		    	    if ( myObject.type == OBJECT_STRUCTURED_VIEW )
		    	    	printf( "    -- Structured View %d \n", myObject.instance );
		    	    */
				}
				myState = GET_ALL_REQUEST;

			} while ( myObject.type == OBJECT_DEVICE );
    	    /* Else, don't re-do the Device Object; move to the next object. */
    		break;

    	default:
    		assert( false );	/* program error; fix this */
    		break;
    	}

    	/* Check for timeouts */
        if ( !found || ( invoke_id > 0 ) )
        {
            /* increment timer - exit if timed out */
            elapsed_seconds += (current_seconds - last_seconds);
            if (elapsed_seconds > timeout_seconds) {
                printf("\rError: APDU Timeout!\r\n");
                break;
            }
        }

    } while ( myObject.type < MAX_BACNET_OBJECT_TYPE );

    /* Closing brace for all Objects */
    printf("} \r\n");

    return 0;
}

/*@}*/      /* End group BACEPICS */

