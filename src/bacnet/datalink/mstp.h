/**
 * @file
 * @brief API for the BACnet MS/TP finite state machines and their data
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2004
 * @copyright SPDX-License-Identifier: MIT
 * @defgroup DLMSTP BACnet MS/TP DataLink
 * @ingroup DataLink
 */
#ifndef BACNET_MSTP_H
#define BACNET_MSTP_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/datalink/mstpdef.h"

/* Repeater turnoff delay. The duration of a continuous logical one state */
/* at the active input port of an MS/TP repeater after which the repeater */
/* will enter the IDLE state: 29 bit times < Troff < 40 bit times. */
#ifndef Troff
#define Troff 30
#endif

/* size of the buffer used to send and validate a unique test request */
#define MSTP_UUID_SIZE 16

struct mstp_port_struct_t {
    MSTP_RECEIVE_STATE receive_state;
    /* When a master node is powered up or reset, */
    /* it shall unconditionally enter the INITIALIZE state. */
    MSTP_MASTER_STATE master_state;
    /* A Boolean flag set to TRUE by the Receive State Machine  */
    /* if an error is detected during the reception of a frame.  */
    /* Set to FALSE by the Master or Slave Node state machine. */
    unsigned ReceiveError : 1;
    /* There is data in the buffer */
    unsigned DataAvailable : 1;
    unsigned ReceivedInvalidFrame : 1;
    /* A Boolean flag set to TRUE by the Receive State Machine  */
    /* if a valid frame is received.  */
    /* Set to FALSE by the Master or Slave Node state machine. */
    unsigned ReceivedValidFrame : 1;
    /* A Boolean flag set to TRUE by the master machine if this node is the
       only known master node. */
    unsigned SoleMaster : 1;
    /* A Boolean flag set to TRUE if this node is a slave node */
    unsigned SlaveNodeEnabled : 1;
    /* A Boolean flag set to TRUE if this node is using a ZeroConfig address */
    unsigned ZeroConfigEnabled : 1;
    /* stores the latest received data */
    uint8_t DataRegister;
    /* Used to accumulate the CRC on the data field of a frame. */
    uint16_t DataCRC;
    /* Used to store the actual CRC from the data field. */
    uint8_t DataCRCActualMSB;
    uint8_t DataCRCActualLSB;
    /* Used to store the data length of a received frame. */
    uint16_t DataLength;
    /* Used to store the destination address of a received frame. */
    uint8_t DestinationAddress;
    /* Used to count the number of received octets or errors.
       This is used in the detection of link activity.
       Compared to Nmin_octets */
    uint8_t EventCount;
    /* Used to store the frame type of a received frame. */
    uint8_t FrameType;
    /* The number of frames sent by this node during a single token hold.
       When this counter reaches the value Nmax_info_frames, the node must */
    /* pass the token. */
    uint8_t FrameCount;
    /* Used to accumulate the CRC on the header of a frame. */
    uint8_t HeaderCRC;
    /* Used to store the actual CRC from the header. */
    uint8_t HeaderCRCActual;
    /* Used as an index by the Receive State Machine,
       up to a maximum value of InputBufferSize. */
    uint32_t Index;
    /* An array of octets, used to store octets as they are received.
       InputBuffer is indexed from 0 to InputBufferSize-1. */
    /* Note: assign this to an actual array of bytes! */
    /* Note: the buffer is designed as a pointer since some compilers
       and microcontroller architectures have limits as to places to
       hold contiguous memory. */
    uint8_t *InputBuffer;
    uint16_t InputBufferSize;
    /* "Next Station," the MAC address of the node to which
       This Station passes */
    /* the token. If the Next_Station is unknown, Next_Station
       shall be equal to */
    /* This_Station. */
    uint8_t Next_Station;
    /* "Poll Station," the MAC address of the node to which This Station last
       sent a Poll For Master. This is used during token maintenance. */
    uint8_t Poll_Station;
    /* A counter of transmission retries used for Token and Poll For Master
       transmission. */
    unsigned RetryCount;
    /* A timer with nominal 5 millisecond resolution used to measure
       and generate silence on the medium between octets. It is
       incremented by a timer process and is cleared by the Receive
       State Machine when activity is detected and by the SendFrame
       procedure as each octet is transmitted. */
    /* Since the timer resolution is limited and the timer is not necessarily
       synchronized to other machine events, a timer value of N will actually
       denote intervals between N-1 and N */
    /* Note: done here as functions - put into timer task or ISR
       so that you can be atomic on 8 bit microcontrollers */
    uint32_t (*SilenceTimer)(void *pArg);
    void (*SilenceTimerReset)(void *pArg);

    /* A timer used to measure and generate Reply Postponed frames.  It is
       incremented by a timer process and is cleared by the Master Node State
       Machine when a Data Expecting Reply Answer activity is completed.
       note: we always send a reply postponed since a message other than
       the reply may be in the transmit queue */
    /*    uint16_t ReplyPostponedTimer; */

    /* Used to store the Source Address of a received frame. */
    uint8_t SourceAddress;

    /* The number of tokens received by this node. When this counter
       reaches the value Npoll, the node polls the address range between
       TS and NS for additional master nodes. TokenCount is set to zero
       at the end of the polling process. */
    unsigned TokenCount;

    /* "This Station," the MAC address of this node. TS is generally read from a
       hardware DIP switch, or from nonvolatile memory. Valid values for TS are
       0 to 254. The value 255 is used to denote broadcast when used as a
       destination address but is not allowed as a value for TS. */
    uint8_t This_Station;

    /* This parameter represents the value of the Max_Info_Frames property of
       the node's Device object. The value of Max_Info_Frames specifies the
       maximum number of information frames the node may send before it must
       pass the token. Max_Info_Frames may have different values on different
       nodes. This may be used to allocate more or less of the available link
       bandwidth to particular nodes. If Max_Info_Frames is not writable in a
       node, its value shall be 1.*/
    uint8_t Nmax_info_frames;

    /* This parameter represents the value of the Max_Master property of the
       node's Device object. The value of Max_Master specifies the highest
       allowable address for master nodes. The value of Max_Master shall be
       less than or equal to 127. If Max_Master is not writable in a node,
       its value shall be 127. */
    uint8_t Nmax_master;

    /* An array of octets, used to store octets for transmitting
       OutputBuffer is indexed from 0 to OutputBufferSize-1.
       FIXME: assign this to an actual array of bytes!
       Note: the buffer is designed as a pointer since some compilers
       and microcontroller architectures have limits as to places to
       hold contiguous memory. */
    uint8_t *OutputBuffer;
    uint16_t OutputBufferSize;

    /* orderly transition tracking for zero-configuration node startup */
    MSTP_ZERO_CONFIG_STATE Zero_Config_State;
    /* the MAC address that this node is testing for MAC addresses
       that are not in-use.*/
    uint8_t Zero_Config_Station;
    /* the MAC address that this node prefers to use.*/
    uint8_t Zero_Config_Preferred_Station;
    /* Used to count the number of received poll-for-master frames
       This is used in the detection of addresses not in-use. */
    uint8_t Poll_Count;
    /* This parameter is random value 1..64, used to choose the poll slot */
    uint8_t Npoll_slot;
    /* UUID for storing the unique identifier for this node
       used to send and validate a unique test request and response */
    uint8_t UUID[MSTP_UUID_SIZE];
    /* amount of silence time to wait, in milliseconds */
    uint32_t Zero_Config_Silence;
    /* This parameter tracks the highest polled station address.
       The value of this parameter shall be less than or equal to 127.
       In the absence of other fixed address nodes, this value shall be 127. */
    uint8_t Zero_Config_Max_Master;

    /* The minimum time without a DataAvailable or ReceiveError event within
       a frame before a receiving node may discard the frame: 60 bit times.
       Implementations may use larger values for this timeout,
       not to exceed 100 milliseconds.
       Tframe_abort = 1 + ((60*1000UL)/RS485_Baud); */
    uint8_t Tframe_abort;

    /* The maximum time a node may wait after reception of a frame that
       expects a reply before sending the first octet of a reply or
       Reply Postponed frame: 250 milliseconds. */
    uint8_t Treply_delay;

    /* The minimum time without a DataAvailable or ReceiveError event
       that a node must wait for a station to begin replying to a
       confirmed request: 255 milliseconds. (Implementations may use
       larger values for this timeout, not to exceed 300 milliseconds.) */
    uint16_t Treply_timeout;

    /* The minimum time without a DataAvailable or ReceiveError event
       that a node must wait for a remote node to begin using a token
       or replying to a Poll For Master frame: 20 milliseconds.
       (Implementations may use larger values for this timeout,
       not to exceed 35 milliseconds.) */
    uint8_t Tusage_timeout;

    /* The minimum time after the end of the stop bit of the final
       octet of a received frame before a node may enable its
       EIA-485 driver: 40 bit times.
       40 bits is 4 octets including a start and stop bit with each octet.
       turnaround_time_milliseconds = (Tturnaround*1000UL)/RS485_Baud; */
    uint8_t Tturnaround_timeout;

    /* orderly transition tracking for auto-baud node startup */
    MSTP_AUTO_BAUD_STATE Auto_Baud_State;
    /* A Boolean flag set to TRUE if this node is checking frames for
       automatic baud rate detection */
    unsigned CheckAutoBaud : 1;
    /* The number of elapsed milliseconds since the last received valid frame */
    uint32_t (*ValidFrameTimer)(void *pArg);
    void (*ValidFrameTimerReset)(void *pArg);
    /* The number of header frames received with good CRC since
       initialization at the current trial baudrate. */
    uint8_t ValidFrames;
    /** Get the current baud rate */
    uint32_t (*BaudRate)(void);
    /** Set the current baud rate */
    void (*BaudRateSet)(uint32_t baud);
    /* The zero-based index in TestBaudrates of the next baudrate to try. */
    unsigned BaudRateIndex;

    /*Platform-specific port data */
    void *UserData;
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

BACNET_STACK_EXPORT
void MSTP_Init(struct mstp_port_struct_t *mstp_port);
BACNET_STACK_EXPORT
void MSTP_Receive_Frame_FSM(struct mstp_port_struct_t *mstp_port);
BACNET_STACK_EXPORT
bool MSTP_Master_Node_FSM(struct mstp_port_struct_t *mstp_port);
BACNET_STACK_EXPORT
void MSTP_Slave_Node_FSM(struct mstp_port_struct_t *mstp_port);

/* returns true if line is active */
BACNET_STACK_EXPORT
bool MSTP_Line_Active(const struct mstp_port_struct_t *mstp_port);

BACNET_STACK_EXPORT
uint16_t MSTP_Create_Frame(
    uint8_t *buffer,
    uint16_t buffer_len,
    uint8_t frame_type,
    uint8_t destination,
    uint8_t source,
    const uint8_t *data,
    uint16_t data_len);

BACNET_STACK_EXPORT
void MSTP_Create_And_Send_Frame(
    struct mstp_port_struct_t *mstp_port,
    uint8_t frame_type,
    uint8_t destination,
    uint8_t source,
    const uint8_t *data,
    uint16_t data_len);

BACNET_STACK_EXPORT
void MSTP_Fill_BACnet_Address(BACNET_ADDRESS *src, uint8_t mstp_address);

BACNET_STACK_EXPORT
void MSTP_Zero_Config_UUID_Init(struct mstp_port_struct_t *mstp_port);

BACNET_STACK_EXPORT
unsigned MSTP_Zero_Config_Station_Increment(unsigned station);

BACNET_STACK_EXPORT
void MSTP_Zero_Config_FSM(struct mstp_port_struct_t *mstp_port);

BACNET_STACK_EXPORT
uint32_t MSTP_Auto_Baud_Rate(unsigned baud_rate_index);

BACNET_STACK_EXPORT
void MSTP_Auto_Baud_FSM(struct mstp_port_struct_t *mstp_port);

/* functions used by the MS/TP state machine to put or get data */
/* FIXME: developer must implement these in their DLMSTP module */

BACNET_STACK_EXPORT
uint16_t MSTP_Put_Receive(struct mstp_port_struct_t *mstp_port);

/* for the MS/TP state machine to use for getting data to send */
/* Return: amount of PDU data */
BACNET_STACK_EXPORT
uint16_t MSTP_Get_Send(
    struct mstp_port_struct_t *mstp_port,
    unsigned timeout); /* milliseconds to wait for a packet */
/* for the MS/TP state machine to use for getting the reply for
   Data-Expecting-Reply Frame */
/* Return: amount of PDU data */
BACNET_STACK_EXPORT
uint16_t MSTP_Get_Reply(
    struct mstp_port_struct_t *mstp_port,
    unsigned timeout); /* milliseconds to wait for a packet */

BACNET_STACK_EXPORT
void MSTP_Send_Frame(
    struct mstp_port_struct_t *mstp_port,
    const uint8_t *buffer,
    uint16_t nbytes);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
