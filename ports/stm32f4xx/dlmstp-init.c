/**
 * @file
 * @brief Configuration BACnet MSTP datalink
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
#include "dlmstp-init.h"
#include "rs485.h"

/* MS/TP port */
static volatile struct mstp_port_struct_t MSTP_Port;
static struct mstp_rs485_driver RS485_Driver = { 
    .send = rs485_bytes_send,
    .read = rs485_byte_available,
    .transmitting = rs485_rts_enabled,
    .baud_rate = rs485_baud_rate,
    .baud_rate_set = rs485_baud_rate_set };
static struct mstp_user_data_t MSTP_User_Data;

/**
 * @brief initialize the datalink for this product
 */
void dlmstp_framework_init(void)
{
    MSTP_Port.InputBuffer = &MSTP_User_Data.Input_Buffer[0];
    MSTP_Port.InputBufferSize = sizeof(MSTP_User_Data.Input_Buffer);
    MSTP_Port.OutputBuffer = &MSTP_User_Data.Output_Buffer[0];
    MSTP_Port.OutputBufferSize = sizeof(MSTP_User_Data.Output_Buffer);
    MSTP_Port.Nmax_info_frames = DLMSTP_MAX_INFO_FRAMES;
    MSTP_Port.Nmax_master = DLMSTP_MAX_MASTER;
    MSTP_Port.SilenceTimer = rs485_silence_milliseconds;
    MSTP_Port.SilenceTimerReset = rs485_silence_reset;
    /* user data */
    MSTP_Port.UserData = &MSTP_User_Data;
    MSTP_User_Data.RS485_Driver = &RS485_Driver;
    Ringbuf_Init(&MSTP_User_Data.PDU_Queue,
        (volatile uint8_t *)&MSTP_User_Data.PDU_Buffer,
        sizeof(struct dlmstp_packet), DLMSTP_MAX_INFO_FRAMES);
    /* initialize the datalink */
    dlmstp_init((char *)&MSTP_Port);
}
