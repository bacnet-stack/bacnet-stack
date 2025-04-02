/************************************************************************
 *
 * Copyright (C) 2011 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "hardware.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlmstp.h"
#include "bacnet/datalink/mstp.h"
#include "bacnet/basic/object/device.h"
#include "rs485.h"
#include "led.h"
#include "bacnet.h"

/* local version override */
char *BACnet_Version = "1.0";
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
 * @brief Called from _write() function from printf and friends
 * @param[in] ch Character to send
 */
int __io_putchar(int ch)
{
    (void)ch;
    return 0;
}

#ifdef USE_FULL_ASSERT

/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line
       number, ex: printf("Wrong parameters value: file %s on line %d\r\n",
       file, line) */

    /* Infinite loop */
    while (1) { }
}
#endif

/* Private define ------------------------------------------------------------*/
#define LSE_FAIL_FLAG 0x80
#define LSE_PASS_FLAG 0x100

void lse_init(void)
{
    uint32_t LSE_Delay = 0;
    struct mstimer Delay_Timer;

    /* Enable access to the backup register => LSE can be enabled */
    PWR_BackupAccessCmd(ENABLE);
    /* Enable LSE (Low Speed External Oscillation) */
    RCC_LSEConfig(RCC_LSE_ON);

    /* Check the LSE Status */
    while (1) {
        if (LSE_Delay < LSE_FAIL_FLAG) {
            mstimer_set(&Delay_Timer, 0);
            while (mstimer_elapsed(&Delay_Timer) < 500) {
                /* do nothing */
            }
            /* check whether LSE is ready, with 4 seconds timeout */
            LSE_Delay += 0x10;
            if (RCC_GetFlagStatus(RCC_FLAG_LSERDY) != RESET) {
                /* Set flag: LSE PASS */
                LSE_Delay |= LSE_PASS_FLAG;
                led_ld4_off();
                /* Disable LSE */
                RCC_LSEConfig(RCC_LSE_OFF);
                break;
            }
        }

        /* LSE_FAIL_FLAG = 0x80 */
        else if (LSE_Delay >= LSE_FAIL_FLAG) {
            if (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET) {
                /* Set flag: LSE FAIL */
                LSE_Delay |= LSE_FAIL_FLAG;
                led_ld4_on();
            }
            /* Disable LSE */
            RCC_LSEConfig(RCC_LSE_OFF);
            break;
        }
    }
}

/**
 * @brief Configure the MSTP datalink layer
 */
static void mstp_configure(void)
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
    MSTP_Port.CheckAutoBaud = false;
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
 * @brief Main entry point of the C application
 */
int main(void)
{
    struct mstimer Blink_Timer;
    uint32_t Object_Instance_Number = 103;

    /*At this stage the microcontroller clock setting is already configured,
       this is done through SystemInit() function which is called from startup
       file (startup_stm32f10x_xx.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f10x.c file */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
            RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD | RCC_APB2Periph_GPIOE,
        ENABLE);
    /* initialize hardware layer */
    mstimer_init();
    lse_init();
    led_init();
    /* FIXME: get device instance from EEPROM */
    (void)Device_Set_Object_Instance_Number(Object_Instance_Number);
    /* seed libc random number generator */
    srand(Object_Instance_Number);
    /* initialize MSTP datalink layer */
    mstp_configure();
    /* initialize application layer*/
    bacnet_init();
    mstimer_set(&Blink_Timer, 125);
    for (;;) {
        if (mstimer_expired(&Blink_Timer)) {
            mstimer_reset(&Blink_Timer);
            led_ld3_toggle();
        }
        led_task();
        bacnet_task();
    }
}
