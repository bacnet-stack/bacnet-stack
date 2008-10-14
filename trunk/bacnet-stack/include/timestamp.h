#ifndef _TIMESTAMP_H_
#define _TIMESTAMP_H_

#include "bacdcode.h"

typedef enum {
	TIME_STAMP_TIME		= 0,
	TIME_STAMP_SEQUENCE = 1,
	TIME_STAMP_DATETIME = 2,
} BACNET_TIMESTAMP_TAG;

typedef uint8_t TYPE_BACNET_TIMESTAMP_TYPE;

typedef struct {
	TYPE_BACNET_TIMESTAMP_TYPE  tag;
	union {
		BACNET_TIME					time;
		uint16_t					sequenceNum;
		BACNET_DATE_TIME			dateTime;
	} value;
} BACNET_TIMESTAMP;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */



int bacapp_encode_context_timestamp(
    uint8_t * apdu,
	uint8_t   tag_number,
    BACNET_TIMESTAMP * value);

int bacapp_decode_context_timestamp(
    uint8_t * apdu,
	uint8_t   tag_number,
    BACNET_TIMESTAMP * value);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
