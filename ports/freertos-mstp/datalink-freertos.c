/**
 * @file
 * @brief Configuration FreeRTOS BACnet datalink
 * @date February 2023
 * @author Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "bacnet/basic/sys/ringbuf.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlmstp.h"
#include "bacnet/datalink/mstp.h"
#include "datalink-freertos.h"
#include "rs485.h"

#ifndef DATALINK_MSTP_MAX_INFO_FRAMES
#define DATALINK_MSTP_MAX_INFO_FRAMES DEFAULT_MAX_INFO_FRAMES
#endif
#ifndef DATALINK_MSTP_BAUD_RATE_DEFAULT
#define DATALINK_MSTP_BAUD_RATE_DEFAULT 38400
#endif

/* MS/TP port */
static uint8_t MSTP_MAC_Address;
static uint8_t MSTP_Max_Master;
static uint8_t MSTP_Baud_K = 0;
static volatile struct mstp_port_struct_t MSTP_Port;
static uint8_t Output_Buffer[MAX_MPDU];
static struct rs485_driver RS485_Driver = {
    .init = rs485_init,
    .send = rs485_bytes_send,
    .read = rs485_byte_available,
    .transmitting = rs485_rts_enabled,
    .baud_rate = rs485_baud_rate,
    .baud_rate_set = rs485_baud_rate_set,
    .silence_milliseconds = rs485_silence_milliseconds,
    .silence_reset = rs485_silence_reset
};
static struct bacnet_port_data MSTP_Port_Data;
static struct dlmstp_packet_t PDU_Buffer[CONF_MSTP_MAX_INFO_FRAMES];

/**
 * @brief convert from baud K value stored in EEPROM to baud rate
 * @return baud rate in bps
 */
static unsigned long datalink_mstp_baud_from_baud_k(uint8_t baud_k)
{
    unsigned long baud_rate = 0;
    switch (baud_k) {
        case 9:
            baud_rate = 9600;
            break;
        case 19:
            baud_rate = 19200;
            break;
        case 38:
            baud_rate = 38400;
            break;
        case 57:
            baud_rate = 57600;
            break;
        case 76:
            baud_rate = 76800;
            break;
        case 115:
            baud_rate = 115200;
            break;
        default:
			baud_rate = 115200;
            break;
    }

    return baud_rate;
}

/**
 * @brief convert from baud K value stored in EEPROM to baud rate
 * @return baud rate in bps
 */
static uint8_t datalink_mstp_baud_to_baud_k(unsigned long baud_rate)
{
    uint8_t baud_k = 0;

    baud_k = baud_rate / 1000;

    return baud_k;
}

/**
 * @brief Initialize datalink variables from non-volatile storage
 */
void datalink_freertos_non_volatile_init(void)
{
    /* get data from non-volatile memory and bounds check */
    if (MSTP_MAC_Address > 127) {
        MSTP_MAC_Address = 127;
    }
    /* get data from non-volatile memory and bounds check */
    if (MSTP_Max_Master > 127) {
        MSTP_Max_Master = 127;
    }
    /* get data from non-volatile memory and bounds check */
    if ((MSTP_Baud_K < 9) || (MSTP_Baud_K > 115)) {
        MSTP_Baud_K = datalink_mstp_baud_to_baud_k(
            DATALINK_MSTP_BAUD_RATE_DEFAULT);
    }
}

/**
 * @brief Initialize datalink variables to their defaults
 */
void datalink_freertos_defaults_init(void)
{
    /* configure port MS/TP defaults */
    MSTP_MAC_Address = 127;
    MSTP_Max_Master = 127;
    MSTP_Baud_K = datalink_mstp_baud_to_baud_k(
        DATALINK_MSTP_BAUD_RATE_DEFAULT);
}

/**
 * @brief initialize the datalink for this product
 */
void datalink_freertos_init(void)
{
    /* InputBuffer - set to NULL for zero copy */
    MSTP_Port.InputBuffer = NULL;
    MSTP_Port.InputBufferSize = 0;
    MSTP_Port.OutputBuffer = &Output_Buffer[0];
    MSTP_Port.OutputBufferSize = sizeof(Output_Buffer);
    MSTP_Port.Nmax_info_frames = DATALINK_MSTP_MAX_INFO_FRAMES;
    MSTP_Port.Nmax_master = DATALINK_MSTP_MAX_MASTER;
    /* user data */
    MSTP_Port.UserData = MSTP_Port_Data;
    MSTP_Port_Data.RS485_Driver = &RS485_Driver;
    MSTP_Port_Data.PDU_Mutex = xSemaphoreCreateMutex();
    Ringbuf_Init((RING_BUFFER *)&MSTP_Port_Data.PDU_Queue,
        (volatile uint8_t *)&PDU_Buffer, sizeof(struct dlmstp_packet_t),
        DATALINK_MSTP_MAX_INFO_FRAMES);
    /* configure datalink */
    dlmstp_init(&MSTP_Port);
    dlmstp_set_mac_address(&MSTP_Port, BMS_MSTP_MAC_Address);
    dlmstp_set_max_master(&MSTP_Port, BMS_MSTP_Max_Master);
    dlmstp_set_baud_rate(
        &MSTP_Port, nvdata_baud_from_baud_k(BMS_MSTP_Baud_K));
    dlmstp_rs485_init(&MSTP_Port);
}

/**
 * @brief function to send a packet out the BACnet/IP and BACnet/IPv6 ports
 * @param dest - address to where packet is sent
 * @param npdu_data - NPCI data to control network destination
 * @param pdu - protocol data unit to be sent
 * @param pdu_len - number of bytes to send
 * @return number of bytes sent
 */
int datalink_send_pdu(BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned int pdu_len)
{
    return dlmstp_send_pdu(&MSTP_Port, dest, npdu_data, pdu, pdu_len);
}

/**
 * @brief function to get a packet from the MSTP port
 * @param src - address to where packet is sent from
 * @param pdu - protocol data unit that was sent
 * @param max_pdu - size of buffer for protocol data unit that was sent
 * @param timeout - amount of time to wait, or amount of time since last call
 * @return number of bytes in the PDU received
 */
uint16_t datalink_freertos_receive(
    BACNET_ADDRESS *src, uint8_t *pdu, uint16_t max_pdu, unsigned timeout)
{
    return dlmstp_receive(&MSTP_Port, src, pdu, max_pdu, timeout);
}

/**
 * @brief Initialize the a data link broadcast address
 * @param dest - address to be filled with broadcast designator
 */
void datalink_get_broadcast_address(BACNET_ADDRESS *dest)
{
    if (dest) {
        dest->mac_len = 0;
        dest->net = BACNET_BROADCAST_NETWORK;
        dest->len = 0;
    }

    return;
}

/**
 * @brief Initialize the data link broadcast address
 * @param my_address - address to be filled with unicast designator
 */
void datalink_get_my_address(BACNET_ADDRESS *my_address)
{
    dlmstp_get_my_address(&MSTP_Port, my_address);
}

/**
 * @brief Determine if the send PDU queue is full
 * @return true if the send PDU is full
 */
bool datalink_freertos_send_pdu_queue_full(void)
{
    return dlmstp_send_pdu_queue_full(&MSTP_Port);
}

/**
 * @brief Determine if the send PDU queue is full
 * @return true if the send PDU is full
 */
uint8_t datalink_freertos_max_info_frames(void)
{
    return dlmstp_max_info_frames(&MSTP_Port);
}

/**
 * @brief Determine if the send PDU queue is full
 * @return true if the send PDU is full
 */
uint8_t datalink_freertos_max_master(void)
{
    return dlmstp_max_master(&MSTP_Port);
}
