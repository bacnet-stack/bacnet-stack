/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2023
 * @brief BACnet FreeRTOS MSTP datalink API
 *
 * SPDX-License-Identifier: MIT
 *
 */
#ifndef DLMSTP_INIT_H
#define DLMSTP_INIT_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "bacnet/basic/sys/ringbuf.h"
#include "bacnet/datalink/crc.h"
#include "bacnet/datalink/dlmstp.h"
#include "bacnet/datalink/mstp.h"

#ifndef DLMSTP_MAX_INFO_FRAMES
#define DLMSTP_MAX_INFO_FRAMES DEFAULT_MAX_INFO_FRAMES
#endif
#ifndef DLMSTP_MAX_MASTER
#define DLMSTP_MAX_MASTER DEFAULT_MAX_MASTER
#endif
#ifndef DLMSTP_BAUD_RATE_DEFAULT
#define DLMSTP_BAUD_RATE_DEFAULT 38400UL
#endif

/**
 * The structure of RS485 driver for BACnet MS/TP
 */
struct rs485_driver {

    /** Initialize the driver hardware */
    void (* init)(void);

    /** Prepare & transmit a packet. */
    void (* send)(uint8_t *payload, uint16_t payload_len);

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
struct mstp_user_data_t {
    struct dlmstp_statistics Statistics;
    struct rs485_driver *RS485_Driver;
    RING_BUFFER PDU_Queue;
    SemaphoreHandle_t PDU_Mutex;
    bool Initialized;
    uint8_t Input_Buffer[DLMSTP_MPDU_MAX];
    bool ReceivePacketPending;
    uint8_t Output_Buffer[DLMSTP_MPDU_MAX];
    struct dlmstp_packet PDU_Buffer[DLMSTP_MAX_INFO_FRAMES];
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void dlmstp_freertos_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
