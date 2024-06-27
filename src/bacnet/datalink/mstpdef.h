/**
 * @file
 * @brief Defines for the BACnet MS/TP finite state machines
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2009
 * @copyright SPDX-License-Identifier: MIT
 * @defgroup DLMSTP BACnet MS/TP DataLink
 * @ingroup DataLink
 */
#ifndef BACNET_MSTPDEF_H
#define BACNET_MSTPDEF_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

/*  The value 255 is used to denote broadcast when used as a */
/* destination address but is not allowed as a value for a station. */
/* Station addresses for master nodes can be 0-127.  */
/* Station addresses for slave nodes can be 0-254.  */
#define MSTP_BROADCAST_ADDRESS 255

/* MS/TP Frame Type */
/* Frame Types 8 through 127 are reserved by ASHRAE. */
#define FRAME_TYPE_TOKEN 0
#define FRAME_TYPE_POLL_FOR_MASTER 1
#define FRAME_TYPE_REPLY_TO_POLL_FOR_MASTER 2
#define FRAME_TYPE_TEST_REQUEST 3
#define FRAME_TYPE_TEST_RESPONSE 4
#define FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY 5
#define FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY 6
#define FRAME_TYPE_REPLY_POSTPONED 7
#define FRAME_TYPE_BACNET_EXTENDED_DATA_EXPECTING_REPLY 32
#define FRAME_TYPE_BACNET_EXTENDED_DATA_NOT_EXPECTING_REPLY 33
#define FRAME_TYPE_IPV6_ENCAPSULATION 34
/* Frame Types 128 through 255: Proprietary Frames */
/* These frames are available to vendors as proprietary (non-BACnet) frames. */
/* The first two octets of the Data field shall specify the unique vendor */
/* identification code, most significant octet first, for the type of */
/* vendor-proprietary frame to be conveyed.  */
#define FRAME_TYPE_PROPRIETARY_MIN 128
#define FRAME_TYPE_PROPRIETARY_MAX 255
/* The initial CRC16 checksum value */
#define CRC16_INITIAL_VALUE (0xFFFF)
#define CRC32K_INITIAL_VALUE (0xFFFFFFFF)
#define CRC32K_RESIDUE (0x0843323B)
/* frame specific data */
#define MSTP_PREAMBLE_X55 (0x55)
/* The length of the data portion of a Test_Request, Test_Response,
   BACnet Data Expecting Reply, or BACnet Data Not Expecting Reply frame 
   may range from 0 to 501 octets. 
   The length of the data portion of a proprietary frame shall 
   be in the range of 2 to 501 octets.*/
#define MSTP_FRAME_NPDU_MAX 501
/* COBS-encoded frames data parameter length is between 
   502 and 1497 octets, inclusive */
#define MSTP_EXTENDED_FRAME_NPDU_MAX 1497

/* receive FSM states */
typedef enum {
    MSTP_RECEIVE_STATE_IDLE = 0,
    MSTP_RECEIVE_STATE_PREAMBLE = 1,
    MSTP_RECEIVE_STATE_HEADER = 2,
    MSTP_RECEIVE_STATE_DATA = 3,
    MSTP_RECEIVE_STATE_SKIP_DATA = 4
} MSTP_RECEIVE_STATE;

/* master node FSM states */
typedef enum {
    MSTP_MASTER_STATE_INITIALIZE = 0,
    MSTP_MASTER_STATE_IDLE = 1,
    MSTP_MASTER_STATE_USE_TOKEN = 2,
    MSTP_MASTER_STATE_WAIT_FOR_REPLY = 3,
    MSTP_MASTER_STATE_DONE_WITH_TOKEN = 4,
    MSTP_MASTER_STATE_PASS_TOKEN = 5,
    MSTP_MASTER_STATE_NO_TOKEN = 6,
    MSTP_MASTER_STATE_POLL_FOR_MASTER = 7,
    MSTP_MASTER_STATE_ANSWER_DATA_REQUEST = 8
} MSTP_MASTER_STATE;

/* MSTP zero config FSM states */
typedef enum MSTP_Zero_Config_State {
    MSTP_ZERO_CONFIG_STATE_INIT = 0,
    MSTP_ZERO_CONFIG_STATE_IDLE = 1,
    MSTP_ZERO_CONFIG_STATE_LURK = 2,
    MSTP_ZERO_CONFIG_STATE_CLAIM = 3,
    MSTP_ZERO_CONFIG_STATE_CONFIRM = 4,
    MSTP_ZERO_CONFIG_STATE_USE = 5
} MSTP_ZERO_CONFIG_STATE;

/* The time without a DataAvailable or ReceiveError event before declaration */
/* of loss of token: 500 milliseconds. */
#define Tno_token 500

/* The minimum time after the end of the stop bit of the final octet of a */
/* received frame before a node may enable its EIA-485 driver: 40 bit times. */
/* At 9600 baud, 40 bit times would be about 4.166 milliseconds */
/* At 19200 baud, 40 bit times would be about 2.083 milliseconds */
/* At 38400 baud, 40 bit times would be about 1.041 milliseconds */
/* At 57600 baud, 40 bit times would be about 0.694 milliseconds */
/* At 76800 baud, 40 bit times would be about 0.520 milliseconds */
/* At 115200 baud, 40 bit times would be about 0.347 milliseconds */
/* 40 bits is 4 octets including a start and stop bit with each octet */
#define Tturnaround (40UL)
/* turnaround_time_milliseconds = (Tturnaround*1000UL)/RS485_Baud; */

/* The number of tokens received or used before a Poll For Master cycle */
/* is executed: 50. */
#define Npoll 50

/* The minimum number of polls received before a zero-config address */
/* is claimed: 8. */
#define Nmin_poll 8

/* The first zero-config address: 64 */
#define Nmin_poll_station 64

/* The last zero-config address: 127 */
#define Nmax_poll_station 127

/* The number of zero-config station poll slots: 64 */
#define Nmax_poll_slot 64

/* The last master node address: 127 */
#define Nmax_master_station 127

/* The number of retries on sending Token: 1. */
#define Nretry_token 1

/* The maximum idle time a sending node may allow to elapse between octets */
/* of a frame the node is transmitting: 20 bit times. */
#define Tframe_gap 20

/* The maximum time after the end of the stop bit of the final */
/* octet of a transmitted frame before a node must disable its */
/* EIA-485 driver: 15 bit times. */
#define Tpostdrive 15

/* The width of the time slot within which a node may generate a token: */
/* 10 milliseconds. */
#define Tslot 10

/* The maximum time a node may wait after reception of the token or */
/* a Poll For Master frame before sending the first octet of a frame: */
/* 15 milliseconds. */
#define Tusage_delay 15

/* The minimum number of DataAvailable or ReceiveError events that must be */
/* seen by a receiving node in order to declare the line "active": 4. */
#define Nmin_octets 4

#define DEFAULT_Tframe_abort 95
#define DEFAULT_Treply_delay 245
#define DEFAULT_Treply_timeout 250
#define DEFAULT_Tusage_timeout 35

#define DEFAULT_MAX_INFO_FRAMES 1
#define DEFAULT_MAX_MASTER 127
#define DEFAULT_MAC_ADDRESS 127

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
