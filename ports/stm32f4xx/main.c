/**
 * @file
 * @brief Main function for the STM32F4xx NUCLEO board
 * @author Steve Karg
 * @date 2021
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "stm32f4xx.h"
#include "stm32f4xx_pwr.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_rng.h"
#include "system_stm32f4xx.h"
#include "bacnet/basic/object/bacfile.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/program.h"
#include "bacnet/basic/sys/bramfs.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/sys/ringbuf.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlmstp.h"
#include "bacnet/datalink/mstp.h"
#include "rs485.h"
#include "led.h"
#include "bacnet.h"
#include "program-ubasic.h"

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
static const char *UBASIC_Program_1 =
    /* program listing with either \0, \n, or ';' at the end of each line.
       note: indentation is not required */
    "println 'Demo - GPIO';"
    ":loop;"
    "  dwrite(3, 1);"
    "  sleep (0.3);"
    "  dwrite(3, 0);"
    "  sleep (0.3);"
    "goto loop;"
    "end;";
static const char *UBASIC_Program_2 =
    /* program listing with either \0, \n, or ';' at the end of each line.
       note: indentation is not required */
    "println 'Demo - GPIO';"
    ":loop;"
    "  dwrite(2, 1);"
    "  sleep (0.5);"
    "  dwrite(2, 0);"
    "  sleep (0.1);"
    "goto loop;"
    "end;";
static const char *UBASIC_Program_3 =
    /* program listing with either \0, \n, or ';' at the end of each line.
       note: indentation is not required */
    "println 'Demo - BACnet & GPIO';"
    "bac_create(0, 1, 'ADC-1-AVG');"
    "bac_create(0, 2, 'ADC-2-AVG');"
    "bac_create(0, 3, 'ADC-1-RAW');"
    "bac_create(0, 4, 'ADC-3-RAW');"
    "bac_create(4, 1, 'LED-1');"
    ":startover;"
    "  a = aread(1);"
    "  bac_write(0, 3, 85, a);"
    "  c = avgw(a, c, 10);"
    "  bac_write(0, 1, 85, c);"
    "  b = aread(2);"
    "  bac_write(0, 4, 85, b);"
    "  d = avgw(b, d, 10);"
    "  bac_write(0, 2, 85, d);"
    "  h = bac_read(4, 1, 85);"
    "  dwrite(1, (h % 2));"
    "  sleep (0.2);"
    "goto startover;"
    "end;";
/* uBASIC data tree for each program running */
static struct ubasic_data UBASIC_Data[3];

/**
 * @brief Called from _write() function from printf and friends
 * @param[in] ch Character to send
 */
int __io_putchar(int ch)
{
    (void)ch;
    return 0;
}

/**
 * @brief Main function
 * @return 0 - never returns
 */
int main(void)
{
    /*At this stage the microcontroller clock setting is already configured,
       this is done through SystemInit() function which is called from startup
       file (startup_stm32f4xx.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32f4xx.c file */
    SystemCoreClockUpdate();
    /* enable some clocks - USART and GPIO clocks are enabled in our drivers */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);
    /* initialize hardware layer */
    mstimer_init();
    led_init();
    rs485_init();
    /* FIXME: get the device ID from EEPROM */
    Device_Set_Object_Instance_Number(103);
    /* seed stdlib rand() with device-id to get pseudo consistent
       zero-config poll slot, or use hardware RNG to get a more random slot */
#ifdef BACNET_ZERO_CONFIG_RNG_HARDWARE
    /* enable the random number generator hardware */
    RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);
    RNG_Cmd(ENABLE);
    while (RNG_GetFlagStatus(RNG_FLAG_DRDY) == RESET) {
        /* wait for 32-bit random number to generate */
    }
    srand(RNG_GetRandomNumber());
#else
    srand(Device_Object_Instance_Number());
#endif
    /* initialize the Device UUID from rand() */
    Device_UUID_Init();
    Device_UUID_Get(MSTP_Port.UUID, sizeof(MSTP_Port.UUID));
    /* initialize MSTP datalink layer */
    MSTP_Port.Nmax_info_frames = DLMSTP_MAX_INFO_FRAMES;
    MSTP_Port.Nmax_master = DLMSTP_MAX_MASTER;
    MSTP_Port.InputBuffer = Input_Buffer;
    MSTP_Port.InputBufferSize = sizeof(Input_Buffer);
    MSTP_Port.OutputBuffer = Output_Buffer;
    MSTP_Port.OutputBufferSize = sizeof(Output_Buffer);
    /* choose from non-volatile configuration for zero-config or slave mode */
    MSTP_Port.ZeroConfigEnabled = false;
    MSTP_Port.Zero_Config_Preferred_Station = 0;
    MSTP_Port.SlaveNodeEnabled = false;
    MSTP_Port.CheckAutoBaud = false;
    /* user data */
    MSTP_User_Data.RS485_Driver = &RS485_Driver;
    MSTP_Port.UserData = &MSTP_User_Data;
    dlmstp_init((char *)&MSTP_Port);
    if (MSTP_Port.ZeroConfigEnabled) {
        /* set node to monitor address */
        dlmstp_set_mac_address(255);
    } else {
        /* FIXME: get the address from hardware DIP or from EEPROM */
        dlmstp_set_mac_address(63);
    }
    if (!MSTP_Port.CheckAutoBaud) {
        /* FIXME: get the baud rate from hardware DIP or from EEPROM */
        dlmstp_set_baud_rate(DLMSTP_BAUD_RATE_DEFAULT);
    }
    /* initialize application layer*/
    bacnet_init();
    bacfile_ramfs_init();
    /* configure the program object and loop time */
    Program_UBASIC_Init(10);
    /* create the uBASIC programs and link to file objects */
    bacfile_create(1);
    bacfile_pathname_set(1, "/program1.bas");
    Program_Instance_Of_Set(1, bacfile_pathname(1));
    bacfile_write(
        1, (const uint8_t *)UBASIC_Program_1, strlen(UBASIC_Program_1));
    Program_UBASIC_Create(1, &UBASIC_Data[0], UBASIC_Program_1);

    Program_UBASIC_Create(2, &UBASIC_Data[1], UBASIC_Program_2);
    bacfile_create(2);
    bacfile_pathname_set(2, "/program2.bas");
    Program_Instance_Of_Set(2, bacfile_pathname(2));
    bacfile_write(
        2, (const uint8_t *)UBASIC_Program_2, strlen(UBASIC_Program_2));

    Program_UBASIC_Create(3, &UBASIC_Data[2], UBASIC_Program_3);
    bacfile_create(3);
    bacfile_pathname_set(3, "/program3.bas");
    Program_Instance_Of_Set(3, bacfile_pathname(3));
    bacfile_write(
        3, (const uint8_t *)UBASIC_Program_3, strlen(UBASIC_Program_3));

    for (;;) {
        led_task();
        bacnet_task();
        Program_UBASIC_Task();
    }
}
