/**************************************************************************
*
* Copyright (C) Peter Mc Shane
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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>     /* for memmove */
#include "bacdef.h"
#include "bacdcode.h"
#include "bacenum.h"
#include "bacapp.h"
#include "config.h"     /* the custom stuff */
#include "apdu.h"
#include "wp.h" /* write property handling */
#include "version.h"
#include "device.h"     /* me */
#include "handlers.h"
#include "datalink.h"
#include "address.h"
#include "bacdevobjpropref.h"
#if defined(BACFILE)
#include "bacfile.h"    /* object list dependency */
#endif

#define MAX_TREND_LOGS 8

/* Error code for Trend Log storage */

typedef struct tl_error {
    uint16_t usClass;
    uint16_t usCode;
} TL_ERROR;

/* Bit string of up to 32 bits for Trend Log storage */

typedef struct tl_bits {
    uint8_t  ucLen;
    uint8_t  ucStore[4];
} TL_BITS;

/* Storage structure for Trend Log data */

typedef struct tl_data_record {
    uint32_t ulTimeStamp;  /* When the event occurred */
    uint8_t ucRecType;     /* What type of Event */
    uint8_t ucStatus;      /* Optional Status for read value */
    union {
        uint8_t  ucLogStatus;   /* Change of log state flags */
        uint8_t  ucBoolean;     /* Stored boolean value */
        float    fReal;         /* Stored floating point value */
        uint32_t ulEnum;        /* Stored enumerated value - max 32 bits */
        uint32_t ulUValue;      /* Stored unsigned value - max 32 bits */
        int32_t  lSValue;       /* Stored signed value - max 32 bits */
        TL_BITS  Bits;          /* Stored bitstring - max 32 bits */
        TL_ERROR Error;         /* Two part error class/code combo */
        float    fTime;         /* Interval value for change of time - seconds */
    } Datum;
} TL_DATA_REC;

#define TL_T_START_WILD 1 /* Start time is wild carded */
#define TL_T_STOP_WILD  2 /* Stop Time is wild carded */

#define TL_MAX_ENTRIES 1000 /* Entries per datalog */

TL_DATA_REC Logs[MAX_TREND_LOGS][TL_MAX_ENTRIES];

/* Structure containing config and status info for a Trend Log */

typedef struct tl_log_info {
    bool bEnable;               /* Trend log is active when this is true */
    BACNET_DATE_TIME StartTime; /* BACnet format start time */
    uint32_t ulStartTime;       /* Local time working copy of start time */
    BACNET_DATE_TIME StopTime;  /* BACnet format stop time */
    uint32_t ulStopTime;        /* Local time working copy of stop time */
    uint8_t  ucTimeFlags;       /* Shorthand info on times */
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE Source; /* Where the data comes from */
    uint32_t ulLogInterval;     /* Time between entries in 1/100s */
    bool bStopWhenFull;         /* Log halts when full if true */
    uint32_t ulRecordCount;     /* Count of items currently in the buffer */
    uint32_t ulTotalRecordCount;     /* Count of all items that have ever been inserted into the buffer */
    BACNET_LOGGING_TYPE LoggingType; /* Polled/cov/triggered */
    bool bAlignIntervals;            /* If true align to the clock */
    uint32_t ulIntervalOffset;       /* Offset from start of period for taking reading */
    bool bTrigger;                   /* Set to 1 to cause a reading to be taken */
} TL_LOG_INFO;


static TL_LOG_INFO LogInfo[MAX_TREND_LOGS];

/* 
 * Data types associated with a BACnet Log Record. We use these for managing the
 * log buffer but they are also the tag numbers to use when encoding/decoding
 * the log datum field.
 */

#define TL_TYPE_STATUS  0
#define TL_TYPE_BOOL    1
#define TL_TYPE_REAL    2
#define TL_TYPE_ENUM    3
#define TL_TYPE_UNSIGN  4
#define TL_TYPE_SIGN    5
#define TL_TYPE_BITS    6
#define TL_TYPE_NULL    7
#define TL_TYPE_ERROR   8
#define TL_TYPE_DELTA   9
#define TL_TYPE_ANY     10 /* We don't support this particular can of worms! */

/* These three arrays are used by the ReadPropertyMultiple handler */
static const int Trend_Log_Properties_Required[] = {
    PROP_OBJECT_IDENTIFIER,
    PROP_OBJECT_NAME,
    PROP_OBJECT_TYPE,
    PROP_ENABLE,
    PROP_STOP_WHEN_FULL,
    PROP_BUFFER_SIZE,
    PROP_LOG_BUFFER,
    PROP_RECORD_COUNT,
    PROP_TOTAL_RECORD_COUNT,
    PROP_EVENT_STATE,
    PROP_LOGGING_TYPE,
    PROP_STATUS_FLAGS,
    -1
};

static const int Trend_Log_Properties_Optional[] = {
    PROP_DESCRIPTION,
    PROP_START_TIME,
    PROP_STOP_TIME,
    PROP_LOG_DEVICE_OBJECT_PROPERTY,
    PROP_LOG_INTERVAL,

/* Required if COV logging supported
    PROP_COV_RESUBSCRIPTION_INTERVAL,
    PROP_CLIENT_COV_INCREMENT, */
    
/* Required if intrinsic reporting supported    
    PROP_NOTIFICATION_THRESHOLD,
    PROP_RECORDS_SINCE_NOTIFICATION,
    PROP_LAST_NOTIFY_RECORD,
    PROP_NOTIFICATION_CLASS,
    PROP_EVENT_ENABLE,
    PROP_ACKED_TRANSITIONS,
    PROP_NOTIFY_TYPE,
    PROP_EVENT_TIME_STAMPS, */
    
    PROP_ALIGN_INTERVALS,
    PROP_INTERVAL_OFFSET,
    PROP_TRIGGER,
    -1
};

static const int Trend_Log_Properties_Proprietary[] = {
    -1
};

void Trend_Log_Property_Lists(
    const int **pRequired,
    const int **pOptional,
    const int **pProprietary)
{
    if (pRequired)
        *pRequired = Trend_Log_Properties_Required;
    if (pOptional)
        *pOptional = Trend_Log_Properties_Optional;
    if (pProprietary)
        *pProprietary = Trend_Log_Properties_Proprietary;

    return;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need validate that the */
/* given instance exists */
bool Trend_Log_Valid_Instance(
    uint32_t object_instance)
{
    if (object_instance < MAX_TREND_LOGS) {
        return true;
    }

    return false;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then count how many you have */
unsigned Trend_Log_Count(
    void)
{
    return MAX_TREND_LOGS;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the instance */
/* that correlates to the correct index */
uint32_t Trend_Log_Index_To_Instance(
    unsigned index)
{
    return index;
}

/* we simply have 0-n object instances.  Yours might be */
/* more complex, and then you need to return the index */
/* that correlates to the correct instance number */
unsigned Trend_Log_Instance_To_Index(
    uint32_t object_instance)
{
    unsigned index = MAX_TREND_LOGS;

    if (object_instance < MAX_TREND_LOGS) {
        index = object_instance;
    }

    return index;
}

/*
 * Things to do when starting up the stack for Trend Logs.
 * Should be called whenever we reset the device or power it up
 */
void Trend_Log_Init(
    void)
{
    static bool initialized = false;
    int iLog;
    int iEntry;
    struct tm TempTime;
    time_t Clock;
    
    if (!initialized) {
        initialized = true;

        /* initialize all the values */
        
        for (iLog = 0; iLog < MAX_TREND_LOGS; iLog++) {
            /* 
             * Do we need to do anything here?
             * Trend logs are usually assumed to survive over resets
             * and are frequently implemented using Battery Backed RAM
             * If they are implemented using Flash or SD cards or some
             * such mechanism there may be some RAM based setup needed
             * for log management purposes.
             * We probably need to look at inserting LOG_INTERRUPTED
             * entries into any active logs if the power down or reset
             * may have caused us to miss readings.
             */
             
            /* We will just fill the logs with some entries for testing
             * purposes.
             */
            TempTime.tm_year = 109;
            TempTime.tm_mon  = iLog + 1; /* Different month for each log */
            TempTime.tm_mday = 1;
            TempTime.tm_hour = 0;
            TempTime.tm_min  = 0;
            TempTime.tm_sec  = 0;
            Clock = mktime(&TempTime);
            
            for(iEntry = 0; iEntry < TL_MAX_ENTRIES; iEntry++) {
                Logs[iLog][iEntry].ulTimeStamp = Clock;
                Logs[iLog][iEntry].ucRecType   = TL_TYPE_REAL;
                Logs[iLog][iEntry].Datum.fReal = (float)(iEntry + (iLog * TL_MAX_ENTRIES));
                Logs[iLog][iEntry].ucStatus    = 0;
                Clock += 900; /* advance 15 minutes */
            }
            
           LogInfo[iLog].bAlignIntervals    = true;
           LogInfo[iLog].bEnable            = true;
           LogInfo[iLog].bStopWhenFull      = false;
           LogInfo[iLog].bTrigger           = false;
           LogInfo[iLog].LoggingType        = LOGGING_TYPE_POLLED;
           LogInfo[iLog].Source.arrayIndex  = 0;
           LogInfo[iLog].ucTimeFlags        = 0;
           LogInfo[iLog].ulIntervalOffset   = 0;
           LogInfo[iLog].ulLogInterval      = 900;
           LogInfo[iLog].ulRecordCount      = 1000;
           LogInfo[iLog].ulTotalRecordCount = 10000;
           
           LogInfo[iLog].Source.deviceIndentifier.instance = Device_Object_Instance_Number();
           LogInfo[iLog].Source.deviceIndentifier.type     = OBJECT_DEVICE;
           LogInfo[iLog].Source.objectIdentifier.instance  = iLog;
           LogInfo[iLog].Source.objectIdentifier.type      = OBJECT_ANALOG_INPUT;
           LogInfo[iLog].Source.propertyIdentifier         = PROP_PRESENT_VALUE;
           
           datetime_set_values(&LogInfo[iLog].StartTime, 109, 1,   1,  0,  0,  0,  0);
           datetime_set_values(&LogInfo[iLog].StopTime,  109, 11, 22, 23, 59, 59, 99);
        }
    }

    return;
}


char *Trend_Log_Name(
    uint32_t object_instance)
{
    static char text_string[32] = "";   /* okay for single thread */

    if (object_instance < MAX_TREND_LOGS) {
        sprintf(text_string, "Trend Log %u", object_instance);
        return text_string;
    }

    return NULL;
}


/* return the length of the apdu encoded or -1 for error or
   -2 for abort message */
int Trend_Log_Encode_Property_APDU(
    uint8_t * apdu,
    uint32_t object_instance,
    BACNET_PROPERTY_ID property,
    int32_t array_index,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    int apdu_len = 0;   /* return value */
    int len = 0;        /* apdu len intermediate value */
    BACNET_BIT_STRING bit_string;
    BACNET_CHARACTER_STRING char_string;
    unsigned i = 0;
    int object_type = 0;
    uint32_t instance = 0;
    unsigned count = 0;
    TL_LOG_INFO *CurrentLog;

    CurrentLog = &LogInfo[Trend_Log_Instance_To_Index(object_instance)]; /* Pin down which log to look at */

    switch (property) {
        case PROP_OBJECT_IDENTIFIER:
            apdu_len = 
                encode_application_object_id(&apdu[0], OBJECT_TRENDLOG,
                object_instance);
            break;
            
        case PROP_DESCRIPTION:
        case PROP_OBJECT_NAME:
            characterstring_init_ansi(&char_string, Trend_Log_Name(object_instance));
            apdu_len =
                encode_application_character_string(&apdu[0], &char_string);
            break;
            
        case PROP_OBJECT_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], OBJECT_TRENDLOG);
            break;

        case PROP_ENABLE:
            apdu_len = encode_application_boolean(&apdu[0], CurrentLog->bEnable);
            break;
            
        case PROP_STOP_WHEN_FULL:
            apdu_len = encode_application_boolean(&apdu[0], CurrentLog->bEnable);
            break;

        case PROP_BUFFER_SIZE:
            apdu_len = encode_application_unsigned(&apdu[0], TL_MAX_ENTRIES);
            break;

        case PROP_LOG_BUFFER:
            /* You can only read the buffer via the ReadRange service */
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_READ_ACCESS_DENIED;
            apdu_len = -1;
            break;

        case PROP_RECORD_COUNT:
            apdu_len += encode_application_unsigned(&apdu[apdu_len], CurrentLog->ulRecordCount);
            break;

        case PROP_TOTAL_RECORD_COUNT:
            apdu_len += encode_application_unsigned(&apdu[apdu_len], CurrentLog->ulTotalRecordCount);
            break;

        case PROP_EVENT_STATE:
            /* note: see the details in the standard on how to use this */
            apdu_len = encode_application_enumerated(&apdu[0], EVENT_STATE_NORMAL);
            break;

        case PROP_LOGGING_TYPE:
            apdu_len = encode_application_enumerated(&apdu[0], CurrentLog->LoggingType);
            break;

        case PROP_STATUS_FLAGS:
            /* note: see the details in the standard on how to use these */
            bitstring_init(&bit_string);
            bitstring_set_bit(&bit_string, STATUS_FLAG_IN_ALARM, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_FAULT, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OVERRIDDEN, false);
            bitstring_set_bit(&bit_string, STATUS_FLAG_OUT_OF_SERVICE,false);
            apdu_len = encode_application_bitstring(&apdu[0], &bit_string);
            break;

        case PROP_START_TIME:
            len = encode_application_date(&apdu[0],
                &CurrentLog->StartTime.date);
            apdu_len = len;
            len = encode_application_time(&apdu[apdu_len],
                &CurrentLog->StartTime.time);
            apdu_len += len;
            break;

        case PROP_STOP_TIME:
            len = encode_application_date(&apdu[0],
                &CurrentLog->StopTime.date);
            apdu_len = len;
            len = encode_application_time(&apdu[apdu_len],
                &CurrentLog->StopTime.time);
            apdu_len += len;
            break;

        case PROP_LOG_DEVICE_OBJECT_PROPERTY:
            /*
             * BACnetDeviceObjectPropertyReference ::= SEQUENCE {
	         *     objectIdentifier   [0] BACnetObjectIdentifier,
             *     propertyIdentifier [1] BACnetPropertyIdentifier,
             *     propertyArrayIndex [2] Unsigned OPTIONAL, -- used only with array datatype
             *                                               -- if omitted with an array then
             *                                               -- the entire array is referenced
             *     deviceIdentifier   [3] BACnetObjectIdentifier OPTIONAL
	         * }
	         */
            apdu_len += bacapp_encode_device_obj_property_ref(&apdu[apdu_len], &CurrentLog->Source);
            break;

        case PROP_LOG_INTERVAL:
            /* We only log to 1 sec accuracy so must multiply by 100 before passing it on */
            apdu_len += encode_application_unsigned(&apdu[apdu_len], CurrentLog->ulLogInterval * 100);
            break;

        case PROP_ALIGN_INTERVALS:
            apdu_len = encode_application_boolean(&apdu[0], CurrentLog->bAlignIntervals);
            break;
            
        case PROP_INTERVAL_OFFSET:
            /* We only log to 1 sec accuracy so must multiply by 100 before passing it on */
            apdu_len += encode_application_unsigned(&apdu[apdu_len], CurrentLog->ulIntervalOffset * 100);
            break;
            
        case PROP_TRIGGER:
            break;
            
        default:
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_UNKNOWN_PROPERTY;
            apdu_len = -1;
            break;
    }
    /*  only array properties can have array options */
    if ((apdu_len >= 0) &&
        (property != PROP_EVENT_TIME_STAMPS) &&
        (array_index != BACNET_ARRAY_ALL)) {
        *error_class = ERROR_CLASS_PROPERTY;
        *error_code = ERROR_CODE_PROPERTY_IS_NOT_AN_ARRAY;
        apdu_len = -1;
    }

    return apdu_len;
}

/* returns true if successful */
bool Trend_Log_Write_Property(
    BACNET_WRITE_PROPERTY_DATA * wp_data,
    BACNET_ERROR_CLASS * error_class,
    BACNET_ERROR_CODE * error_code)
{
    bool status = false;        /* return value */
    int len = 0;
    int iOffset = 0;
    BACNET_APPLICATION_DATA_VALUE value;
    TL_LOG_INFO *CurrentLog;
    BACNET_DATE TempDate; /* build here in case of error in time half of datetime */
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE TempSource;
    
    if (!Trend_Log_Valid_Instance(wp_data->object_instance)) {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code = ERROR_CODE_UNKNOWN_OBJECT;
        return false;
    }

    CurrentLog = &LogInfo[Trend_Log_Instance_To_Index(wp_data->object_instance)]; /* Pin down which log to look at */

    /* decode the some of the request */
    len =
        bacapp_decode_application_data(wp_data->application_data,
        wp_data->application_data_len, &value);
    /* FIXME: len < application_data_len: more data? */
    /* FIXME: len == 0: unable to decode? */

    switch (wp_data->object_property) {
        case PROP_ENABLE:
            if (value.tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                CurrentLog->bEnable = value.type.Boolean;
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;

        case PROP_STOP_WHEN_FULL:
            if (value.tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                CurrentLog->bStopWhenFull = value.type.Boolean;
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            /* To do: implement logic for when full log is switched from
             * normal to stop when full see 12.25.12
             */
            break;

        case PROP_BUFFER_SIZE:
            /* Fixed size buffer so deny write */
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;

        case PROP_RECORD_COUNT:
            /* To Do - Reset log if count of zero is written.
             * what do we do if any other value is written?
             */ 
        
            if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                CurrentLog->ulRecordCount = value.type.Unsigned_Int;
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;

        case PROP_LOGGING_TYPE:
            /* To Do - implement logic as per 12.25.27 for
             * triggered and polled options.
             */ 
        
            if (value.tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                if(value.type.Enumerated != LOGGING_TYPE_COV) {
                    CurrentLog->LoggingType = value.type.Enumerated;
                    status = true;
                } else {
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED;
                }                
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;

        case PROP_START_TIME:
            /* Copy the date part to safe place */
            if (value.tag == BACNET_APPLICATION_TAG_DATE) {
                TempDate = value.type.Date;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
                break;
            }
            /* Then decode the time part */
            len = bacapp_decode_application_data(wp_data->application_data + len,
                wp_data->application_data_len - len, &value);
                
            if (len && value.tag == BACNET_APPLICATION_TAG_TIME) {
                CurrentLog->StartTime.date = TempDate; /* Safe to copy the date now */
                CurrentLog->StartTime.time = value.type.Time;
                                    
                if (datetime_wildcard(&CurrentLog->StartTime)) {
                    CurrentLog->ucTimeFlags |= TL_T_START_WILD;
                    CurrentLog->ulStartTime = 0; 
                } else {
                    CurrentLog->ucTimeFlags &= ~TL_T_START_WILD;
                    /* To do - convert bacnet date time to local 32 bit value */
                }
                    
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;

        case PROP_STOP_TIME:
            /* Copy the date part to safe place */
            if (value.tag == BACNET_APPLICATION_TAG_DATE) {
                TempDate = value.type.Date;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
                break;
            }
            /* Then decode the time part */
            len = bacapp_decode_application_data(wp_data->application_data + len,
                wp_data->application_data_len - len, &value);
                
            if (len && value.tag == BACNET_APPLICATION_TAG_TIME) {
                CurrentLog->StopTime.date = TempDate; /* Safe to copy the date now */
                CurrentLog->StopTime.time = value.type.Time;
                                    
                if (datetime_wildcard(&CurrentLog->StopTime)) {
                    CurrentLog->ucTimeFlags |= TL_T_STOP_WILD;
                    CurrentLog->ulStopTime = 0xFFFFFFFF; 
                } else {
                    CurrentLog->ucTimeFlags &= ~TL_T_STOP_WILD;
                    /* To do - convert bacnet date time to local 32 bit value */
                }
                    
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;

        case PROP_LOG_DEVICE_OBJECT_PROPERTY:
            memset(&TempSource, 0, sizeof(TempSource)); /* Start with clean sheet */
            
            /* First up is the object ID */
            len = bacapp_decode_context_data(wp_data->application_data, wp_data->application_data_len, &value, PROP_LOG_DEVICE_OBJECT_PROPERTY);
            if((len == 0) || (value.context_tag != 0) || ((wp_data->application_data_len - len) == 0)) {
                /* Bad decode, wrong tag or following required parameter missing */                
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_OTHER;
                break;
            }
            
            TempSource.objectIdentifier = value.type.Object_Id;
            wp_data->application_data_len -= len;
            iOffset = len;
            /* Second up is the property id */
            len = bacapp_decode_context_data(&wp_data->application_data[iOffset], wp_data->application_data_len, &value, PROP_LOG_DEVICE_OBJECT_PROPERTY);
            if((len == 0) || (value.context_tag != 1)) {
                /* Bad decode or wrong tag */                
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_OTHER;
                break;
            }
            
            TempSource.propertyIdentifier  = value.type.Enumerated;
            wp_data->application_data_len -= len;
            
            /* If there is still more to come */
            if(wp_data->application_data_len != 0) {
                iOffset += len;
                len = bacapp_decode_context_data(&wp_data->application_data[iOffset], wp_data->application_data_len, &value, PROP_LOG_DEVICE_OBJECT_PROPERTY);
                if((len == 0) || ((value.context_tag != 2) && (value.context_tag != 3))) {
                    /* Bad decode or wrong tag */                
                    *error_class = ERROR_CLASS_PROPERTY;
                    *error_code = ERROR_CODE_OTHER;
                    break;
                }

                if(value.context_tag == 2) {
                    /* Got an index so deal with it */
                    TempSource.arrayIndex = value.type.Unsigned_Int;
                    wp_data->application_data_len -= len;
                    /* Still some remaining */
                    if(wp_data->application_data_len != 0) {
                        iOffset += len;
                        len = bacapp_decode_context_data(&wp_data->application_data[iOffset], wp_data->application_data_len, &value, PROP_LOG_DEVICE_OBJECT_PROPERTY);
                        if((len == 0) || (value.context_tag != 3)) {
                            /* Bad decode or wrong tag */                
                            *error_class = ERROR_CLASS_PROPERTY;
                            *error_code = ERROR_CODE_OTHER;
                            break;
                        }
                    }
                }
                
                if(value.context_tag == 2) {
                    /* Got a device ID so deal with it */
                    TempSource.deviceIndentifier = value.type.Object_Id;
                    if(TempSource.deviceIndentifier.instance != Device_Object_Instance_Number()) {
                         /* Not our ID so can't handle it at the moment */                
                         *error_class = ERROR_CLASS_PROPERTY;
                         *error_code = ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED;
                        break;
                    }
                }    
            }
            
            CurrentLog->Source = TempSource;    
            status = true;
            break;

        case PROP_LOG_INTERVAL:
            /* To Do - Make non writable when Logging set to triggered 
             * also implement rules for switching between cov and polled by
             * writing to this propert see section 12.25.9
             */ 
        
            /* We only log to 1 sec accuracy so must divide by 100 before passing it on */
            if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                CurrentLog->ulLogInterval = value.type.Unsigned_Int / 100;
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;

        case PROP_ALIGN_INTERVALS:
            if (value.tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                CurrentLog->bAlignIntervals = value.type.Boolean;
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
            
        case PROP_INTERVAL_OFFSET:
            /* We only log to 1 sec accuracy so must divide by 100 before passing it on */
            if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                CurrentLog->ulIntervalOffset = value.type.Unsigned_Int / 100;
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;
            
        case PROP_TRIGGER:
            /* To Do - implement trigger logic as per
             * 12.25.30
             */ 
            if (value.tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                CurrentLog->bAlignIntervals = value.type.Boolean;
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;

        default:
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;
    }

    return status;
}

void TrendLog_Init(
    void)
{
}

bool Trend_Log_GetRRInfo(
    uint32_t           Object,   /* Which particular object - obviously not important for device object */
    BACNET_PROPERTY_ID Property, /* Which property */
    RR_PROP_INFO      *pInfo,    /* Where to put the information */
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE  *error_code)
{
    if(Property == PROP_LOG_BUFFER) {
        pInfo->RequestTypes = RR_BY_POSITION | RR_BY_TIME | RR_BY_SEQUENCE;
        pInfo->Handler = NULL;
        return(true);
    } else {
        *error_class = ERROR_CLASS_SERVICES;
        *error_code  = ERROR_CODE_PROPERTY_IS_NOT_A_LIST;
    }
    
    return(false);
}

