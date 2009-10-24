/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2009 Peter Mc Shane

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to:
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/

struct BACnet_Read_Range_Data;
typedef struct BACnet_Read_Range_Data {
    BACNET_OBJECT_TYPE object_type;
    uint32_t object_instance;
    BACNET_PROPERTY_ID object_property;
    uint32_t array_index;
    uint8_t *application_data;
    int application_data_len;
    BACNET_BIT_STRING ResultFlags;      /*  FIRST_ITEM, LAST_ITEM, MORE_ITEMS */
    int RequestType;    /* Index, sequence or time based request */
    uint32_t ItemCount;
    uint32_t FirstSequence;
    union {     /* Pick the appropriate data type */
        uint32_t RefIndex;
        uint32_t RefSeqNum;
        BACNET_DATE_TIME RefTime;
    } Range;
    int32_t Count;      /* SIGNED value as +ve vs -ve  is important */
} BACNET_READ_RANGE_DATA;

/* Defines to indicate which type of read range request it is */

#define RR_BY_POSITION 0
#define RR_BY_SEQUENCE 1
#define RR_BY_TIME     2
#define RR_READ_ALL    4        /* Read all of array - so don't send any range in the request */

/* Bit String Enumerations */
typedef enum {
    RESULT_FLAG_FIRST_ITEM = 0,
    RESULT_FLAG_LAST_ITEM = 1,
    RESULT_FLAG_MORE_ITEMS = 2
} BACNET_RESULT_FLAGS;

int rr_encode_apdu(
    uint8_t * apdu,
    uint8_t invoke_id,
    BACNET_READ_RANGE_DATA * rrdata);

int rr_decode_service_request(
    uint8_t * apdu,
    unsigned apdu_len,
    BACNET_READ_RANGE_DATA * rrdata);

int rr_ack_encode_apdu(
    uint8_t * apdu,
    uint8_t invoke_id,
    BACNET_READ_RANGE_DATA * rrdata);

int rr_ack_decode_service_request(
    uint8_t * apdu,
    int apdu_len,       /* total length of the apdu */
    BACNET_READ_RANGE_DATA * rrdata);

uint8_t Send_ReadRange_Request(
    uint32_t device_id, /* destination device */
    BACNET_READ_RANGE_DATA * read_access_data);
