/**************************************************************************
 *
 * Copyright (C) 2025 Testimony Adams <adamstestimony@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/

#include "dlenv.h"
#include <stdio.h>

volatile struct mstp_port_struct_t MSTP_Port;

/* Internal state associated with that port */
static struct dlmstp_user_data_t MSTP_User_Data;
static uint8_t Input_Buffer[DLMSTP_MPDU_MAX];
static uint8_t Output_Buffer[DLMSTP_MPDU_MAX];

static struct dlmstp_rs485_driver RS485_Driver = {
    .init = rs485_init,
    .send = rs485_bytes_send,
    .read = rs485_byte_available,
    .transmitting = rs485_rts_enabled,
    .baud_rate = rs485_baud_rate,
    .baud_rate_set = rs485_baud_rate_set,
    .silence_milliseconds = rs485_silence_milliseconds,
    .silence_reset = rs485_silence_reset
};

// TO BE IMPROVED
volatile bool g_mstp_have_token = false;
volatile uint8_t g_last_frame_type = 0;
volatile uint8_t g_last_src = 0;
volatile uint8_t g_last_dst = 0;

static void mstp_frame_rx_complete_cb(
    uint8_t src, uint8_t dst, uint8_t frame_type,
    const uint8_t *buf, uint16_t len)
{
    (void)buf;
    (void)len;

    g_last_src = src;
    g_last_dst = dst;
    g_last_frame_type = frame_type;

    // TO BE IMPROVED (For other frame types as needed)
    if (frame_type == FRAME_TYPE_TOKEN) {
        g_mstp_have_token = true;
    }
}

bool pico_dlenv_init(uint8_t mac_address)
{
    /* RS-485 low-level init (pins, UART config, etc.) */
    RS485_Driver.init();

    /* Configure MSTP port structure that the core stack will drive */
    MSTP_Port.Nmax_info_frames = BACNET_MSTP_MAX_INFO_FRAMES;
    MSTP_Port.Nmax_master      = BACNET_MSTP_MAX_MASTER;

    MSTP_Port.InputBuffer      = Input_Buffer;
    MSTP_Port.InputBufferSize  = sizeof(Input_Buffer);
    MSTP_Port.OutputBuffer     = Output_Buffer;
    MSTP_Port.OutputBufferSize = sizeof(Output_Buffer);

    /* No ZeroConfig / slaves / auto-baud for this Pico port (you can tweak) */
    MSTP_Port.ZeroConfigEnabled = false;
    MSTP_Port.SlaveNodeEnabled  = false;
    MSTP_Port.CheckAutoBaud     = false;

    MSTP_Zero_Config_UUID_Init((struct mstp_port_struct_t *)&MSTP_Port);

    /* Hook the RS-485 driver into the MS/TP layer */
    MSTP_User_Data.RS485_Driver = &RS485_Driver;
    MSTP_Port.UserData          = &MSTP_User_Data;

    /* Start the MS/TP state machine using our port */
    if (!dlmstp_init((char *)&MSTP_Port)) return false;

    /* Apply logical MS/TP parameters */
    dlmstp_set_mac_address(mac_address);
    dlmstp_set_baud_rate(RS485_BAUD_RATE);
    // dlmstp_slave_mode_enabled_set(true);
    dlmstp_set_max_master(BACNET_MSTP_MAX_MASTER);
    dlmstp_set_max_info_frames(BACNET_MSTP_MAX_INFO_FRAMES);
    dlmstp_set_frame_rx_complete_callback(mstp_frame_rx_complete_cb);
    // TO BE IMPROVED (Add other callbacks as needed)
    dlmstp_set_frame_not_for_us_rx_complete_callback(NULL);
    dlmstp_set_invalid_frame_rx_complete_callback(NULL);
    dlmstp_set_frame_rx_start_callback(NULL);
    
    return true;
}

/* The stack may call this periodically; for this Pico MS/TP-only port
   we don't need extra maintenance, so it's just a no-op. */
void dlenv_maintenance_timer(uint16_t seconds)
{
    (void)seconds;
}

/* Likewise, nothing special to clean up on embedded target */
void dlenv_cleanup(void)
{
    /* no action */
}

