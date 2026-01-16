/**
 * @file
 * @author Peter Mc Shane <petermcs@users.sourceforge.net>
 * @date 2009
 * @brief API for a basic Trend Log object implementation.
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BACNET_BASIC_OBJECT_TRENDLOG_H
#define BACNET_BASIC_OBJECT_TRENDLOG_H
#include <stdbool.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/cov.h"
#include "bacnet/datetime.h"
#include "bacnet/readrange.h"
#include "bacnet/rp.h"
#include "bacnet/wp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Error code for Trend Log storage */
typedef struct tl_error {
    uint16_t usClass;
    uint16_t usCode;
} TL_ERROR;

/* Bit string of up to 32 bits for Trend Log storage */

typedef struct tl_bits {
    /* Bytes used in upper nibble/bits free in lower nibble */
    uint8_t ucLen;
    uint8_t ucStore[4];
} TL_BITS;

/* Storage structure for Trend Log data
 *
 * Note. I've tried to minimize the storage requirements here
 * as the memory requirements for logging in embedded
 * implementations are frequently a big issue. For PC or
 * embedded Linux type setupz this may seem like overkill
 * but if you have limited memory and need to squeeze as much
 * logging capacity as possible every little byte counts!
 */

typedef struct tl_data_record {
    bacnet_time_t tTimeStamp; /* When the event occurred */
    uint8_t ucRecType; /* What type of Event */
    /* Optional Status for read value in b0-b2, b7 = 1 if status is used */
    uint8_t ucStatus;
    union {
        uint8_t ucLogStatus; /* Change of log state flags */
        uint8_t ucBoolean; /* Stored boolean value */
        float fReal; /* Stored floating point value */
        uint32_t ulEnum; /* Stored enumerated value - max 32 bits */
        uint32_t ulUValue; /* Stored unsigned value - max 32 bits */
        int32_t lSValue; /* Stored signed value - max 32 bits */
        TL_BITS Bits; /* Stored bitstring - max 32 bits */
        TL_ERROR Error; /* Two part error class/code combo */
        float fTime; /* Interval value for change of time - seconds */
    } Datum;
} TL_DATA_REC;

#define TL_T_START_WILD 1 /* Start time is wild carded */
#define TL_T_STOP_WILD 2 /* Stop Time is wild carded */

#define TL_MAX_ENTRIES 1000 /* Entries per datalog */

/* Structure containing config and status info for a Trend Log */

typedef struct tl_log_info {
    bool bEnable; /* Trend log is active when this is true */
    BACNET_DATE_TIME StartTime; /* BACnet format start time */
    bacnet_time_t tStartTime; /* Local time working copy of start time */
    BACNET_DATE_TIME StopTime; /* BACnet format stop time */
    bacnet_time_t tStopTime; /* Local time working copy of stop time */
    uint8_t ucTimeFlags; /* Shorthand info on times */
    /* Where the data comes from */
    BACNET_DEVICE_OBJECT_PROPERTY_REFERENCE Source;
    uint32_t ulLogInterval; /* Time between entries in seconds */
    bool bStopWhenFull; /* Log halts when full if true */
    uint32_t ulRecordCount; /* Count of items currently in the buffer */
    /* Count of all items that have ever been inserted into the buffer */
    uint32_t ulTotalRecordCount;
    BACNET_LOGGING_TYPE LoggingType; /* Polled/cov/triggered */
    bool bAlignIntervals; /* If true align to the clock */
    /* Offset from start of period for taking reading in seconds */
    uint32_t ulIntervalOffset;
    bool bTrigger; /* Set to 1 to cause a reading to be taken */
    int iIndex; /* Current insertion point */
    bacnet_time_t tLastDataTime;
} TL_LOG_INFO;

/*
 * Data types associated with a BACnet Log Record. We use these for managing the
 * log buffer but they are also the tag numbers to use when encoding/decoding
 * the log datum field.
 */

#define TL_TYPE_STATUS 0
#define TL_TYPE_BOOL 1
#define TL_TYPE_REAL 2
#define TL_TYPE_ENUM 3
#define TL_TYPE_UNSIGN 4
#define TL_TYPE_SIGN 5
#define TL_TYPE_BITS 6
#define TL_TYPE_NULL 7
#define TL_TYPE_ERROR 8
#define TL_TYPE_DELTA 9
#define TL_TYPE_ANY 10 /* We don't support this particular can of worms! */

BACNET_STACK_EXPORT
void Trend_Log_Property_Lists(
    const int32_t **pRequired,
    const int32_t **pOptional,
    const int32_t **pProprietary);
BACNET_STACK_EXPORT
void Trend_Log_Writable_Property_List(
    uint32_t object_instance, const int32_t **properties);

BACNET_STACK_EXPORT
bool Trend_Log_Valid_Instance(uint32_t object_instance);
BACNET_STACK_EXPORT
unsigned Trend_Log_Count(void);
BACNET_STACK_EXPORT
uint32_t Trend_Log_Index_To_Instance(unsigned index);
BACNET_STACK_EXPORT
unsigned Trend_Log_Instance_To_Index(uint32_t instance);
BACNET_STACK_EXPORT
bool Trend_Log_Object_Instance_Add(uint32_t instance);

BACNET_STACK_EXPORT
bool Trend_Log_Object_Name(
    uint32_t object_instance, BACNET_CHARACTER_STRING *object_name);

BACNET_STACK_EXPORT
int Trend_Log_Read_Property(BACNET_READ_PROPERTY_DATA *rpdata);

BACNET_STACK_EXPORT
bool Trend_Log_Write_Property(BACNET_WRITE_PROPERTY_DATA *wp_data);
BACNET_STACK_EXPORT
void Trend_Log_Init(void);

BACNET_STACK_EXPORT
void TL_Insert_Status_Rec(int iLog, BACNET_LOG_STATUS eStatus, bool bState);

BACNET_STACK_EXPORT
bool TL_Is_Enabled(int iLog);

BACNET_STACK_EXPORT
bacnet_time_t TL_BAC_Time_To_Local(const BACNET_DATE_TIME *SourceTime);

BACNET_STACK_EXPORT
void TL_Local_Time_To_BAC(BACNET_DATE_TIME *DestTime, bacnet_time_t SourceTime);

BACNET_STACK_EXPORT
int TL_encode_entry(uint8_t *apdu, int iLog, int iEntry);

BACNET_STACK_EXPORT
int TL_encode_by_position(uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest);

BACNET_STACK_EXPORT
int TL_encode_by_sequence(uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest);

BACNET_STACK_EXPORT
int TL_encode_by_time(uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest);

BACNET_STACK_EXPORT
bool TrendLogGetRRInfo(
    BACNET_READ_RANGE_DATA *pRequest, /* Info on the request */
    RR_PROP_INFO *pInfo); /* Where to put the information */

BACNET_STACK_EXPORT
int rr_trend_log_encode(uint8_t *apdu, BACNET_READ_RANGE_DATA *pRequest);

BACNET_STACK_EXPORT
void trend_log_timer(uint16_t uSeconds);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
