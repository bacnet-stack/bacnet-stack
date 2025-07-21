/**
 * @file
 * @brief XMEGA-A3BU BACnet application
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2014
 * @copyright SPDX-License-Identifier: MIT
 */
#include <asf.h>
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlmstp.h"
#include "bacnet/datalink/mstp.h"
#include "rs485.h"
#include "led.h"
#include "adc-hdw.h"
#include "bacnet.h"
#include "nvmdata.h"

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
 * Initializes some data from non-volatile memory module
 */
static void nvm_data_init(void)
{
    uint32_t device_id = 127;
    uint8_t max_master = 127;
    uint8_t mac_address = 127;
    uint8_t kbaud_rate = 38;

    nvm_read(NVM_BAUD_K, &kbaud_rate, 1);
    rs485_kbaud_rate_set(kbaud_rate);

    nvm_read(NVM_MAC_ADDRESS, &mac_address, 1);
    dlmstp_set_mac_address(mac_address);
    nvm_read(NVM_MAX_MASTER, &max_master, 1);
    if (max_master > 127) {
        max_master = 127;
    }
    dlmstp_set_max_master(max_master);

    /* Get the device ID from the EEPROM */
    nvm_read(NVM_DEVICE_0, (uint8_t *)&device_id, sizeof(device_id));
    if (device_id < BACNET_MAX_INSTANCE) {
        Device_Set_Object_Instance_Number(device_id);
    } else {
        Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    }
}

/**
 * @brief MS/TP configuraiton
 */
static void dlmstp_configure(void)
{
    uint8_t mac_address = 0;

    /* initialize MSTP datalink layer */
    MSTP_Port.Nmax_info_frames = DLMSTP_MAX_INFO_FRAMES;
    MSTP_Port.Nmax_master = DLMSTP_MAX_MASTER;
    MSTP_Port.InputBuffer = Input_Buffer;
    MSTP_Port.InputBufferSize = sizeof(Input_Buffer);
    MSTP_Port.OutputBuffer = Output_Buffer;
    MSTP_Port.OutputBufferSize = sizeof(Output_Buffer);
    /* user data */
    mac_address = dlmstp_mac_address();
    if (mac_address == 255) {
        MSTP_Port.ZeroConfigEnabled = true;
    } else {
        MSTP_Port.ZeroConfigEnabled = false;
    }
    if (mac_address <= 127) {
        MSTP_Port.SlaveNodeEnabled = false;
    } else {
        MSTP_Port.SlaveNodeEnabled = true;
    }
    MSTP_Port.CheckAutoBaud = false;
    if (!MSTP_Port.CheckAutoBaud) {
        /* FIXME: get the baud rate from hardware DIP or from EEPROM */
        dlmstp_set_baud_rate(DLMSTP_BAUD_RATE_DEFAULT);
    }
    MSTP_Zero_Config_UUID_Init(&MSTP_Port);
    MSTP_User_Data.RS485_Driver = &RS485_Driver;
    MSTP_Port.UserData = &MSTP_User_Data;
    dlmstp_init((char *)&MSTP_Port);
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
    nvm_data_init();
    dlmstp_configure();
    bacnet_init();
    /*  run forever - timed tasks */
    mstimer_callback(&BACnet_Callback, bacnet_task_timed, 5);
    for (;;) {
        bacnet_task();
        led_task();
    }
}
