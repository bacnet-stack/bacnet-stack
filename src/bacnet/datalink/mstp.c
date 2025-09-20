/**
 * @file
 * @brief Implementation of the finite state machines
 *  and BACnet Master-Slave Twisted Pair (MS/TP) functions
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2003
 * @details This clause describes a Master-Slave/Token-Passing (MS/TP)
 *  data link protocol, which provides the same services to the network layer
 *  as ISO 8802-2 Logical Link Control. It uses services provided by the
 *  EIA-485 physical layer. Relevant clauses of EIA-485 are deemed to be
 *  included in this standard by reference. The following hardware is assumed:
 *  (a) A UART (Universal Asynchronous Receiver/Transmitter) capable of
 *      transmitting and receiving eight data bits with one stop bit
 *      and no parity.
 *  (b) An EIA-485 transceiver whose driver may be disabled.
 *  (c) A timer with a resolution of five milliseconds or less
 *
 * @copyright SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 * @defgroup DLMSTP BACnet MS/TP DataLink Network Layer
 * @ingroup DataLink
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#if PRINT_ENABLED
#include <stdio.h>
#endif
#include "bacnet/datalink/cobs.h"
#include "bacnet/datalink/crc.h"
#include "bacnet/datalink/mstp.h"
#include "bacnet/datalink/crc.h"
#include "bacnet/datalink/mstptext.h"
#include "bacnet/npdu.h"
#include "bacnet/basic/sys/debug.h"

#if PRINT_ENABLED
#undef PRINT_ENABLED_RECEIVE
#undef PRINT_ENABLED_RECEIVE_DATA
#undef PRINT_ENABLED_RECEIVE_ERRORS
#undef PRINT_ENABLED_MASTER
#endif

#if defined(PRINT_ENABLED_RECEIVE)
#define printf_receive debug_printf
#else
static __inline__ void printf_receive(const char *format, ...)
{
    (void)format;
}
#endif

#if defined(PRINT_ENABLED_RECEIVE_DATA)
#define printf_receive_data debug_printf
#else
static __inline__ void printf_receive_data(const char *format, ...)
{
    (void)format;
}
#endif

#if defined(PRINT_ENABLED_RECEIVE_ERRORS)
#define printf_receive_error debug_printf
#else
static __inline__ void printf_receive_error(const char *format, ...)
{
    (void)format;
}
#endif

#if defined(PRINT_ENABLED_MASTER)
#define printf_master debug_printf
#else
static __inline__ void printf_master(const char *format, ...)
{
    (void)format;
}
#endif

/* MS/TP Frame Format */
/* All frames are of the following format: */
/* */
/* Preamble: two octet preamble: X`55', X`FF' */
/* Frame Type: one octet */
/* Destination Address: one octet address */
/* Source Address: one octet address */
/* Length: two octets, most significant octet first, of the Data field */
/* Header CRC: one octet */
/* Data: (present only if Length is non-zero) */
/* Data CRC: (present only if Length is non-zero) two octets, */
/*           least significant octet first */
/* (pad): (optional) at most one octet of padding: X'FF' */

/* we need to be able to increment without rolling over */
#define INCREMENT_AND_LIMIT_UINT8(x) \
    {                                \
        if (x < 0xFF)                \
            x++;                     \
    }

bool MSTP_Line_Active(const struct mstp_port_struct_t *mstp_port)
{
    if (!mstp_port) {
        return false;
    }

    return (mstp_port->EventCount > Nmin_octets);
}

void MSTP_Fill_BACnet_Address(BACNET_ADDRESS *src, uint8_t mstp_address)
{
    int i = 0;

    if (!src) {
        return;
    }
    if (mstp_address == MSTP_BROADCAST_ADDRESS) {
        /* mac_len = 0 if broadcast address */
        src->mac_len = 0;
        src->mac[0] = 0;
    } else {
        src->mac_len = 1;
        src->mac[0] = mstp_address;
    }
    /* fill with 0's starting with index 1; index 0 filled above */
    for (i = 1; i < MAX_MAC_LEN; i++) {
        src->mac[i] = 0;
    }
    src->net = 0;
    src->len = 0;
    for (i = 0; i < MAX_MAC_LEN; i++) {
        src->adr[i] = 0;
    }
}

/**
 * @brief Create an MS/TP Frame
 *
 * All MS/TP frames are of the following format:
 *   Preamble: two octet preamble: X`55', X`FF'
 *   Frame Type: one octet
 *   Destination Address: one octet address
 *   Source Address: one octet address
 *   Length: two octets, most significant octet first, of the Data field
 *   Header CRC: one octet
 *   Data: (present only if Length is non-zero)
 *   Data CRC: (present only if Length is non-zero) two octets,
 *     least significant octet first
 *   (pad): (optional) at most one octet of padding: X'FF'
 *
 * @param buffer - where frame is loaded
 * @param buffer_size - amount of space available in the buffer
 * @param frame_type - type of frame to send - see defines
 * @param destination - destination address
 * @param source - source address
 * @param data - any data to be sent - may be null
 * @param data_len - number of bytes of data
 * @return number of bytes encoded, or 0 on error
 */
uint16_t MSTP_Create_Frame(
    uint8_t *buffer,
    uint16_t buffer_size,
    uint8_t frame_type,
    uint8_t destination,
    uint8_t source,
    const uint8_t *data,
    uint16_t data_len)
{
    uint8_t crc8 = 0xFF; /* used to calculate the crc value */
    uint16_t crc16 = 0xFFFF; /* used to calculate the crc value */
    uint16_t index = 0; /* used to load the data portion of the frame */
    uint16_t cobs_len; /* length of the COBS encoded frame */
    bool cobs_bacnet_frame = false; /* true for COBS BACnet frames */

    if (!buffer) {
        return 0;
    }
    /* encode the data portion of the packet */
    if ((data_len > MSTP_FRAME_NPDU_MAX) ||
        ((frame_type >= Nmin_COBS_type) && (frame_type <= Nmax_COBS_type))) {
        /* COBS encoded frame */
        if (frame_type == FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY) {
            frame_type = FRAME_TYPE_BACNET_EXTENDED_DATA_EXPECTING_REPLY;
            cobs_bacnet_frame = true;
        } else if (frame_type == FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY) {
            frame_type = FRAME_TYPE_BACNET_EXTENDED_DATA_NOT_EXPECTING_REPLY;
            cobs_bacnet_frame = true;
        } else if (
            (frame_type < Nmin_COBS_type) || (frame_type > Nmax_COBS_type)) {
            /* I'm sorry, Dave, I'm afraid I can't do that. */
            return 0;
        }
        cobs_len =
            cobs_frame_encode(&buffer[8], buffer_size - 8, data, data_len);
        /* check the results of COBs encoding for validity */
        if (cobs_bacnet_frame) {
            if (cobs_len < Nmin_COBS_length_BACnet) {
                return 0;
            } else if (cobs_len > Nmax_COBS_length_BACnet) {
                return 0;
            }
        } else {
            if (cobs_len < Nmin_COBS_length) {
                return 0;
            } else if (cobs_len > Nmax_COBS_length) {
                return 0;
            }
        }
        /* for COBS, we must subtract two before use as the
           MS/TP frame length field since CRC32 is 2 bytes longer
           than CRC16 in original MSTP and non-COBS devices need
           to be able to ingest the entire frame */
        data_len = cobs_len - 2;
    } else if (data_len > 0) {
        if (!data) {
            return 0;
        }
        if ((8 + data_len + 2) > buffer_size) {
            return 0;
        }
        for (index = 8; index < (data_len + 8); index++, data++) {
            buffer[index] = *data;
            crc16 = CRC_Calc_Data(buffer[index], crc16);
        }
        crc16 = ~crc16;
        buffer[index] = crc16 & 0xFF; /* LSB first */
        buffer[index + 1] = crc16 >> 8;
    }
    buffer[0] = 0x55;
    buffer[1] = 0xFF;
    buffer[2] = frame_type;
    crc8 = CRC_Calc_Header(buffer[2], crc8);
    buffer[3] = destination;
    crc8 = CRC_Calc_Header(buffer[3], crc8);
    buffer[4] = source;
    crc8 = CRC_Calc_Header(buffer[4], crc8);
    buffer[5] = data_len >> 8; /* MSB first */
    crc8 = CRC_Calc_Header(buffer[5], crc8);
    buffer[6] = data_len & 0xFF;
    crc8 = CRC_Calc_Header(buffer[6], crc8);
    buffer[7] = ~crc8;
    index = 8;
    if (data_len > 0) {
        index += data_len + 2;
    }

    return index; /* returns the frame length */
}

/**
 * @brief Send an MS/TP Frame
 * @param mstp_port - port to send from
 * @param frame_type - type of frame to send - see defines
 * @param destination - destination address
 * @param source - source address
 * @param data - any data to be sent - may be null
 * @param data_len - number of bytes of data
 */
void MSTP_Create_And_Send_Frame(
    struct mstp_port_struct_t *mstp_port,
    uint8_t frame_type,
    uint8_t destination,
    uint8_t source,
    const uint8_t *data,
    uint16_t data_len)
{
    uint16_t len = 0; /* number of bytes to send */

    len = MSTP_Create_Frame(
        mstp_port->OutputBuffer, mstp_port->OutputBufferSize, frame_type,
        destination, source, data, data_len);

    MSTP_Send_Frame(mstp_port, &mstp_port->OutputBuffer[0], len);
    /* FIXME: be sure to reset SilenceTimer() after each octet is sent! */
}

static bool MSTP_Frame_For_Us(struct mstp_port_struct_t *mstp_port)
{
    if ((mstp_port->DestinationAddress == mstp_port->This_Station) ||
        (mstp_port->DestinationAddress == MSTP_BROADCAST_ADDRESS) ||
        (mstp_port->This_Station == MSTP_BROADCAST_ADDRESS)) {
        return true;
    }

    return false;
}

/**
 * @brief Finite State Machine for receiving an MSTP frame
 * @param mstp_port MSTP port context data
 */
void MSTP_Receive_Frame_FSM(struct mstp_port_struct_t *mstp_port)
{
    MSTP_RECEIVE_STATE receive_state = mstp_port->receive_state;

    printf_receive(
        "MSTP Rx: State=%s Data=%02X hCRC=%02X Index=%u EC=%u DateLen=%u "
        "Silence=%u\n",
        mstptext_receive_state(mstp_port->receive_state),
        mstp_port->DataRegister, mstp_port->HeaderCRC, mstp_port->Index,
        mstp_port->EventCount, mstp_port->DataLength,
        mstp_port->SilenceTimer((void *)mstp_port));
    switch (mstp_port->receive_state) {
        case MSTP_RECEIVE_STATE_IDLE:
            /* In the IDLE state, the node waits for
               the beginning of a frame. */
            /* EatAnError */
            if (mstp_port->ReceiveError == true) {
                mstp_port->ReceiveError = false;
                mstp_port->SilenceTimerReset((void *)mstp_port);
                INCREMENT_AND_LIMIT_UINT8(mstp_port->EventCount);
            } else if (mstp_port->DataAvailable == true) {
                /* wait for the start of a frame. */
                printf_receive_data("MSTP Rx: %02X ", mstp_port->DataRegister);
                if (mstp_port->DataRegister == 0x55) {
                    /* Preamble1 */
                    /* receive the remainder of the frame. */
                    mstp_port->receive_state = MSTP_RECEIVE_STATE_PREAMBLE;
                } else {
                    /* EatAnOctet */
                    printf_receive_data("\n");
                    /* wait for the start of a frame. */
                }
                mstp_port->DataAvailable = false;
                mstp_port->SilenceTimerReset((void *)mstp_port);
                INCREMENT_AND_LIMIT_UINT8(mstp_port->EventCount);
            }
            break;
        case MSTP_RECEIVE_STATE_PREAMBLE:
            /* In the PREAMBLE state, the node waits for
               the second octet of the preamble. */
            /* Timeout */
            if (mstp_port->SilenceTimer((void *)mstp_port) >
                mstp_port->Tframe_abort) {
                /* a correct preamble has not been received */
                /* wait for the start of a frame. */
                mstp_port->receive_state = MSTP_RECEIVE_STATE_IDLE;
            } else if (mstp_port->ReceiveError == true) {
                /* Error */
                mstp_port->ReceiveError = false;
                mstp_port->SilenceTimerReset((void *)mstp_port);
                INCREMENT_AND_LIMIT_UINT8(mstp_port->EventCount);
                /* wait for the start of a frame. */
                mstp_port->receive_state = MSTP_RECEIVE_STATE_IDLE;
            } else if (mstp_port->DataAvailable == true) {
                printf_receive_data("%02X ", mstp_port->DataRegister);
                if (mstp_port->DataRegister == 0xFF) {
                    /* Preamble2 */
                    mstp_port->Index = 0;
                    mstp_port->HeaderCRC = 0xFF;
                    /* receive the remainder of the frame. */
                    mstp_port->receive_state = MSTP_RECEIVE_STATE_HEADER;
                } else if (mstp_port->DataRegister == 0x55) {
                    /* ignore RepeatedPreamble1 */
                    /* wait for the second preamble octet. */
                } else {
                    /* NotPreamble */
                    /* wait for the start of a frame. */
                    mstp_port->receive_state = MSTP_RECEIVE_STATE_IDLE;
                }
                mstp_port->DataAvailable = false;
                mstp_port->SilenceTimerReset((void *)mstp_port);
                INCREMENT_AND_LIMIT_UINT8(mstp_port->EventCount);
            }
            break;
        case MSTP_RECEIVE_STATE_HEADER:
            /* In the HEADER state, the node waits for
               the fixed message header. */
            /* Timeout */
            if (mstp_port->SilenceTimer((void *)mstp_port) >
                mstp_port->Tframe_abort) {
                /* indicate that an error has occurred during the reception of a
                 * frame */
                mstp_port->ReceivedInvalidFrame = true;
                /* wait for the start of a frame. */
                mstp_port->receive_state = MSTP_RECEIVE_STATE_IDLE;
                printf_receive_error(
                    "MSTP: Rx Header: SilenceTimer %u > %d\n",
                    (unsigned)mstp_port->SilenceTimer((void *)mstp_port),
                    mstp_port->Tframe_abort);
            } else if (mstp_port->ReceiveError == true) {
                /* Error */
                mstp_port->ReceiveError = false;
                mstp_port->SilenceTimerReset((void *)mstp_port);
                INCREMENT_AND_LIMIT_UINT8(mstp_port->EventCount);
                /* indicate that an error has occurred during the reception of a
                 * frame */
                mstp_port->ReceivedInvalidFrame = true;
                printf_receive_error("MSTP: Rx Header: ReceiveError\n");
                /* wait for the start of a frame. */
                mstp_port->receive_state = MSTP_RECEIVE_STATE_IDLE;
            } else if (mstp_port->DataAvailable == true) {
                printf_receive_data("%02X ", mstp_port->DataRegister);
                if (mstp_port->Index == 0) {
                    /* FrameType */
                    mstp_port->HeaderCRC = CRC_Calc_Header(
                        mstp_port->DataRegister, mstp_port->HeaderCRC);
                    mstp_port->FrameType = mstp_port->DataRegister;
                    mstp_port->Index = 1;
                } else if (mstp_port->Index == 1) {
                    /* Destination */
                    mstp_port->HeaderCRC = CRC_Calc_Header(
                        mstp_port->DataRegister, mstp_port->HeaderCRC);
                    mstp_port->DestinationAddress = mstp_port->DataRegister;
                    mstp_port->Index = 2;
                } else if (mstp_port->Index == 2) {
                    /* Source */
                    mstp_port->HeaderCRC = CRC_Calc_Header(
                        mstp_port->DataRegister, mstp_port->HeaderCRC);
                    mstp_port->SourceAddress = mstp_port->DataRegister;
                    mstp_port->Index = 3;
                } else if (mstp_port->Index == 3) {
                    /* Length1 */
                    mstp_port->HeaderCRC = CRC_Calc_Header(
                        mstp_port->DataRegister, mstp_port->HeaderCRC);
                    mstp_port->DataLength = mstp_port->DataRegister * 256;
                    mstp_port->Index = 4;
                } else if (mstp_port->Index == 4) {
                    /* Length2 */
                    mstp_port->HeaderCRC = CRC_Calc_Header(
                        mstp_port->DataRegister, mstp_port->HeaderCRC);
                    mstp_port->DataLength += mstp_port->DataRegister;
                    mstp_port->Index = 5;
                } else if (mstp_port->Index == 5) {
                    /* HeaderCRC */
                    mstp_port->HeaderCRC = CRC_Calc_Header(
                        mstp_port->DataRegister, mstp_port->HeaderCRC);
                    mstp_port->HeaderCRCActual = mstp_port->DataRegister;
                    /* don't wait for next state - do it here */
                    if (mstp_port->HeaderCRC != 0x55) {
                        /* BadCRC */
                        /* indicate that an error has occurred during
                           the reception of a frame */
                        mstp_port->ReceivedInvalidFrame = true;
                        printf_receive_error(
                            "MSTP: Rx Header: BadCRC [%02X]\n",
                            mstp_port->DataRegister);
                        /* wait for the start of the next frame. */
                        mstp_port->receive_state = MSTP_RECEIVE_STATE_IDLE;
                    } else {
                        if (mstp_port->DataLength == 0) {
                            /* NoData */
                            if (MSTP_Frame_For_Us(mstp_port)) {
                                printf_receive_data(
                                    "%s",
                                    mstptext_frame_type(
                                        (unsigned)mstp_port->FrameType));
                                /* indicate that a frame with no data has been
                                 * received */
                                mstp_port->ReceivedValidFrame = true;
                            } else {
                                /* NotForUs */
                                mstp_port->ReceivedValidFrameNotForUs = true;
                            }
                            /* wait for the start of the next frame. */
                            mstp_port->receive_state = MSTP_RECEIVE_STATE_IDLE;
                        } else {
                            if (MSTP_Frame_For_Us(mstp_port)) {
                                if (mstp_port->DataLength <=
                                    mstp_port->InputBufferSize) {
                                    /* Data */
                                    mstp_port->receive_state =
                                        MSTP_RECEIVE_STATE_DATA;
                                } else {
                                    /* FrameTooLong */
                                    printf_receive_error(
                                        "MSTP: Rx Header: FrameTooLong %u\n",
                                        (unsigned)mstp_port->DataLength);
                                    mstp_port->receive_state =
                                        MSTP_RECEIVE_STATE_SKIP_DATA;
                                }
                            } else {
                                /* DataNotForUs */
                                mstp_port->receive_state =
                                    MSTP_RECEIVE_STATE_SKIP_DATA;
                            }
                            mstp_port->Index = 0;
                            mstp_port->DataCRC = 0xFFFF;
                        }
                    }
                } else {
                    /* not per MS/TP standard, but it is a case not covered */
                    mstp_port->ReceiveError = false;
                    /* indicate that an error has occurred during  */
                    /* the reception of a frame */
                    mstp_port->ReceivedInvalidFrame = true;
                    printf_receive_error(
                        "MSTP: Rx Data: BadIndex %u\n",
                        (unsigned)mstp_port->Index);
                    /* wait for the start of a frame. */
                    mstp_port->receive_state = MSTP_RECEIVE_STATE_IDLE;
                }
                mstp_port->SilenceTimerReset((void *)mstp_port);
                INCREMENT_AND_LIMIT_UINT8(mstp_port->EventCount);
                mstp_port->DataAvailable = false;
            }
            break;
        case MSTP_RECEIVE_STATE_DATA:
        case MSTP_RECEIVE_STATE_SKIP_DATA:
            /* In the DATA and SKIP DATA states, the node waits for the
               data portion of a frame. */
            if (mstp_port->SilenceTimer((void *)mstp_port) >
                mstp_port->Tframe_abort) {
                /* Timeout */
                /* indicate that an error has occurred during the reception of a
                 * frame */
                mstp_port->ReceivedInvalidFrame = true;
                printf_receive_error(
                    "MSTP: Rx Data: SilenceTimer %ums > %dms\n",
                    (unsigned)mstp_port->SilenceTimer((void *)mstp_port),
                    mstp_port->Tframe_abort);
                /* wait for the start of the next frame. */
                mstp_port->receive_state = MSTP_RECEIVE_STATE_IDLE;
            } else if (mstp_port->ReceiveError == true) {
                /* Error */
                mstp_port->ReceiveError = false;
                mstp_port->SilenceTimerReset((void *)mstp_port);
                /* indicate that an error has occurred during the reception of a
                 * frame */
                mstp_port->ReceivedInvalidFrame = true;
                printf_receive_error("MSTP: Rx Data: ReceiveError\n");
                /* wait for the start of the next frame. */
                mstp_port->receive_state = MSTP_RECEIVE_STATE_IDLE;
            } else if (mstp_port->DataAvailable == true) {
                printf_receive_data("%02X ", mstp_port->DataRegister);
                if (mstp_port->Index < mstp_port->DataLength) {
                    /* DataOctet */
                    mstp_port->DataCRC = CRC_Calc_Data(
                        mstp_port->DataRegister, mstp_port->DataCRC);
                    if (mstp_port->Index < mstp_port->InputBufferSize) {
                        mstp_port->InputBuffer[mstp_port->Index] =
                            mstp_port->DataRegister;
                    }
                    mstp_port->Index++;
                    /* SKIP_DATA or DATA - no change in state */
                } else if (mstp_port->Index == mstp_port->DataLength) {
                    /* CRC1 */
                    mstp_port->DataCRC = CRC_Calc_Data(
                        mstp_port->DataRegister, mstp_port->DataCRC);
                    mstp_port->DataCRCActualMSB = mstp_port->DataRegister;
                    if (mstp_port->Index < mstp_port->InputBufferSize) {
                        mstp_port->InputBuffer[mstp_port->Index] =
                            mstp_port->DataRegister;
                    }
                    mstp_port->Index++;
                    /* SKIP_DATA or DATA - no change in state */
                } else if (mstp_port->Index == (mstp_port->DataLength + 1)) {
                    /* CRC2 */
                    if (mstp_port->Index < mstp_port->InputBufferSize) {
                        mstp_port->InputBuffer[mstp_port->Index] =
                            mstp_port->DataRegister;
                    }
                    mstp_port->DataCRC = CRC_Calc_Data(
                        mstp_port->DataRegister, mstp_port->DataCRC);
                    mstp_port->DataCRCActualLSB = mstp_port->DataRegister;
                    printf_receive_data(
                        "%s",
                        mstptext_frame_type((unsigned)mstp_port->FrameType));
                    if (((mstp_port->Index + 1) < mstp_port->InputBufferSize) &&
                        (mstp_port->FrameType >= Nmin_COBS_type) &&
                        (mstp_port->FrameType <= Nmax_COBS_type)) {
                        mstp_port->DataLength = cobs_frame_decode(
                            &mstp_port->InputBuffer[mstp_port->Index + 1],
                            mstp_port->InputBufferSize, mstp_port->InputBuffer,
                            mstp_port->Index + 1);
                        if (mstp_port->DataLength > 0) {
                            /* GoodCRC */
                            if (mstp_port->receive_state ==
                                MSTP_RECEIVE_STATE_DATA) {
                                /* ForUs */
                                mstp_port->ReceivedValidFrame = true;
                            } else {
                                /* NotForUs */
                                mstp_port->ReceivedValidFrameNotForUs = true;
                            }
                        } else {
                            /* BadCRC */
                            mstp_port->ReceivedInvalidFrame = true;
                            printf_receive_error(
                                "MSTP: Rx Data: BadCRC [%02X]\n",
                                mstp_port->DataRegister);
                        }
                    } else {
                        /* STATE DATA CRC - no need for new state */
                        if (mstp_port->DataCRC == 0xF0B8) {
                            /* GoodCRC */
                            if (mstp_port->receive_state ==
                                MSTP_RECEIVE_STATE_DATA) {
                                /* ForUs */
                                mstp_port->ReceivedValidFrame = true;
                            } else {
                                /* NotForUs */
                                mstp_port->ReceivedValidFrameNotForUs = true;
                            }
                        } else {
                            /* BadCRC */
                            mstp_port->ReceivedInvalidFrame = true;
                            printf_receive_error(
                                "MSTP: Rx Data: BadCRC [%02X]\n",
                                mstp_port->DataRegister);
                        }
                    }
                    mstp_port->receive_state = MSTP_RECEIVE_STATE_IDLE;
                } else {
                    mstp_port->ReceivedInvalidFrame = true;
                    mstp_port->receive_state = MSTP_RECEIVE_STATE_IDLE;
                }
                mstp_port->DataAvailable = false;
                mstp_port->SilenceTimerReset((void *)mstp_port);
            }
            break;
        default:
            /* shouldn't get here - but if we do... */
            mstp_port->receive_state = MSTP_RECEIVE_STATE_IDLE;
            break;
    }
    if ((receive_state != MSTP_RECEIVE_STATE_IDLE) &&
        (mstp_port->receive_state == MSTP_RECEIVE_STATE_IDLE)) {
        printf_receive_data("\n");
    }
    return;
}

/**
 * @brief Finite State Machine for receiving an MSTP frame
 * @param mstp_port MSTP port context data
 * @return true if we need to transition immediately
 */
bool MSTP_Master_Node_FSM(struct mstp_port_struct_t *mstp_port)
{
    unsigned length = 0;
    uint8_t next_poll_station = 0;
    uint8_t next_this_station = 0;
    uint8_t next_next_station = 0;
    uint16_t my_timeout = 10, ns_timeout = 0, mm_timeout = 0;
    /* transition immediately to the next state */
    bool transition_now = false;
    MSTP_MASTER_STATE master_state = mstp_port->master_state;

    /* some calculations that several states need */
    next_poll_station =
        (mstp_port->Poll_Station + 1) % (mstp_port->Nmax_master + 1);
    next_this_station =
        (mstp_port->This_Station + 1) % (mstp_port->Nmax_master + 1);
    next_next_station =
        (mstp_port->Next_Station + 1) % (mstp_port->Nmax_master + 1);
    /* The zero config checks before running FSM */
    if ((mstp_port->ZeroConfigEnabled) &&
        (mstp_port->master_state != MSTP_MASTER_STATE_INITIALIZE) &&
        (mstp_port->ReceivedValidFrame == true) &&
        (mstp_port->SourceAddress == mstp_port->This_Station)) {
        /* DuplicateNode */
        mstp_port->This_Station = 255;
        mstp_port->Zero_Config_State = MSTP_ZERO_CONFIG_STATE_INIT;
        mstp_port->master_state = MSTP_MASTER_STATE_INITIALIZE;
        /* ignore the frame */
        mstp_port->ReceivedValidFrame = false;
    }
    switch (mstp_port->master_state) {
        case MSTP_MASTER_STATE_INITIALIZE:
            if (mstp_port->CheckAutoBaud) {
                MSTP_Auto_Baud_FSM(mstp_port);
            } else if (mstp_port->ZeroConfigEnabled) {
                MSTP_Zero_Config_FSM(mstp_port);
                if (mstp_port->This_Station != 255) {
                    /* indicate that the next station is unknown */
                    mstp_port->Next_Station = mstp_port->This_Station;
                    /* Send a Poll For Master since we just received
                       the token */
                    mstp_port->Poll_Station = (mstp_port->Next_Station + 1) %
                        (mstp_port->Zero_Config_Max_Master + 1);
                    mstp_port->TokenCount = Npoll;
                    mstp_port->RetryCount = 0;
                    mstp_port->EventCount = 0;
                    mstp_port->SoleMaster = true;
                    MSTP_Create_And_Send_Frame(
                        mstp_port, FRAME_TYPE_POLL_FOR_MASTER,
                        mstp_port->Poll_Station, mstp_port->This_Station, NULL,
                        0);
                    mstp_port->master_state = MSTP_MASTER_STATE_POLL_FOR_MASTER;
                    transition_now = true;
                }
            } else {
                /* DoneInitializing */
                /* indicate that the next station is unknown */
                mstp_port->Next_Station = mstp_port->This_Station;
                mstp_port->Poll_Station = mstp_port->This_Station;
                /* cause a Poll For Master to be sent when this node first */
                /* receives the token */
                mstp_port->TokenCount = Npoll;
                mstp_port->SoleMaster = false;
                mstp_port->master_state = MSTP_MASTER_STATE_IDLE;
                transition_now = true;
            }
            break;
        case MSTP_MASTER_STATE_IDLE:
            /* In the IDLE state, the node waits for a frame. */
            if (mstp_port->ReceivedInvalidFrame == true) {
                /* ReceivedInvalidFrame */
                /* invalid frame was received */
                /* wait for the next frame - remain in IDLE */
                mstp_port->ReceivedInvalidFrame = false;
            } else if (mstp_port->ReceivedValidFrameNotForUs == true) {
                /* ReceivedValidFrameNotForUs */
                /* valid frame was received, but not for this node */
                /* wait for the next frame - remain in IDLE */
                mstp_port->ReceivedValidFrameNotForUs = false;
            } else if (mstp_port->ReceivedValidFrame == true) {
                printf_master(
                    "MSTP: ReceivedValidFrame "
                    "Src=%02X Dest=%02X DataLen=%u "
                    "FC=%u ST=%u Type=%s\n",
                    mstp_port->SourceAddress, mstp_port->DestinationAddress,
                    mstp_port->DataLength, mstp_port->FrameCount,
                    mstp_port->SilenceTimer((void *)mstp_port),
                    mstptext_frame_type((unsigned)mstp_port->FrameType));
                /* destined for me! */
                switch (mstp_port->FrameType) {
                    case FRAME_TYPE_TOKEN:
                        /* ReceivedToken */
                        /* tokens cannot be broadcast */
                        if (mstp_port->DestinationAddress ==
                            MSTP_BROADCAST_ADDRESS) {
                            break;
                        }
                        mstp_port->ReceivedValidFrame = false;
                        mstp_port->FrameCount = 0;
                        mstp_port->SoleMaster = false;
                        mstp_port->master_state = MSTP_MASTER_STATE_USE_TOKEN;
                        transition_now = true;
                        break;
                    case FRAME_TYPE_POLL_FOR_MASTER:
                        /* ReceivedPFM */
                        /* DestinationAddress is equal to TS */
                        if (mstp_port->DestinationAddress ==
                            mstp_port->This_Station) {
                            MSTP_Create_And_Send_Frame(
                                mstp_port, FRAME_TYPE_REPLY_TO_POLL_FOR_MASTER,
                                mstp_port->SourceAddress,
                                mstp_port->This_Station, NULL, 0);
                        }
                        break;
                    case FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY:
                    case FRAME_TYPE_BACNET_EXTENDED_DATA_NOT_EXPECTING_REPLY:
                        if ((mstp_port->DestinationAddress ==
                             MSTP_BROADCAST_ADDRESS) &&
                            (npdu_confirmed_service(
                                mstp_port->InputBuffer,
                                mstp_port->DataLength))) {
                            /* BTL test: verifies that the IUT will
                            quietly discard any Confirmed-Request-PDU,
                            whose destination address is a multicast or
                            broadcast address, received from the
                            network layer. */
                        } else {
                            /* ForUs */
                            /* indicate successful reception
                                to the higher layers */
                            (void)MSTP_Put_Receive(mstp_port);
                        }
                        break;
                    case FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY:
                    case FRAME_TYPE_BACNET_EXTENDED_DATA_EXPECTING_REPLY:
                        if (mstp_port->DestinationAddress ==
                            MSTP_BROADCAST_ADDRESS) {
                            /* broadcast DER just remains IDLE */
                        } else {
                            /* indicate successful reception to the higher
                             * layers  */
                            (void)MSTP_Put_Receive(mstp_port);
                            mstp_port->master_state =
                                MSTP_MASTER_STATE_ANSWER_DATA_REQUEST;
                        }
                        break;
                    case FRAME_TYPE_TEST_REQUEST:
                        MSTP_Create_And_Send_Frame(
                            mstp_port, FRAME_TYPE_TEST_RESPONSE,
                            mstp_port->SourceAddress, mstp_port->This_Station,
                            mstp_port->InputBuffer, mstp_port->DataLength);
                        break;
                    case FRAME_TYPE_TEST_RESPONSE:
                    default:
                        break;
                }
                /* For DATA_EXPECTING_REPLY, we will keep the Rx Frame for
                   reference, and the flag will be cleared in the next state */
                if (mstp_port->master_state !=
                    MSTP_MASTER_STATE_ANSWER_DATA_REQUEST) {
                    mstp_port->ReceivedValidFrame = false;
                }
            } else if (
                mstp_port->SilenceTimer((void *)mstp_port) >= Tno_token) {
                /* LostToken */
                /* assume that the token has been lost */
                mstp_port->EventCount = 0; /* Addendum 135-2004d-8 */
                mstp_port->master_state = MSTP_MASTER_STATE_NO_TOKEN;
                /* set the receive frame flags to false in case we received
                   some bytes and had a timeout for some reason */
                mstp_port->ReceivedInvalidFrame = false;
                mstp_port->ReceivedValidFrameNotForUs = false;
                mstp_port->ReceivedValidFrame = false;
                transition_now = true;
            }
            break;
        case MSTP_MASTER_STATE_USE_TOKEN:
            /* In the USE_TOKEN state, the node is allowed to send one or  */
            /* more data frames. These may be BACnet Data frames or */
            /* proprietary frames. */
            /* FIXME: We could wait for up to Tusage_delay */
            length = (unsigned)MSTP_Get_Send(mstp_port, 0);
            if (length < 1) {
                /* NothingToSend */
                mstp_port->FrameCount = mstp_port->Nmax_info_frames;
                mstp_port->master_state = MSTP_MASTER_STATE_DONE_WITH_TOKEN;
                transition_now = true;
            } else {
                uint8_t frame_type = mstp_port->OutputBuffer[2];
                uint8_t destination = mstp_port->OutputBuffer[3];
                MSTP_Send_Frame(
                    mstp_port, &mstp_port->OutputBuffer[0], (uint16_t)length);
                mstp_port->FrameCount++;
                switch (frame_type) {
                    case FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY:
                        if (destination == MSTP_BROADCAST_ADDRESS) {
                            /* SendNoWait */
                            mstp_port->master_state =
                                MSTP_MASTER_STATE_DONE_WITH_TOKEN;
                        } else {
                            /* SendAndWait */
                            mstp_port->master_state =
                                MSTP_MASTER_STATE_WAIT_FOR_REPLY;
                        }
                        break;
                    case FRAME_TYPE_TEST_REQUEST:
                        /* SendAndWait */
                        mstp_port->master_state =
                            MSTP_MASTER_STATE_WAIT_FOR_REPLY;
                        break;
                    default:
                        /* SendNoWait */
                        mstp_port->master_state =
                            MSTP_MASTER_STATE_DONE_WITH_TOKEN;
                        break;
                }
            }
            break;
        case MSTP_MASTER_STATE_WAIT_FOR_REPLY:
            /* In the WAIT_FOR_REPLY state, the node waits for  */
            /* a reply from another node. */
            if (mstp_port->SilenceTimer((void *)mstp_port) >=
                mstp_port->Treply_timeout) {
                /* ReplyTimeout */
                /* assume that the request has failed */
                mstp_port->FrameCount = mstp_port->Nmax_info_frames;
                mstp_port->master_state = MSTP_MASTER_STATE_DONE_WITH_TOKEN;
                /* Any retry of the data frame shall await the next entry */
                /* to the USE_TOKEN state. (Because of the length of the
                 * timeout,  */
                /* this transition will cause the token to be passed regardless
                 */
                /* of the initial value of FrameCount.) */
                transition_now = true;
            } else {
                if ((mstp_port->ReceivedInvalidFrame == true) ||
                    (mstp_port->ReceivedValidFrameNotForUs == true)) {
                    /* InvalidFrame in this state */
                    mstp_port->ReceivedInvalidFrame = false;
                    mstp_port->ReceivedValidFrameNotForUs = false;
                    mstp_port->master_state = MSTP_MASTER_STATE_DONE_WITH_TOKEN;
                    transition_now = true;
                } else if (mstp_port->ReceivedValidFrame == true) {
                    if (mstp_port->DestinationAddress ==
                        mstp_port->This_Station) {
                        switch (mstp_port->FrameType) {
                            case FRAME_TYPE_REPLY_POSTPONED:
                                /* ReceivedReplyPostponed */
                                mstp_port->master_state =
                                    MSTP_MASTER_STATE_DONE_WITH_TOKEN;
                                break;
                            case FRAME_TYPE_TEST_RESPONSE:
                                mstp_port->master_state =
                                    MSTP_MASTER_STATE_DONE_WITH_TOKEN;
                                break;
                            case FRAME_TYPE_TOKEN:
                            case FRAME_TYPE_POLL_FOR_MASTER:
                            case FRAME_TYPE_REPLY_TO_POLL_FOR_MASTER:
                            case FRAME_TYPE_TEST_REQUEST:
                                /* ReceivedUnexpectedFrame */
                                /* FrameType has a value other than a FrameType
                                   known to this node that indicates a reply */
                                mstp_port->master_state = MSTP_MASTER_STATE_IDLE;
                                break;
                            default:
                                /* ReceivedReply */
                                /* FrameType known to this node that
                                   indicates a reply */
                                /* indicate successful reception
                                   to the higher layers */
                                (void)MSTP_Put_Receive(mstp_port);
                                mstp_port->master_state =
                                    MSTP_MASTER_STATE_DONE_WITH_TOKEN;
                                break;
                        }
                    } else {
                        /* ReceivedUnexpectedFrame */
                        /* an unexpected frame was received */
                        /* This may indicate the presence of multiple tokens. */
                        /* Synchronize with the network. */
                        /* This action drops the token. */
                        mstp_port->master_state = MSTP_MASTER_STATE_IDLE;
                    }
                    mstp_port->ReceivedValidFrame = false;
                    transition_now = true;
                }
            }
            break;
        case MSTP_MASTER_STATE_DONE_WITH_TOKEN:
            /* The DONE_WITH_TOKEN state either sends another data frame,  */
            /* passes the token, or initiates a Poll For Master cycle. */
            /* SendAnotherFrame */
            if (mstp_port->FrameCount < mstp_port->Nmax_info_frames) {
                /* then this node may send another information frame  */
                /* before passing the token.  */
                mstp_port->master_state = MSTP_MASTER_STATE_USE_TOKEN;
                transition_now = true;
            } else if (
                (mstp_port->SoleMaster == false) &&
                (mstp_port->Next_Station == mstp_port->This_Station)) {
                /* NextStationUnknown - added in Addendum 135-2008v-1 */
                /*  then the next station to which the token
                   should be sent is unknown - so PollForMaster */
                mstp_port->Poll_Station = next_this_station;
                MSTP_Create_And_Send_Frame(
                    mstp_port, FRAME_TYPE_POLL_FOR_MASTER,
                    mstp_port->Poll_Station, mstp_port->This_Station, NULL, 0);
                mstp_port->RetryCount = 0;
                mstp_port->master_state = MSTP_MASTER_STATE_POLL_FOR_MASTER;
            } else if (mstp_port->TokenCount < (Npoll - 1)) {
                /* Npoll changed in Errata SSPC-135-2004 */
                if ((mstp_port->SoleMaster == true) &&
                    (mstp_port->Next_Station != next_this_station)) {
                    /* SoleMaster */
                    /* there are no other known master nodes to */
                    /* which the token may be sent (true master-slave
                     * operation).  */
                    mstp_port->FrameCount = 0;
                    mstp_port->TokenCount++;
                    mstp_port->master_state = MSTP_MASTER_STATE_USE_TOKEN;
                    transition_now = true;
                } else {
                    /* SendToken */
                    /* Npoll changed in Errata SSPC-135-2004 */
                    /* The comparison of NS and TS+1 eliminates the Poll For
                     * Master  */
                    /* if there are no addresses between TS and NS, since there
                     * is no  */
                    /* address at which a new master node may be found in that
                     * case. */
                    mstp_port->TokenCount++;
                    /* transmit a Token frame to NS */
                    MSTP_Create_And_Send_Frame(
                        mstp_port, FRAME_TYPE_TOKEN, mstp_port->Next_Station,
                        mstp_port->This_Station, NULL, 0);
                    mstp_port->RetryCount = 0;
                    mstp_port->EventCount = 0;
                    mstp_port->master_state = MSTP_MASTER_STATE_PASS_TOKEN;
                }
            } else if (next_poll_station == mstp_port->Next_Station) {
                if (mstp_port->SoleMaster == true) {
                    /* SoleMasterRestartMaintenancePFM */
                    mstp_port->Poll_Station = next_next_station;
                    MSTP_Create_And_Send_Frame(
                        mstp_port, FRAME_TYPE_POLL_FOR_MASTER,
                        mstp_port->Poll_Station, mstp_port->This_Station, NULL,
                        0);
                    /* no known successor node */
                    mstp_port->Next_Station = mstp_port->This_Station;
                    mstp_port->RetryCount = 0;
                    /* changed in Errata SSPC-135-2004 */
                    mstp_port->TokenCount = 1;
                    /* mstp_port->EventCount = 0; removed in Addendum
                     * 135-2004d-8 */
                    /* find a new successor to TS */
                    mstp_port->master_state = MSTP_MASTER_STATE_POLL_FOR_MASTER;
                } else {
                    /* ResetMaintenancePFM */
                    mstp_port->Poll_Station = mstp_port->This_Station;
                    /* transmit a Token frame to NS */
                    MSTP_Create_And_Send_Frame(
                        mstp_port, FRAME_TYPE_TOKEN, mstp_port->Next_Station,
                        mstp_port->This_Station, NULL, 0);
                    mstp_port->RetryCount = 0;
                    /* changed in Errata SSPC-135-2004 */
                    mstp_port->TokenCount = 1;
                    mstp_port->EventCount = 0;
                    mstp_port->master_state = MSTP_MASTER_STATE_PASS_TOKEN;
                }
            } else {
                /* SendMaintenancePFM */
                mstp_port->Poll_Station = next_poll_station;
                MSTP_Create_And_Send_Frame(
                    mstp_port, FRAME_TYPE_POLL_FOR_MASTER,
                    mstp_port->Poll_Station, mstp_port->This_Station, NULL, 0);
                mstp_port->RetryCount = 0;
                mstp_port->master_state = MSTP_MASTER_STATE_POLL_FOR_MASTER;
            }
            break;
        case MSTP_MASTER_STATE_PASS_TOKEN:
            /* The PASS_TOKEN state listens for a successor to begin using */
            /* the token that this node has just attempted to pass. */
            if (mstp_port->SilenceTimer((void *)mstp_port) <=
                mstp_port->Tusage_timeout) {
                if (mstp_port->EventCount > Nmin_octets) {
                    /* SawTokenUser */
                    /* Assume that a frame has been sent by the new token user.
                     */
                    /* Enter the IDLE state to process the frame. */
                    mstp_port->master_state = MSTP_MASTER_STATE_IDLE;
                    transition_now = true;
                }
            } else {
                if (mstp_port->RetryCount < Nretry_token) {
                    /* RetrySendToken */
                    mstp_port->RetryCount++;
                    /* Transmit a Token frame to NS */
                    MSTP_Create_And_Send_Frame(
                        mstp_port, FRAME_TYPE_TOKEN, mstp_port->Next_Station,
                        mstp_port->This_Station, NULL, 0);
                    mstp_port->EventCount = 0;
                    /* re-enter the current state to listen for NS  */
                    /* to begin using the token. */
                } else {
                    /* FindNewSuccessor */
                    /* Assume that NS has failed.  */
                    /* note: if NS=TS-1, this node could send PFM to self! */
                    mstp_port->Poll_Station = next_next_station;
                    /* Transmit a Poll For Master frame to PS. */
                    MSTP_Create_And_Send_Frame(
                        mstp_port, FRAME_TYPE_POLL_FOR_MASTER,
                        mstp_port->Poll_Station, mstp_port->This_Station, NULL,
                        0);
                    /* no known successor node */
                    mstp_port->Next_Station = mstp_port->This_Station;
                    mstp_port->RetryCount = 0;
                    mstp_port->TokenCount = 0;
                    /* mstp_port->EventCount = 0; removed in Addendum
                     * 135-2004d-8 */
                    /* find a new successor to TS */
                    mstp_port->master_state = MSTP_MASTER_STATE_POLL_FOR_MASTER;
                }
            }
            break;
        case MSTP_MASTER_STATE_NO_TOKEN:
            /* The NO_TOKEN state is entered if mstp_port->SilenceTimer()
             * becomes greater  */
            /* than Tno_token, indicating that there has been no network
             * activity */
            /* for that period of time. The timeout is continued to determine */
            /* whether or not this node may create a token. */
            my_timeout = Tno_token + (Tslot * mstp_port->This_Station);
            if (mstp_port->SilenceTimer((void *)mstp_port) < my_timeout) {
                if (mstp_port->EventCount > Nmin_octets) {
                    /* SawFrame */
                    /* Some other node exists at a lower address.  */
                    /* Enter the IDLE state to receive and process the incoming
                     * frame. */
                    mstp_port->master_state = MSTP_MASTER_STATE_IDLE;
                    transition_now = true;
                }
            } else {
                ns_timeout =
                    Tno_token + (Tslot * (mstp_port->This_Station + 1));
                mm_timeout = Tno_token + (Tslot * (mstp_port->Nmax_master + 1));
                if ((mstp_port->SilenceTimer((void *)mstp_port) < ns_timeout) ||
                    (mstp_port->SilenceTimer((void *)mstp_port) > mm_timeout)) {
                    /* GenerateToken */
                    /* Assume that this node is the lowest numerical address  */
                    /* on the network and is empowered to create a token.  */
                    mstp_port->Poll_Station = next_this_station;
                    /* Transmit a Poll For Master frame to PS. */
                    MSTP_Create_And_Send_Frame(
                        mstp_port, FRAME_TYPE_POLL_FOR_MASTER,
                        mstp_port->Poll_Station, mstp_port->This_Station, NULL,
                        0);
                    /* indicate that the next station is unknown */
                    mstp_port->Next_Station = mstp_port->This_Station;
                    mstp_port->RetryCount = 0;
                    mstp_port->TokenCount = 0;
                    /* mstp_port->EventCount = 0;
                       removed Addendum 135-2004d-8 */
                    /* enter the POLL_FOR_MASTER state
                       to find a new successor to TS. */
                    mstp_port->master_state = MSTP_MASTER_STATE_POLL_FOR_MASTER;
                } else {
                    /* We missed our time slot!
                       We should never get here unless
                       OS timer resolution is poor or we were busy */
                    if (mstp_port->EventCount > Nmin_octets) {
                        /* SawFrame */
                        /* Some other node exists at a lower address.  */
                        /* Enter the IDLE state to receive and
                           process the incoming frame. */
                        mstp_port->master_state = MSTP_MASTER_STATE_IDLE;
                        transition_now = true;
                    }
                }
            }
            break;
        case MSTP_MASTER_STATE_POLL_FOR_MASTER:
            /* In the POLL_FOR_MASTER state, the node listens for a reply to */
            /* a previously sent Poll For Master frame in order to find  */
            /* a successor node. */
            if (mstp_port->ReceivedValidFrame == true) {
                if ((mstp_port->DestinationAddress ==
                     mstp_port->This_Station) &&
                    (mstp_port->FrameType ==
                     FRAME_TYPE_REPLY_TO_POLL_FOR_MASTER)) {
                    /* ReceivedReplyToPFM */
                    mstp_port->SoleMaster = false;
                    mstp_port->Next_Station = mstp_port->SourceAddress;
                    mstp_port->EventCount = 0;
                    /* Transmit a Token frame to NS */
                    MSTP_Create_And_Send_Frame(
                        mstp_port, FRAME_TYPE_TOKEN, mstp_port->Next_Station,
                        mstp_port->This_Station, NULL, 0);
                    mstp_port->Poll_Station = mstp_port->This_Station;
                    mstp_port->TokenCount = 0;
                    mstp_port->RetryCount = 0;
                    mstp_port->master_state = MSTP_MASTER_STATE_PASS_TOKEN;
                } else {
                    /* ReceivedUnexpectedFrame */
                    /* An unexpected frame was received.  */
                    /* This may indicate the presence of multiple tokens. */
                    /* enter the IDLE state to synchronize with the network.  */
                    /* This action drops the token. */
                    mstp_port->master_state = MSTP_MASTER_STATE_IDLE;
                    transition_now = true;
                }
                mstp_port->ReceivedValidFrame = false;
            } else if (
                (mstp_port->SilenceTimer((void *)mstp_port) >
                 mstp_port->Tusage_timeout) ||
                (mstp_port->ReceivedInvalidFrame == true) ||
                (mstp_port->ReceivedValidFrameNotForUs == true)) {
                if (mstp_port->SoleMaster == true) {
                    /* SoleMaster */
                    /* There was no valid reply to the periodic poll  */
                    /* by the sole known master for other masters. */
                    mstp_port->FrameCount = 0;
                    /* mstp_port->TokenCount++; removed in 2004 */
                    mstp_port->master_state = MSTP_MASTER_STATE_USE_TOKEN;
                    transition_now = true;
                } else {
                    if (mstp_port->Next_Station != mstp_port->This_Station) {
                        /* DoneWithPFM */
                        /* There was no valid reply to the maintenance  */
                        /* poll for a master at address PS.  */
                        mstp_port->EventCount = 0;
                        /* transmit a Token frame to NS */
                        MSTP_Create_And_Send_Frame(
                            mstp_port, FRAME_TYPE_TOKEN,
                            mstp_port->Next_Station, mstp_port->This_Station,
                            NULL, 0);
                        mstp_port->RetryCount = 0;
                        mstp_port->master_state = MSTP_MASTER_STATE_PASS_TOKEN;
                    } else {
                        if (next_poll_station != mstp_port->This_Station) {
                            /* SendNextPFM */
                            mstp_port->Poll_Station = next_poll_station;
                            /* Transmit a Poll For Master frame to PS. */
                            MSTP_Create_And_Send_Frame(
                                mstp_port, FRAME_TYPE_POLL_FOR_MASTER,
                                mstp_port->Poll_Station,
                                mstp_port->This_Station, NULL, 0);
                            mstp_port->RetryCount = 0;
                            /* Re-enter the current state. */
                        } else {
                            /* DeclareSoleMaster */
                            /* to indicate that this station is the only master
                             */
                            mstp_port->SoleMaster = true;
                            mstp_port->FrameCount = 0;
                            mstp_port->master_state =
                                MSTP_MASTER_STATE_USE_TOKEN;
                            transition_now = true;
                        }
                    }
                }
                mstp_port->ReceivedInvalidFrame = false;
                mstp_port->ReceivedValidFrameNotForUs = false;
            }
            break;
        case MSTP_MASTER_STATE_ANSWER_DATA_REQUEST:
            /* The ANSWER_DATA_REQUEST state is entered when a  */
            /* BACnet Data Expecting Reply, a Test_Request, or  */
            /* a proprietary frame that expects a reply is received. */
            /* FIXME: MSTP_Get_Reply waits for a matching reply, but
               if the next queued message doesn't match, then we
               sit here for Treply_delay doing nothing */
            length = (unsigned)MSTP_Get_Reply(mstp_port, 0);
            if (length > 0) {
                /* Reply */
                /* If a reply is available from the higher layers  */
                /* within Treply_delay after the reception of the  */
                /* final octet of the requesting frame  */
                /* (the mechanism used to determine this is a local matter), */
                /* then call MSTP_Create_And_Send_Frame to transmit the reply
                 * frame  */
                /* and enter the IDLE state to wait for the next frame. */
                MSTP_Send_Frame(
                    mstp_port, &mstp_port->OutputBuffer[0], (uint16_t)length);
                mstp_port->master_state = MSTP_MASTER_STATE_IDLE;
                /* clear our flag we were holding for comparison */
                mstp_port->ReceivedValidFrame = false;
            } else if (
                mstp_port->SilenceTimer((void *)mstp_port) >
                mstp_port->Treply_delay) {
                /* DeferredReply */
                /* If no reply will be available from the higher layers */
                /* within Treply_delay after the reception of the */
                /* final octet of the requesting frame (the mechanism */
                /* used to determine this is a local matter), */
                /* then an immediate reply is not possible. */
                /* Any reply shall wait until this node receives the token. */
                /* Call MSTP_Create_And_Send_Frame to transmit a Reply Postponed
                 * frame, */
                /* and enter the IDLE state. */
                MSTP_Create_And_Send_Frame(
                    mstp_port, FRAME_TYPE_REPLY_POSTPONED,
                    mstp_port->SourceAddress, mstp_port->This_Station, NULL, 0);
                mstp_port->master_state = MSTP_MASTER_STATE_IDLE;
                /* clear our flag we were holding for comparison */
                mstp_port->ReceivedValidFrame = false;
            }
            break;
        default:
            mstp_port->master_state = MSTP_MASTER_STATE_IDLE;
            break;
    }
    if (mstp_port->master_state != master_state) {
        /* change of state detected - so print the details for debugging */
        printf_master(
            "MSTP: TS=%02X[%02X] NS=%02X[%02X] PS=%02X[%02X] EC=%u TC=%u ST=%u "
            "%s\n",
            mstp_port->This_Station, next_this_station, mstp_port->Next_Station,
            next_next_station, mstp_port->Poll_Station, next_poll_station,
            mstp_port->EventCount, mstp_port->TokenCount,
            mstp_port->SilenceTimer((void *)mstp_port),
            mstptext_master_state(mstp_port->master_state));
    }

    return transition_now;
}

/**
 * @brief Finite State Machine for the Slave Node process
 * @param mstp_port the context of the MSTP port
 */
void MSTP_Slave_Node_FSM(struct mstp_port_struct_t *mstp_port)
{
    unsigned length = 0;

    mstp_port->master_state = MSTP_MASTER_STATE_IDLE;
    if (mstp_port->ReceivedInvalidFrame == true) {
        /* ReceivedInvalidFrame */
        /* invalid frame was received */
        mstp_port->ReceivedInvalidFrame = false;
    } else if (mstp_port->ReceivedValidFrameNotForUs) {
        /* ReceivedValidFrameNotForUs */
        /* valid frame was received, but not for this node */
        mstp_port->ReceivedValidFrameNotForUs = false;
    } else if (mstp_port->ReceivedValidFrame) {
        mstp_port->ReceivedValidFrame = false;
        switch (mstp_port->FrameType) {
            case FRAME_TYPE_BACNET_DATA_EXPECTING_REPLY:
            case FRAME_TYPE_BACNET_EXTENDED_DATA_EXPECTING_REPLY:
                if (mstp_port->DestinationAddress != MSTP_BROADCAST_ADDRESS) {
                    /* indicate successful reception to the higher layers  */
                    (void)MSTP_Put_Receive(mstp_port);
                }
                break;
            case FRAME_TYPE_BACNET_DATA_NOT_EXPECTING_REPLY:
            case FRAME_TYPE_BACNET_EXTENDED_DATA_NOT_EXPECTING_REPLY:
                if ((mstp_port->DestinationAddress == MSTP_BROADCAST_ADDRESS) &&
                    (npdu_confirmed_service(
                        mstp_port->InputBuffer, mstp_port->DataLength))) {
                    /* quietly discard any Confirmed-Request-PDU,
                       whose destination address is a multicast or
                       broadcast address, received from the
                       network layer. */
                } else {
                    /* ForUs */
                    /* indicate successful reception
                       to the higher layers */
                    (void)MSTP_Put_Receive(mstp_port);
                }
                break;
            case FRAME_TYPE_TEST_REQUEST:
                MSTP_Create_And_Send_Frame(
                    mstp_port, FRAME_TYPE_TEST_RESPONSE,
                    mstp_port->SourceAddress, mstp_port->This_Station,
                    &mstp_port->InputBuffer[0], mstp_port->DataLength);
                break;
            case FRAME_TYPE_TOKEN:
            case FRAME_TYPE_POLL_FOR_MASTER:
            case FRAME_TYPE_TEST_RESPONSE:
            default:
                break;
        }
    } else {
        /* The ANSWER_DATA_REQUEST state is entered when a  */
        /* BACnet Data Expecting Reply, a Test_Request, or  */
        /* a proprietary frame that expects a reply is received. */
        length = (unsigned)MSTP_Get_Reply(mstp_port, 0);
        if (length > 0) {
            /* Reply */
            /* If a reply is available from the higher layers  */
            /* within Treply_delay after the reception of the  */
            /* final octet of the requesting frame  */
            /* (the mechanism used to determine this is a local
             * matter), */
            /* then call MSTP_Create_And_Send_Frame to transmit the
             * reply frame  */
            /* and enter the IDLE state to wait for the next frame.
             */
            MSTP_Send_Frame(
                mstp_port, &mstp_port->OutputBuffer[0], (uint16_t)length);
            /* clear our flag we were holding for comparison */
            mstp_port->ReceivedValidFrame = false;
        } else if (
            mstp_port->SilenceTimer((void *)mstp_port) >
            mstp_port->Treply_delay) {
            /* If no reply will be available from the higher layers
                within Treply_delay after the reception of the final
                octet of the requesting frame (the mechanism used to
                determine this is a local matter), then no reply is
                possible. */
            /* clear our flag we were holding for comparison */
            mstp_port->ReceivedValidFrame = false;
        }
    }
}

/**
 * @brief Initialize a UUID for storing the unique identifier of this node
 *  which is used to send and validate a test request and test response
 * @note A Universally Unique IDentifier (UUID) - also called a
 * Global Unique IDentifier (GUID) - is a 128-bit value.
 *
 * 4.4.  Algorithms for Creating a UUID from Truly Random or
 *      Pseudo-Random Numbers
 *
 *   The version 4 UUID is meant for generating UUIDs from truly-random or
 *   pseudo-random numbers.
 *
 *   The algorithm is as follows:
 *
 *   o  Set the two most significant bits (bits 6 and 7) of the
 *      clock_seq_hi_and_reserved to zero and one, respectively.
 *
 *   o  Set the four most significant bits (bits 12 through 15) of the
 *      time_hi_and_version field to the 4-bit version number from
 *      Section 4.1.3.
 *
 *   o  Set all the other bits to randomly (or pseudo-randomly) chosen
 *      values.
 *
 * @param mstp_port the context of the MSTP port
 */
void MSTP_Zero_Config_UUID_Init(struct mstp_port_struct_t *mstp_port)
{
    unsigned i = 0;

    if (!mstp_port) {
        return;
    }
    /* 1. Generate 16 random bytes = 128 bits */
    for (i = 0; i < MSTP_UUID_SIZE; i++) {
        mstp_port->UUID[i] = rand() % 256;
    }
    /* 2. Adjust certain bits according to RFC 4122 section 4.4.
       This just means do the following
       (a) set the high nibble of the 7th byte equal to 4 and
       (b) set the two most significant bits of the 9th byte to 10'B,
       so the high nibble will be one of {8,9,A,B}.
       From http://www.cryptosys.net/pki/Uuid.c.html */
    mstp_port->UUID[6] = 0x40 | (mstp_port->UUID[6] & 0x0f);
    mstp_port->UUID[8] = 0x80 | (mstp_port->UUID[8] & 0x3f);
}

/**
 * @brief Increment the Zero Configuration Station address
 * @param station the current station address in the range of min..max
 * @return the next station address
 */
unsigned MSTP_Zero_Config_Station_Increment(unsigned station)
{
    unsigned next_station;

    if (station < Nmin_poll_station) {
        next_station = Nmin_poll_station;
    } else {
#ifdef MSTP_ZERO_CONFIG_STATION_INCREMENT_MODULO
        /* as defined by specification language */
        next_station = Nmin_poll_station +
            ((station + 1) % ((Nmax_poll_station - Nmin_poll_station) + 1));
#else
        next_station = station + 1;
        if (next_station > Nmax_poll_station) {
            next_station = Nmin_poll_station;
        }
#endif
    }

    return next_station;
}

/**
 * @brief The ZERO_CONFIGURATION_INIT state is entered when
 *  ZeroConfigurationMode is TRUE
 * @param mstp_port the context of the MSTP port
 */
static void MSTP_Zero_Config_State_Init(struct mstp_port_struct_t *mstp_port)
{
    uint32_t slots;

    if (!mstp_port) {
        return;
    }
    mstp_port->Poll_Count = 0;
    /* initialize the zero config station address */
    if ((mstp_port->Zero_Config_Preferred_Station < Nmin_poll_station) ||
        (mstp_port->Zero_Config_Preferred_Station > Nmax_poll_station)) {
        mstp_port->Zero_Config_Preferred_Station = Nmin_poll_station;
    }
    mstp_port->Zero_Config_Station = mstp_port->Zero_Config_Preferred_Station;
    mstp_port->Npoll_slot = 1 + (mstp_port->UUID[0] % Nmax_poll_slot);
    /* basic silence timeout is the dropped token time plus
        one Tslot after the last master node. Add one Tslot of
        silence timeout per zero config priority slot */
    slots = 128 + mstp_port->Npoll_slot;
    mstp_port->Zero_Config_Silence = Tno_token + Tslot * slots;
    mstp_port->Zero_Config_Max_Master = 0;
    mstp_port->Zero_Config_State = MSTP_ZERO_CONFIG_STATE_IDLE;
}

/**
 * @brief The ZERO_CONFIGURATION_IDLE state is entered when
 *  ZeroConfigurationMode is TRUE, and a node is
 *  is waiting for any frame or waiting to timeout.
 * @param mstp_port the context of the MSTP port
 */
static void MSTP_Zero_Config_State_Idle(struct mstp_port_struct_t *mstp_port)
{
    if (!mstp_port) {
        return;
    }
    if (mstp_port->ReceivedValidFrame) {
        /* IdleValidFrame */
        /* next state will clear the frame flags */
        mstp_port->Poll_Count = 0;
        mstp_port->Zero_Config_State = MSTP_ZERO_CONFIG_STATE_LURK;
    } else if (mstp_port->ReceivedInvalidFrame) {
        /* IdleInvalidFrame */
        mstp_port->ReceivedInvalidFrame = false;
    } else if (mstp_port->ReceivedValidFrameNotForUs) {
        /* IdleValidFrameNotForUs */
        mstp_port->ReceivedValidFrameNotForUs = false;
    } else if (mstp_port->Zero_Config_Silence > 0) {
        if (mstp_port->SilenceTimer((void *)mstp_port) >
            mstp_port->Zero_Config_Silence) {
            /* IdleTimeout */
            /* long silence indicates we are alone or
            with other silent devices */
            /* claim the token at the current zero-config address */
            /* configure max master at maximum */
            /* confirm this station with a quick test */
            mstp_port->Zero_Config_Max_Master = DEFAULT_MAX_MASTER;
            MSTP_Create_And_Send_Frame(
                mstp_port, FRAME_TYPE_TEST_REQUEST,
                mstp_port->Zero_Config_Station, mstp_port->Zero_Config_Station,
                mstp_port->UUID, sizeof(mstp_port->UUID));
            mstp_port->Zero_Config_State = MSTP_ZERO_CONFIG_STATE_CONFIRM;
        }
    }
}

/**
 * @brief The ZERO_CONFIGURATION_LURK state is entered when
 *  ZeroConfigurationMode is TRUE, and a node is
 *  counting a Poll For Master frames to Zero_Config_Station address
 * @param mstp_port the context of the MSTP port
 */
static void MSTP_Zero_Config_State_Lurk(struct mstp_port_struct_t *mstp_port)
{
    uint8_t count, frame, src, dst;

    if (!mstp_port) {
        return;
    }
    if (mstp_port->ReceivedValidFrame) {
        mstp_port->ReceivedValidFrame = false;
        dst = mstp_port->DestinationAddress;
        src = mstp_port->SourceAddress;
        frame = mstp_port->FrameType;
        if (frame == FRAME_TYPE_POLL_FOR_MASTER) {
            if ((dst > mstp_port->Zero_Config_Max_Master) &&
                (dst <= DEFAULT_MAX_MASTER)) {
                /* LearnMaxMaster */
                mstp_port->Zero_Config_Max_Master = dst;
            }
        }
        if (src == mstp_port->Zero_Config_Station) {
            /* LurkAddressInUse */
            /* monitor PFM from the next address */
            mstp_port->Zero_Config_Station = MSTP_Zero_Config_Station_Increment(
                mstp_port->Zero_Config_Station);
            mstp_port->Poll_Count = 0;
        } else if (
            (frame == FRAME_TYPE_POLL_FOR_MASTER) &&
            (dst == mstp_port->Zero_Config_Station)) {
            /* calculate this node poll count priority number */
            count = Nmin_poll + mstp_port->Npoll_slot;
            if (mstp_port->Poll_Count == count) {
                /* LurkPollResponse */
                MSTP_Create_And_Send_Frame(
                    mstp_port, FRAME_TYPE_REPLY_TO_POLL_FOR_MASTER, src,
                    mstp_port->Zero_Config_Station, NULL, 0);
                mstp_port->Zero_Config_State = MSTP_ZERO_CONFIG_STATE_CLAIM;
            } else {
                /* LurkCountFrame */
                mstp_port->Poll_Count++;
            }
        }
    } else if (mstp_port->ReceivedInvalidFrame) {
        /* LurkInvalidFrame */
        mstp_port->ReceivedInvalidFrame = false;
    } else if (mstp_port->ReceivedValidFrameNotForUs) {
        /* LurkValidFrameNotForUs */
        mstp_port->ReceivedValidFrameNotForUs = false;
    } else if (mstp_port->Zero_Config_Silence > 0) {
        if (mstp_port->SilenceTimer((void *)mstp_port) >
            mstp_port->Zero_Config_Silence) {
            /* LurkTimeout */
            mstp_port->Zero_Config_State = MSTP_ZERO_CONFIG_STATE_IDLE;
        }
    }
}

/**
 * @brief The ZERO_CONFIGURATION_CLAIM state is entered when a node
 *  is waiting for a Token frame from the master to which it
 *  previously sent a Reply To Poll For Master frame, and
 *  ZeroConfigurationMode is TRUE.
 * @param mstp_port the context of the MSTP port
 */
static void MSTP_Zero_Config_State_Claim(struct mstp_port_struct_t *mstp_port)
{
    uint8_t frame, src, dst;

    if (!mstp_port) {
        return;
    }
    if (mstp_port->ReceivedValidFrame) {
        mstp_port->ReceivedValidFrame = false;
        dst = mstp_port->DestinationAddress;
        src = mstp_port->SourceAddress;
        frame = mstp_port->FrameType;
        if (src == mstp_port->Zero_Config_Station) {
            /* ClaimAddressInUse */
            /* monitor PFM from the next address */
            mstp_port->Zero_Config_Station = MSTP_Zero_Config_Station_Increment(
                mstp_port->Zero_Config_Station);
            mstp_port->Poll_Count = 0;
            mstp_port->Zero_Config_State = MSTP_ZERO_CONFIG_STATE_LURK;
        } else if (frame == FRAME_TYPE_TOKEN) {
            if (dst == mstp_port->Zero_Config_Station) {
                /* ClaimTokenForUs */
                MSTP_Create_And_Send_Frame(
                    mstp_port, FRAME_TYPE_TEST_REQUEST, src,
                    mstp_port->Zero_Config_Station, mstp_port->UUID,
                    MSTP_UUID_SIZE);
                mstp_port->Zero_Config_State = MSTP_ZERO_CONFIG_STATE_CONFIRM;
            }
        }
    } else if (mstp_port->ReceivedInvalidFrame) {
        /* ClaimInvalidFrame */
        mstp_port->ReceivedInvalidFrame = false;
    } else if (mstp_port->ReceivedValidFrameNotForUs) {
        /* ClaimValidFrameNotForUs */
        mstp_port->ReceivedValidFrameNotForUs = false;
    } else if (mstp_port->Zero_Config_Silence > 0) {
        /* ClaimTimeout */
        if (mstp_port->SilenceTimer((void *)mstp_port) >
            mstp_port->Zero_Config_Silence) {
            mstp_port->Zero_Config_State = MSTP_ZERO_CONFIG_STATE_IDLE;
        }
    }
}

/**
 * @brief The ZERO_CONFIGURATION_CONFIRM state is entered when
 *  a node is waiting for a Test Response frame and
 *  ZeroConfigurationMode is TRUE.
 * @param mstp_port the context of the MSTP port
 */
static void MSTP_Zero_Config_State_Confirm(struct mstp_port_struct_t *mstp_port)
{
    bool match = false;
    uint8_t frame, src, dst;

    if (!mstp_port) {
        return;
    }
    if (mstp_port->ReceivedValidFrame) {
        mstp_port->ReceivedValidFrame = false;
        dst = mstp_port->DestinationAddress;
        src = mstp_port->SourceAddress;
        frame = mstp_port->FrameType;
        /* note: test frame could be from us. Check frame type first. */
        if (frame == FRAME_TYPE_TEST_RESPONSE) {
            if (dst == mstp_port->Zero_Config_Station) {
                match = true;
            }
            if (match && (mstp_port->DataLength < MSTP_UUID_SIZE)) {
                match = false;
            }
            if (match &&
                (memcmp(
                     mstp_port->InputBuffer, mstp_port->UUID, MSTP_UUID_SIZE) !=
                 0)) {
                match = false;
            }
            if (match) {
                /* ConfirmationSuccessful */
                mstp_port->This_Station = mstp_port->Zero_Config_Station;
                mstp_port->Zero_Config_State = MSTP_ZERO_CONFIG_STATE_USE;
            } else {
                /* ConfirmationFailed */
                mstp_port->Zero_Config_State = MSTP_ZERO_CONFIG_STATE_IDLE;
            }
        } else if (src == mstp_port->Zero_Config_Station) {
            /* ConfirmationAddressInUse */
            /* monitor PFM from the next address */
            mstp_port->Zero_Config_Station = MSTP_Zero_Config_Station_Increment(
                mstp_port->Zero_Config_Station);
            mstp_port->Zero_Config_State = MSTP_ZERO_CONFIG_STATE_LURK;
        }
    } else if (mstp_port->ReceivedInvalidFrame) {
        /* ConfirmationInvalidFrame */
        mstp_port->ReceivedInvalidFrame = false;
    } else if (mstp_port->ReceivedValidFrameNotForUs) {
        /* ConfirmationValidFrameNotForUs */
        mstp_port->ReceivedValidFrameNotForUs = false;
    } else if (
        mstp_port->SilenceTimer((void *)mstp_port) >=
        mstp_port->Treply_timeout) {
        /* ConfirmationTimeout */
        /* In case validating device doesn't support Test Request */
        /* no response and no collision */
        mstp_port->This_Station = mstp_port->Zero_Config_Station;
        mstp_port->Zero_Config_State = MSTP_ZERO_CONFIG_STATE_USE;
    }
}

/**
 * @brief Finite State Machine for the Zero Configuration process
 * @param mstp_port the context of the MSTP port
 */
void MSTP_Zero_Config_FSM(struct mstp_port_struct_t *mstp_port)
{
    if (!mstp_port) {
        return;
    }
    if (!mstp_port->ZeroConfigEnabled) {
        return;
    }
    switch (mstp_port->Zero_Config_State) {
        case MSTP_ZERO_CONFIG_STATE_INIT:
            MSTP_Zero_Config_State_Init(mstp_port);
            break;
        case MSTP_ZERO_CONFIG_STATE_IDLE:
            MSTP_Zero_Config_State_Idle(mstp_port);
            break;
        case MSTP_ZERO_CONFIG_STATE_LURK:
            MSTP_Zero_Config_State_Lurk(mstp_port);
            break;
        case MSTP_ZERO_CONFIG_STATE_CLAIM:
            MSTP_Zero_Config_State_Claim(mstp_port);
            break;
        case MSTP_ZERO_CONFIG_STATE_CONFIRM:
            MSTP_Zero_Config_State_Confirm(mstp_port);
            break;
        case MSTP_ZERO_CONFIG_STATE_USE:
            break;
        default:
            break;
    }
}

/**
 * @brief Get the baud rate for auto-baud at a given index
 * @param baud_rate_index the index of the baud rate
 * @return the baud rate at the index
 * @note A modulo operation keeps the index within the bounds of the array of
 *  baud rates.
 */
uint32_t MSTP_Auto_Baud_Rate(unsigned baud_rate_index)
{
    const uint32_t TestBaudrates[6] = {
        115200, 76800, 57600, 38400, 19200, 9600
    };
    unsigned index;

    index = baud_rate_index % ARRAY_SIZE(TestBaudrates);

    return TestBaudrates[index];
}

/**
 * @brief The MSTP_AUTO_BAUD_STATE_INIT state is entered when
 *  CheckAutoBaud is TRUE
 * @param mstp_port the context of the MSTP port
 */
static void MSTP_Auto_Baud_State_Init(struct mstp_port_struct_t *mstp_port)
{
    uint32_t baud;

    if (!mstp_port) {
        return;
    }
    mstp_port->ValidFrames = 0;
    mstp_port->BaudRateIndex = 0;
    mstp_port->ValidFrameTimerReset((void *)mstp_port);
    baud = MSTP_Auto_Baud_Rate(mstp_port->BaudRateIndex);
    mstp_port->BaudRateSet(baud);
    mstp_port->Auto_Baud_State = MSTP_AUTO_BAUD_STATE_IDLE;
}

/**
 * @brief The MSTP_AUTO_BAUD_STATE_IDLE state is entered when
 *  CheckAutoBaud is TRUE and waits for good frames or timeout
 * @param mstp_port the context of the MSTP port
 */
static void MSTP_Auto_Baud_State_Idle(struct mstp_port_struct_t *mstp_port)
{
    uint32_t baud;

    if (!mstp_port) {
        return;
    }
    if (mstp_port->ReceivedValidFrame) {
        /* IdleValidFrame */
        mstp_port->ValidFrames++;
        if (mstp_port->ValidFrames >= 4) {
            /* GoodBaudRate */
            mstp_port->CheckAutoBaud = false;
            mstp_port->Auto_Baud_State = MSTP_AUTO_BAUD_STATE_USE;
        }
        mstp_port->ReceivedValidFrame = false;
    } else if (mstp_port->ReceivedInvalidFrame) {
        /* IdleInvalidFrame */
        mstp_port->ValidFrames = 0;
        mstp_port->ReceivedInvalidFrame = false;
    } else if (mstp_port->ValidFrameTimer((void *)mstp_port) >= 5000UL) {
        /* IdleTimeout */
        mstp_port->BaudRateIndex++;
        baud = MSTP_Auto_Baud_Rate(mstp_port->BaudRateIndex);
        mstp_port->BaudRateSet(baud);
        mstp_port->ValidFrames = 0;
        mstp_port->ValidFrameTimerReset((void *)mstp_port);
    }
}

/**
 * @brief Finite State Machine for the Auto Baud Rate process
 * @param mstp_port the context of the MSTP port
 */
void MSTP_Auto_Baud_FSM(struct mstp_port_struct_t *mstp_port)
{
    if (!mstp_port) {
        return;
    }
    if (!mstp_port->CheckAutoBaud) {
        return;
    }
    switch (mstp_port->Auto_Baud_State) {
        case MSTP_AUTO_BAUD_STATE_INIT:
            MSTP_Auto_Baud_State_Init(mstp_port);
            break;
        case MSTP_AUTO_BAUD_STATE_IDLE:
            MSTP_Auto_Baud_State_Idle(mstp_port);
            break;
        case MSTP_AUTO_BAUD_STATE_USE:
            break;
        default:
            break;
    }
}

/* note: This_Station assumed to be set with the MAC address */
/* note: Nmax_info_frames assumed to be set (default=1) */
/* note: Nmax_master assumed to be set (default=127) */
/* note: InputBuffer and InputBufferSize assumed to be set */
/* note: OutputBuffer and OutputBufferSize assumed to be set */
/* note: SilenceTimer and SilenceTimerReset assumed to be set */
void MSTP_Init(struct mstp_port_struct_t *mstp_port)
{
    if (mstp_port) {
#if 0
        /* FIXME: you must point these buffers to actual byte buckets
           in the dlmstp function before calling this init. */
        mstp_port->InputBuffer = &InputBuffer[0];
        mstp_port->InputBufferSize = sizeof(InputBuffer);
        mstp_port->OutputBuffer = &OutputBuffer[0];
        mstp_port->OutputBufferSize = sizeof(OutputBuffer);
        /* FIXME: these are adjustable, so you must set these in dlmstp */
        mstp_port->Nmax_info_frames = DEFAULT_MAX_INFO_FRAMES;
        mstp_port->Nmax_master = DEFAULT_MAX_MASTER;
        mstp_port->Tframe_abort = DEFAULT_Tframe_abort;
        mstp_port->Treply_delay = DEFAULT_Treply_delay;
        mstp_port->Treply_timeout = DEFAULT_Treply_timeout;
        mstp_port->Tusage_timeout = DEFAULT_Tusage_timeout;
        mstp_port->SlaveNodeEnabled = false;
        /* FIXME: point to functions */
        mstp_port->SilenceTimer = Timer_Silence;
        mstp_port->SilenceTimerReset = Timer_Silence_Reset;
        /* FIXME: set these in your dlmstp if you are zero-config */
        mstp_port->ZeroConfigEnabled = true;
        /* use the libc srand() and rand() generated random number*/
        MSTP_Zero_Config_UUID_Init(&MSTP_Port);
#endif
        if ((mstp_port->Tframe_abort < 6) || (mstp_port->Tframe_abort > 100)) {
            mstp_port->Tframe_abort = DEFAULT_Tframe_abort;
        }
        if ((mstp_port->Treply_delay == 0) || mstp_port->Treply_delay > 250) {
            mstp_port->Treply_delay = DEFAULT_Treply_delay;
        }
        if ((mstp_port->Treply_timeout < 20) ||
            (mstp_port->Treply_timeout > 300)) {
            mstp_port->Treply_timeout = DEFAULT_Treply_timeout;
        }
        if ((mstp_port->Tusage_timeout < 20) ||
            (mstp_port->Tusage_timeout > 35)) {
            mstp_port->Tusage_timeout = DEFAULT_Tusage_timeout;
        }
        mstp_port->receive_state = MSTP_RECEIVE_STATE_IDLE;
        mstp_port->master_state = MSTP_MASTER_STATE_INITIALIZE;
        mstp_port->ReceiveError = false;
        mstp_port->DataAvailable = false;
        mstp_port->DataRegister = 0;
        mstp_port->DataCRC = 0;
        mstp_port->DataLength = 0;
        mstp_port->DestinationAddress = 0;
        mstp_port->EventCount = 0;
        mstp_port->FrameType = FRAME_TYPE_TOKEN;
        mstp_port->FrameCount = 0;
        mstp_port->HeaderCRC = 0;
        mstp_port->Index = 0;
        mstp_port->Next_Station = mstp_port->This_Station;
        mstp_port->Poll_Station = mstp_port->This_Station;
        mstp_port->ReceivedInvalidFrame = false;
        mstp_port->ReceivedValidFrame = false;
        mstp_port->RetryCount = 0;
        mstp_port->SilenceTimerReset(mstp_port);
        mstp_port->SoleMaster = false;
        mstp_port->SourceAddress = 0;
        mstp_port->TokenCount = 0;
        /* zero config */
        mstp_port->Zero_Config_State = MSTP_ZERO_CONFIG_STATE_INIT;
    }
}
