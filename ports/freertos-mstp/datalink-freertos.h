/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2023
 * @brief BACnet FreeRTOS MSTP datalink API
 *
 * SPDX-License-Identifier: MIT
 *
 */
#ifndef DATALINK_FREERTOS_MSTP_H
#define DATALINK_FREERTOS_MSTP_H

#include <stdint.h>
#include <stdbool.h>
#include <FreeRTOS.h>
#include "bacnet/basic/sys/ringbuf.h"
#include "bacnet/datalink/crc.h"

/**
 * The structure of RS485 driver for BACnet MS/TP
 */
struct rs485_driver {

    /** Initialize the driver hardware */
    void (* init)(void);

    /** Prepare & transmit a packet. */
    bool (* send)(uint8_t *payload, uint16_t payload_len);

    /** Check if one received byte is available */
    bool (* read)(uint8_t *buf);

    /** true if the driver is transmitting */
    bool (* transmitting)(void);

    /** Get the current baud rate */
    uint32_t (* baud_rate)(void);

    /** Set the current baud rate */
    bool (* baud_rate_set)(uint32_t baud);
};

/**
 * The structure of BACnet Port Data for BACnet MS/TP
 */
struct bacnet_port_data {
    /* common RS485 driver functions */
    struct rs485_driver *RS485_Driver;
    /* send PDU ring buffer */
    RING_BUFFER PDU_Queue;
    xSemaphoreHandle PDU_Mutex;
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void datalink_freertos_non_volatile_init(void);
void datalink_freertos_defaults_init(void);
void datalink_freertos_init(void);
bool datalink_freertos_send_pdu_queue_full(void);

uint16_t datalink_freertos_receive(
    BACNET_ADDRESS * src,
    uint8_t * pdu,
    uint16_t max_pdu,
    unsigned timeout);
uint16_t datalink_freertos_receive(
    BACNET_ADDRESS * src,
    uint8_t * pdu,
    uint16_t max_pdu,
    unsigned timeout);

uint8_t datalink_freertos_max_info_frames(void);
uint8_t datalink_freertos_max_master(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
