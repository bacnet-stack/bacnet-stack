/**
 * @file
 * @brief XMEGA-A3BU BACnet application
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2014
 * @copyright SPDX-License-Identifier: MIT
 */
#include <asf.h>
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlmstp.h"
#include "bacnet/datalink/mstp.h"
#include "rs485.h"
#include "led.h"
#include "adc-hdw.h"
#include "bacnet.h"

static struct mstimer_callback_data_t BACnet_Callback;

/* MS/TP port */
static struct mstp_port_struct_t MSTP_Port;
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
static struct dlmstp_user_data_t MSTP_User_Data;
static uint8_t Input_Buffer[DLMSTP_MPDU_MAX];
static uint8_t Output_Buffer[DLMSTP_MPDU_MAX];

/**
 * @brief MS/TP configuraiton
 */
static void dlmstp_configure(void)
{
    /* initialize MSTP datalink layer */
    MSTP_Port.Nmax_info_frames = DLMSTP_MAX_INFO_FRAMES;
    MSTP_Port.Nmax_master = DLMSTP_MAX_MASTER;
    MSTP_Port.InputBuffer = Input_Buffer;
    MSTP_Port.InputBufferSize = sizeof(Input_Buffer);
    MSTP_Port.OutputBuffer = Output_Buffer;
    MSTP_Port.OutputBufferSize = sizeof(Output_Buffer);
    /* user data */
    MSTP_Port.ZeroConfigEnabled = true;
    MSTP_Port.SlaveNodeEnabled = false;
    MSTP_Zero_Config_UUID_Init(&MSTP_Port);
    MSTP_User_Data.RS485_Driver = &RS485_Driver;
    MSTP_Port.UserData = &MSTP_User_Data;
    dlmstp_init((char *)&MSTP_Port);
    if (MSTP_Port.ZeroConfigEnabled) {
        dlmstp_set_mac_address(255);
    } else {
        /* FIXME: get the address from hardware DIP or from EEPROM */
        dlmstp_set_mac_address(1);
    }
    /* FIXME: get the baud rate from hardware DIP or from EEPROM */
    dlmstp_set_baud_rate(DLMSTP_BAUD_RATE_DEFAULT);
}

/**
 * \brief Main function.
 *
 * Initializes the board, and runs the application in an infinite loop.
 */
int main(void)
{
    /* hardware initialization */
    sysclk_init();
    board_init();
    pmic_init();
    mstimer_init();
    rs485_init();
    led_init();
    adc_init();
#ifdef CONF_BOARD_ENABLE_RS485_XPLAINED
    // Enable display backlight
    gpio_set_pin_high(NHD_C12832A1Z_BACKLIGHT);
#endif
    // Workaround for known issue: Enable RTC32 sysclk
    sysclk_enable_module(SYSCLK_PORT_GEN, SYSCLK_RTC);
    while (RTC32.SYNCCTRL & RTC32_SYNCBUSY_bm) {
        // Wait for RTC32 sysclk to become stable
    }
    cpu_irq_enable();
    /* application initialization */
    rs485_baud_rate_set(38400);
    dlmstp_configure();
    bacnet_init();
    /*  run forever - timed tasks */
    mstimer_callback(&BACnet_Callback, bacnet_task_timed, 5);
    for (;;) {
        bacnet_task();
        led_task();
    }
}
