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
#include "trendlog.h"
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
    uint8_t  ucLen;         /* bytes used in upper nibble/bits free in lower nibble */
    uint8_t  ucStore[4];
} TL_BITS;

/* Storage structure for Trend Log data
 *
 * Note. I've tried to minimise the storage requirements here
 * as the memory requirements for logging in embedded
 * implementations are frequently a big issue. For PC or
 * embedded Linux type setupz this may seem like overkill
 * but if you have limited memory and need to squeeze as much
 * logging capacity as possible every little byte counts!
 */

typedef struct tl_data_record {
    time_t tTimeStamp;     /* When the event occurred */
    uint8_t ucRecType;     /* What type of Event */
    uint8_t ucStatus;      /* Optional Status for read value in b0-b2, b7 = 1 if status is used */
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
    time_t tStartTime;          /* Local time working copy of start time */
    BACNET_DATE_TIME StopTime;  /* BACnet format stop time */
    time_t tStopTime;           /* Local time working copy of stop time */
    uint8_t  ucTimeFlags;       /* Shorthand info on times */
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE Source; /* Where the data comes from */
    uint32_t ulLogInterval;     /* Time between entries in seconds */
    bool bStopWhenFull;         /* Log halts when full if true */
    uint32_t ulRecordCount;     /* Count of items currently in the buffer */
    uint32_t ulTotalRecordCount;     /* Count of all items that have ever been inserted into the buffer */
    BACNET_LOGGING_TYPE LoggingType; /* Polled/cov/triggered */
    bool bAlignIntervals;            /* If true align to the clock */
    uint32_t ulIntervalOffset;       /* Offset from start of period for taking reading in seconds */
    bool bTrigger;                   /* Set to 1 to cause a reading to be taken */
    int iIndex;                      /* Current insertion point */
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
                Logs[iLog][iEntry].tTimeStamp = Clock;
                Logs[iLog][iEntry].ucRecType   = TL_TYPE_REAL;
                Logs[iLog][iEntry].Datum.fReal = (float)(iEntry + (iLog * TL_MAX_ENTRIES));
                /* Put status flags with every second log */
                if((iLog & 1) == 0)
                    Logs[iLog][iEntry].ucStatus    = 128;
                else
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
           LogInfo[iLog].iIndex             = 0;
           LogInfo[iLog].ulLogInterval      = 900;
           LogInfo[iLog].ulRecordCount      = 1000;
           LogInfo[iLog].ulTotalRecordCount = 10000;
           
           LogInfo[iLog].Source.deviceIndentifier.instance = Device_Object_Instance_Number();
           LogInfo[iLog].Source.deviceIndentifier.type     = OBJECT_DEVICE;
           LogInfo[iLog].Source.objectIdentifier.instance  = iLog;
           LogInfo[iLog].Source.objectIdentifier.type      = OBJECT_ANALOG_INPUT;
           LogInfo[iLog].Source.propertyIdentifier         = PROP_PRESENT_VALUE;
           
           datetime_set_values(&LogInfo[iLog].StartTime, 2009, 1,   1,  0,  0,  0,  0);
           LogInfo[iLog].tStartTime = TL_BAC_Time_To_Local(&LogInfo[iLog].StartTime);
           datetime_set_values(&LogInfo[iLog].StopTime,  2009, 12, 22, 23, 59, 59, 99);
           LogInfo[iLog].tStopTime = TL_BAC_Time_To_Local(&LogInfo[iLog].StopTime);
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
            apdu_len = encode_application_boolean(&apdu[0], CurrentLog->bStopWhenFull);
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
            apdu_len += encode_application_unsigned(&apdu[0], CurrentLog->ulRecordCount);
            break;

        case PROP_TOTAL_RECORD_COUNT:
            apdu_len += encode_application_unsigned(&apdu[0], CurrentLog->ulTotalRecordCount);
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
            apdu_len += bacapp_encode_device_obj_property_ref(&apdu[0], &CurrentLog->Source);
            break;

        case PROP_LOG_INTERVAL:
            /* We only log to 1 sec accuracy so must multiply by 100 before passing it on */
            apdu_len += encode_application_unsigned(&apdu[0], CurrentLog->ulLogInterval * 100);
            break;

        case PROP_ALIGN_INTERVALS:
            apdu_len = encode_application_boolean(&apdu[0], CurrentLog->bAlignIntervals);
            break;
            
        case PROP_INTERVAL_OFFSET:
            /* We only log to 1 sec accuracy so must multiply by 100 before passing it on */
            apdu_len += encode_application_unsigned(&apdu[0], CurrentLog->ulIntervalOffset * 100);
            break;
            
        case PROP_TRIGGER:
            apdu_len = encode_application_boolean(&apdu[0], CurrentLog->bTrigger);
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
    bool bEffectiveEnable;
    
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
                /* Section 12.25.5 can't enable a full log with stop when full set */
                if((CurrentLog->bEnable       == false) &&
                   (CurrentLog->bStopWhenFull == true) &&
                   (CurrentLog->ulRecordCount == TL_MAX_ENTRIES) &&
                   (value.type.Boolean        == true)) {
                    *error_class = ERROR_CLASS_OBJECT;
                    *error_code = ERROR_CODE_LOG_BUFFER_FULL;
                    break;
                }
                
                /* Only trigger this validation on a potential change of state */
                if(CurrentLog->bEnable != value.type.Boolean) {
                    bEffectiveEnable = TL_Is_Enabled(wp_data->object_instance);
                    CurrentLog->bEnable = value.type.Boolean;
                    /* To do: what actions do we need to take on writing ? */
                    if(value.type.Boolean == false) {
                        if(bEffectiveEnable == true) {
                            /* Only insert record if we really were enabled i.e. times and enable flags */
                            TL_Insert_Status_Rec(wp_data->object_instance, LOG_STATUS_LOG_DISABLED, true);
                        }
                    } else {
                        if(TL_Is_Enabled(wp_data->object_instance)) {
                            /* Have really gone from disabled to enabled as 
                             * enable flag and times were correct
                             */
                            TL_Insert_Status_Rec(wp_data->object_instance, LOG_STATUS_LOG_DISABLED, false);
                        }
                    }
                }
                
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;

        case PROP_STOP_WHEN_FULL:
            if (value.tag == BACNET_APPLICATION_TAG_BOOLEAN) {
                /* Only trigger this on a change of state */
                if(CurrentLog->bStopWhenFull != value.type.Boolean) {
                    CurrentLog->bStopWhenFull = value.type.Boolean;
                    
                    if((value.type.Boolean == true) &&
                       (CurrentLog->ulRecordCount == TL_MAX_ENTRIES) &&
                       (CurrentLog->bEnable == true)) {
                       
                        /* When full log is switched from normal to stop when full
                         * disable the log and record the fact - see 135-2008 12.25.12
                         */
                        CurrentLog->bEnable = false;
                        TL_Insert_Status_Rec(wp_data->object_instance, LOG_STATUS_LOG_DISABLED, true);
                    }
                }    
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;

        case PROP_BUFFER_SIZE:
            /* Fixed size buffer so deny write. If buffer size was writable
             * we would probably erase the current log, resize, re-initalise
             * and carry on - however write is not allowed if enable is true.
             */
            *error_class = ERROR_CLASS_PROPERTY;
            *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            break;

        case PROP_RECORD_COUNT:
            if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
                if(value.type.Unsigned_Int == 0) {
                    /* Time to clear down the log */
                    CurrentLog->ulRecordCount = 0;
                    CurrentLog->iIndex = 0;
                    TL_Insert_Status_Rec(wp_data->object_instance, LOG_STATUS_BUFFER_PURGED, true);
                }
                status = true;
            } else {
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_INVALID_DATA_TYPE;
            }
            break;

        case PROP_LOGGING_TYPE:
            /* logic 
             * triggered and polled options.
             */ 
        
            if (value.tag == BACNET_APPLICATION_TAG_ENUMERATED) {
                if(value.type.Enumerated != LOGGING_TYPE_COV) {
                    CurrentLog->LoggingType = value.type.Enumerated;
                    if(value.type.Enumerated == LOGGING_TYPE_POLLED) {
                        /* As per 12.25.27 pick a suitable default if interval is 0 */
                        if(CurrentLog->ulLogInterval == 0) {
                            CurrentLog->ulLogInterval = 900;
                        }
                    }
                    if(value.type.Enumerated == LOGGING_TYPE_TRIGGERED) {
                        /* As per 12.25.27 0 the interval if triggered logging selected */
                        CurrentLog->ulLogInterval = 0;
                    }
                    status = true;
                } else {
                    /* We don't currently support COV */
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
                /* First record the current enable state of the log */
                bEffectiveEnable = TL_Is_Enabled(wp_data->object_instance);
                CurrentLog->StartTime.date = TempDate; /* Safe to copy the date now */
                CurrentLog->StartTime.time = value.type.Time;
                                    
                if (datetime_wildcard_present(&CurrentLog->StartTime)) {
                    /* Mark start time as wild carded */
                    CurrentLog->ucTimeFlags |= TL_T_START_WILD;
                    CurrentLog->tStartTime = 0; 
                } else {
                    /* Clear wild card flag and set time in local format */
                    CurrentLog->ucTimeFlags &= ~TL_T_START_WILD;
                    CurrentLog->tStartTime = TL_BAC_Time_To_Local(&CurrentLog->StartTime);
                }
                
                if(bEffectiveEnable != TL_Is_Enabled(wp_data->object_instance)) {
                    /* Enable status has changed because of time update */
                    if(bEffectiveEnable == true) {
                        /* Say we went from enabled to disabled */
                        TL_Insert_Status_Rec(wp_data->object_instance, LOG_STATUS_LOG_DISABLED, true);
                    } else {
                        /* Say we went from disabled to enabled */
                        TL_Insert_Status_Rec(wp_data->object_instance, LOG_STATUS_LOG_DISABLED, false);
                    }
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
                /* First record the current enable state of the log */
                bEffectiveEnable = TL_Is_Enabled(wp_data->object_instance);
                CurrentLog->StopTime.date = TempDate; /* Safe to copy the date now */
                CurrentLog->StopTime.time = value.type.Time;
                                    
                if (datetime_wildcard_present(&CurrentLog->StopTime)) {
                    /* Mark stop time as wild carded */
                    CurrentLog->ucTimeFlags |= TL_T_STOP_WILD;
                    CurrentLog->tStopTime = 0xFFFFFFFF; /* Fixme: how do we set this to max for time_t ? */
                } else {
                    /* Clear wild card flag and set time in local format */
                    CurrentLog->ucTimeFlags &= ~TL_T_STOP_WILD;
                    CurrentLog->tStopTime = TL_BAC_Time_To_Local(&CurrentLog->StopTime);
                }
                          
                if(bEffectiveEnable != TL_Is_Enabled(wp_data->object_instance)) {
                    /* Enable status has changed because of time update */
                    if(bEffectiveEnable == true) {
                        /* Say we went from enabled to disabled */
                        TL_Insert_Status_Rec(wp_data->object_instance, LOG_STATUS_LOG_DISABLED, true);
                    } else {
                        /* Say we went from disabled to enabled */
                        TL_Insert_Status_Rec(wp_data->object_instance, LOG_STATUS_LOG_DISABLED, false);
                    }
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
                    /* Still some remaining so fetch potential device ID */
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
                
                if(value.context_tag == 3) {
                    /* Got a device ID so deal with it */
                    TempSource.deviceIndentifier = value.type.Object_Id;
                    if((TempSource.deviceIndentifier.instance != Device_Object_Instance_Number()) ||
                       (TempSource.deviceIndentifier.type != OBJECT_DEVICE)) {
                         /* Not our ID so can't handle it at the moment */                
                         *error_class = ERROR_CLASS_PROPERTY;
                         *error_code = ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED;
                        break;
                    }
                }    
            }
            /* Make sure device ID is set to ours in case not supplied */
            TempSource.deviceIndentifier.type     = OBJECT_DEVICE;
            TempSource.deviceIndentifier.instance = Device_Object_Instance_Number();
            /* Quick comparison if structures are packed ... */
            if(memcmp(&TempSource, &CurrentLog->Source, sizeof(BACNET_OBJECT_ID)) != 0) {
                /* Clear buffer if property being logged is changed */
                CurrentLog->ulRecordCount = 0;
                CurrentLog->iIndex = 0;
                TL_Insert_Status_Rec(wp_data->object_instance, LOG_STATUS_BUFFER_PURGED, true);
            }
            CurrentLog->Source = TempSource;    
            status = true;
            break;

        case PROP_LOG_INTERVAL:
            if(CurrentLog->LoggingType == LOGGING_TYPE_TRIGGERED) {
                /* Read only if triggered log so flag error and bail out */
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_WRITE_ACCESS_DENIED;
            } else if((CurrentLog->LoggingType == LOGGING_TYPE_POLLED) &&
                      (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) &&
                      (value.type.Unsigned_Int == 0)) {
                /* We don't support COV at the moment so don't allow switching
                 * to it by clearing interval whilst in polling mode */
                *error_class = ERROR_CLASS_PROPERTY;
                *error_code = ERROR_CODE_OPTIONAL_FUNCTIONALITY_NOT_SUPPORTED;
            } else if (value.tag == BACNET_APPLICATION_TAG_UNSIGNED_INT) {
            /* We only log to 1 sec accuracy so must divide by 100 before passing it on */
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
                CurrentLog->bTrigger = value.type.Boolean;
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

bool TrendLogGetRRInfo(
    uint32_t           Object,   /* Which particular object */
    BACNET_PROPERTY_ID Property, /* Which property */
    RR_PROP_INFO      *pInfo,    /* Where to put the information */
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE  *error_code)
{
    if(Object >= MAX_TREND_LOGS) {
        *error_class = ERROR_CLASS_OBJECT;
        *error_code  = ERROR_CODE_UNKNOWN_OBJECT;
    } else if(Property == PROP_LOG_BUFFER) {
        pInfo->RequestTypes = RR_BY_POSITION | RR_BY_TIME | RR_BY_SEQUENCE;
        pInfo->Handler = rr_trend_log_encode;
        return(true);
    } else {
        *error_class = ERROR_CLASS_SERVICES;
        *error_code  = ERROR_CODE_PROPERTY_IS_NOT_A_LIST;
    }

    return(false);
}

/*****************************************************************************
 * Insert a status record into a trend log - does not check for enable/log   *
 * full, time slots and so on as these type of entries have to go in         *
 * irrespective of such things which means that valid readings may get       *
 * pushed out of the log to make room.                                       *
 *****************************************************************************/

void TL_Insert_Status_Rec(int iLog, BACNET_LOG_STATUS eStatus, bool bState)

{
    TL_LOG_INFO *CurrentLog;
    TL_DATA_REC  TempRec;
    
    CurrentLog = &LogInfo[iLog];
    
    TempRec.tTimeStamp = time(NULL);
    TempRec.ucRecType   = TL_TYPE_STATUS;
    TempRec.ucStatus    = 0;
    TempRec.Datum.ucLogStatus = 0;
    /* Note we set the bits in correct order so that we can place them directly
     * into the bitstring structure later on when we have to encode them */
    switch(eStatus) {
        case LOG_STATUS_LOG_DISABLED:
            if(bState)
                TempRec.Datum.ucLogStatus = 1 << LOG_STATUS_LOG_DISABLED;
            break;
        case LOG_STATUS_BUFFER_PURGED:
            if(bState)
                TempRec.Datum.ucLogStatus = 1 << LOG_STATUS_BUFFER_PURGED;
            break;
        case LOG_STATUS_LOG_INTERRUPTED:
            TempRec.Datum.ucLogStatus = 1 << LOG_STATUS_LOG_INTERRUPTED;
            break;
        default:
            break;
    }
    
    Logs[iLog][CurrentLog->iIndex++] = TempRec;
    if(CurrentLog->iIndex >= TL_MAX_ENTRIES)
        CurrentLog->iIndex = 0;
        
    CurrentLog->ulTotalRecordCount++;
    if(CurrentLog->ulRecordCount < TL_MAX_ENTRIES)
        CurrentLog->ulRecordCount++;
}

/*****************************************************************************
 * Use the combination of the enable flag and the enable times to determine  *
 * if the log is really enabled now. See 135-2008 sections 12.25.5 - 12.25.7 *
 *****************************************************************************/

bool TL_Is_Enabled(int iLog)
{
    TL_LOG_INFO *CurrentLog;
    time_t Now;
    bool bStatus;
    
    bStatus = true;
    CurrentLog = &LogInfo[iLog];
#if 0
        printf("\nFlags - %u, Start - %u, Stop - %u\n", (unsigned int)CurrentLog->ucTimeFlags,(unsigned int)CurrentLog->tStartTime, (unsigned int)CurrentLog->tStopTime);
#endif
    if(CurrentLog->bEnable == false) {
        /* Not enabled so time is irrelevant */
        bStatus = false;
    } else if((CurrentLog->ucTimeFlags == 0) && (CurrentLog->tStopTime < CurrentLog->tStartTime)) {
        /* Start time was after stop time as per 12.25.6 and 12.25.7 */
        bStatus = false;
    } else if(CurrentLog->ucTimeFlags != (TL_T_START_WILD | TL_T_STOP_WILD)) {
        /* enabled and either 1 wild card or none */
        Now = time(NULL);
#if 0
        printf("\nFlags - %u, Current - %u, Start - %u, Stop - %u\n", (unsigned int)CurrentLog->ucTimeFlags,(unsigned int)Now, (unsigned int)CurrentLog->tStartTime, (unsigned int)CurrentLog->tStopTime);
#endif
        if((CurrentLog->ucTimeFlags & TL_T_START_WILD) != 0) {
            /* wild card start time */
            if(Now > CurrentLog->tStopTime)
                bStatus = false;
        } else if((CurrentLog->ucTimeFlags & TL_T_STOP_WILD) != 0) {
            /* wild card stop time */
            if(Now < CurrentLog->tStartTime)
                bStatus = false;
        } else {
#if 0
        printf("\nCurrent - %u, Start - %u, Stop - %u\n", (unsigned int)Now, (unsigned int)CurrentLog->tStartTime, (unsigned int)CurrentLog->tStopTime);
#endif
            /* No wildcards so use both times */
            if((Now < CurrentLog->tStartTime) || (Now > CurrentLog->tStopTime))
                bStatus = false;                  
        }
    }
    
    return(bStatus);
}

/*****************************************************************************
 * Convert a BACnet time into a local time in seconds since the local epoch  *
 *****************************************************************************/

time_t TL_BAC_Time_To_Local(BACNET_DATE_TIME *SourceTime)
{
    struct tm LocalTime;
    
    LocalTime.tm_year = SourceTime->date.year - 1900; /* We store BACnet year in full format */
    LocalTime.tm_mon  = SourceTime->date.month - 1;
    LocalTime.tm_mday = SourceTime->date.day;
    LocalTime.tm_hour = SourceTime->time.hour;
    LocalTime.tm_min  = SourceTime->time.min;
    LocalTime.tm_sec  = SourceTime->time.sec;
    
    return(mktime(&LocalTime));
}

/*****************************************************************************
 * Convert a local time in seconds since the local epoch into a BACnet time  *
 *****************************************************************************/

void TL_Local_Time_To_BAC(BACNET_DATE_TIME *DestTime, time_t SourceTime)
{
    struct tm *TempTime;
    
    TempTime = localtime(&SourceTime);
    
    DestTime->date.year  = TempTime->tm_year + 1900;
    DestTime->date.month = TempTime->tm_mon + 1;
    DestTime->date.day   = TempTime->tm_mday;
    /* BACnet is 1 to 7 = Monday to Sunday
     * Windows is days from Sunday 0 - 6 so we
     * have to adjust */
    if(TempTime->tm_wday == 0)
        DestTime->date.wday = 7;
    else
        DestTime->date.wday  = TempTime->tm_wday;
    DestTime->time.hour  = TempTime->tm_hour;
    DestTime->time.min   = TempTime->tm_min;
    DestTime->time.sec   = TempTime->tm_sec;
    DestTime->time.hundredths = 0;
}

/****************************************************************************
 * Build a list of Trend Log entries from the Log Buffer property as        *
 * required for the ReadsRange functionality.                               *
 *                                                                          *
 * We have to support By Position, By Sequence and By Time requests.        *
 *                                                                          *
 * We need to treat the address cache as a contiguous array but in reality  *
 * it could be sparsely populated. We can get the count but we can only     *
 * extract entries by doing a linear scan starting from the first entry in  *
 * the cache and picking them off one by one.                               *
 *                                                                          *
 * We do assume the list cannot change whilst we are accessing it so would  *
 * not be multithread safe if there are other tasks that write to the log.  *
 *                                                                          *
 * We take the simple approach here to filling the buffer by taking a max   *
 * size for a single entry and then stopping if there is less than that     *
 * left in the buffer. You could build each entry in a seperate buffer and  *
 * determine the exact length before copying but this is time consuming,    *
 * requires more memory and would probably only let you sqeeeze one more    *
 * entry in on occasion. The value is calculated as 10 bytes for the time   *
 * stamp + 6 bytes for our largest data item (bit string capped at 32 bits) *
 * + 3 bytes for the status flags + 4 for the context tags to give 23.      *
 ****************************************************************************/

#define TL_MAX_ENC 23  /* Maximum size of encoded log entry, see above */

int rr_trend_log_encode(
    uint8_t *apdu,
    BACNET_READ_RANGE_DATA *pRequest,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE  *error_code)
{
    /* Initialise result flags to all false */
    bitstring_init(&pRequest->ResultFlags);
    bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_FIRST_ITEM, false);
    bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_LAST_ITEM,  false);
    bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_MORE_ITEMS, false);
    pRequest->ItemCount = 0; /* Start out with nothing */

    if((pRequest->RequestType == RR_BY_POSITION) || (pRequest->RequestType == RR_READ_ALL))
        return(TL_encode_by_position(apdu, pRequest, error_class, error_code));
    else {
        /* Reject by sequence and by time for the moment */
        *error_class = ERROR_CLASS_SERVICES;
        *error_code = ERROR_CODE_OTHER;
    }
    
    return(-1);
}

int TL_encode_by_position(
    uint8_t *apdu,
    BACNET_READ_RANGE_DATA *pRequest,
    BACNET_ERROR_CLASS *error_class,
    BACNET_ERROR_CODE  *error_code)
{
    int iLen;
    int32_t iTemp;
    TL_LOG_INFO *CurrentLog;

    uint32_t uiIndex;     /* Current entry number */
    uint32_t uiFirst;     /* Entry number we started encoding from */
    uint32_t uiLast;      /* Entry number we finished encoding on */
    uint32_t uiTarget;    /* Last entry we are required to encode */
    uint32_t uiRemaining; /* Amount of unused space in packet */
    
    uiFirst = 0;
    uiLast  = 0;
    iLen    = 0;
    uiRemaining = MAX_APDU - pRequest->Overhead; /* See how much space we have */

    CurrentLog = &LogInfo[pRequest->object_instance];
    
    if(CurrentLog->ulRecordCount == 0) /* Bail out now if nowt - should never happen for a Trend Log... */
        return(0);

    if(pRequest->RequestType == RR_READ_ALL) {
        /*
         * Read all the list or as much as will fit in the buffer by selecting
         * a range that covers the whole list and falling through to the next
         * section of code
         */
        pRequest->Count = CurrentLog->ulRecordCount;     /* Full list */
        pRequest->Range.RefIndex = 1;  /* Starting at the beginning */
        }

    if(pRequest->Count < 0) { /* negative count means work from index backwards */
        /*
         * Convert from end index/negative count to
         * start index/positive count and then process as
         * normal. This assumes that the order to return items
         * is always first to last, if this is not true we will
         * have to handle this differently. 
         *
         * Note: We need to be careful about how we convert these
         * values due to the mix of signed and unsigned types - don't
         * try to optimise the code unless you understand all the
         * implications of the data type conversions!
         */
        
        iTemp = pRequest->Range.RefIndex; /* pull out and convert to signed */
        iTemp += pRequest->Count + 1;     /* Adjust backwards, remember count is -ve */
        if(iTemp < 1) { /* if count is too much, return from 1 to start index */
            pRequest->Count = pRequest->Range.RefIndex;
            pRequest->Range.RefIndex = 1;
        }
        else { /* Otherwise adjust the start index and make count +ve */
            pRequest->Range.RefIndex = iTemp;
            pRequest->Count = -pRequest->Count;
        }
    }
    
    /* From here on in we only have a starting point and a positive count */
    
    if(pRequest->Range.RefIndex > CurrentLog->ulRecordCount)  /* Nothing to return as we are past the end of the list */
        return(0);
        
    uiTarget = pRequest->Range.RefIndex + pRequest->Count - 1; /* Index of last required entry */
    if(uiTarget > CurrentLog->ulRecordCount)                   /* Capped at end of list if necessary */
        uiTarget = CurrentLog->ulRecordCount;
        
    uiIndex = pRequest->Range.RefIndex;
    uiFirst = uiIndex; /* Record where we started from */
    while(uiIndex <= uiTarget) {
        if(uiRemaining < TL_MAX_ENC) {
            /*
             * Can't fit any more in! We just set the result flag to say there 
             * was more and drop out of the loop early
             */
            bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_MORE_ITEMS, true); 
            break;
        }               
        
        iTemp = TL_encode_entry(&apdu[iLen], pRequest->object_instance, uiIndex);    
        
        uiRemaining -= iTemp; /* Reduce the remaining space */
        iLen        += iTemp; /* and increase the length consumed */
        uiLast = uiIndex;       /* Record the last entry encoded */
        uiIndex++;              /* and get ready for next one */
        pRequest->ItemCount++;  /* Chalk up another one for the response count */
    }
    
    /* Set remaining result flags if necessary */
    if(uiFirst == 1)
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_FIRST_ITEM, true);
        
    if(uiLast == CurrentLog->ulRecordCount)
        bitstring_set_bit(&pRequest->ResultFlags, RESULT_FLAG_LAST_ITEM,  true);
    
return(iLen);
}


int TL_encode_entry(uint8_t *apdu, int iLog, int iEntry)
{
    int iLen;
    TL_DATA_REC *pSource;
    BACNET_BIT_STRING TempBits;    
    int iCount;
    BACNET_DATE_TIME TempTime;
    
    /* Convert from BACnet 1 based to 0 based array index and then
     * handle wrap around of the circular buffer */
     
    pSource = &Logs[iLog][(iEntry - 1) % TL_MAX_ENTRIES];
    iLen = 0;
    /* First stick the time stamp in with tag [0] */
    TL_Local_Time_To_BAC(&TempTime, pSource->tTimeStamp);
    iLen += bacapp_encode_context_datetime(apdu, 0, &TempTime);
    
    /* Next comes the actual entry with tag [1] */
    iLen += encode_opening_tag(&apdu[iLen], 1);
    /* The data entry is tagged individually [0] - [10] to indicate which type */
    switch(pSource->ucRecType) {
        case TL_TYPE_STATUS:
            /* Build bit string directly from the stored octet */
            bitstring_init(&TempBits);
            bitstring_set_bits_used(&TempBits, 1, 5);
            bitstring_set_octet(&TempBits, 0, pSource->Datum.ucLogStatus);
            iLen += encode_context_bitstring(&apdu[iLen], pSource->ucRecType, &TempBits);
            break;
            
        case TL_TYPE_BOOL:
            iLen += encode_context_boolean(&apdu[iLen], pSource->ucRecType, pSource->Datum.ucBoolean);
            break;
            
        case TL_TYPE_REAL:
            iLen += encode_context_real(&apdu[iLen], pSource->ucRecType, pSource->Datum.fReal);
            break;
            
        case TL_TYPE_ENUM:
            iLen += encode_context_enumerated(&apdu[iLen], pSource->ucRecType, pSource->Datum.ulEnum);
            break;
            
        case TL_TYPE_UNSIGN:
            iLen += encode_context_unsigned(&apdu[iLen], pSource->ucRecType, pSource->Datum.ulUValue);
            break;
            
        case TL_TYPE_SIGN:
            iLen += encode_context_signed(&apdu[iLen], pSource->ucRecType, pSource->Datum.lSValue);
            break;
            
        case TL_TYPE_BITS:
            /* Rebuild bitstring directly from stored octets - which we
             * have limited to 32 bits maximum as allowed by the standard
             */
            bitstring_init(&TempBits);
            bitstring_set_bits_used(&TempBits, (pSource->Datum.Bits.ucLen >> 4) & 0x0F, pSource->Datum.Bits.ucLen & 0x0F);
            for(iCount = pSource->Datum.Bits.ucLen >> 4; iCount > 0; iCount--)
                bitstring_set_octet(&TempBits, iCount - 1, pSource->Datum.Bits.ucStore[iCount-1]);
                
            iLen += encode_context_bitstring(&apdu[iLen], pSource->ucRecType, &TempBits);
            break;
            
        case TL_TYPE_NULL:
            iLen += encode_context_null(&apdu[iLen], pSource->ucRecType);
            break;
            
        case TL_TYPE_ERROR:
            iLen += encode_opening_tag(&apdu[iLen], TL_TYPE_ERROR);
            iLen += encode_application_enumerated(&apdu[iLen], pSource->Datum.Error.usClass);
            iLen += encode_application_enumerated(&apdu[iLen], pSource->Datum.Error.usCode);
            iLen += encode_closing_tag(&apdu[iLen], TL_TYPE_ERROR);
            break;
            
        case TL_TYPE_DELTA:
            iLen += encode_context_real(&apdu[iLen], pSource->ucRecType, pSource->Datum.fTime);
            break;
            
        case TL_TYPE_ANY:
            /* Should never happen as we don't support this at the moment */
            break;
    }

    iLen += encode_closing_tag(&apdu[iLen], 1);
    /* Check if status bit string is required and insert with tag [2] */
    if((pSource->ucStatus & 128) == 128) {
        bitstring_init(&TempBits);
        bitstring_set_bits_used(&TempBits, 1, 4);
        /* only insert the 1st 4 bits - in BACnet order */
        bitstring_set_octet(&TempBits, 0, (pSource->Datum.ucLogStatus & 0xF0));
        iLen += encode_context_bitstring(&apdu[iLen], 2, &TempBits);
    }

    return(iLen);
}