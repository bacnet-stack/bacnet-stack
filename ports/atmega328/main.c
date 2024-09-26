/**
 * @brief This module manages the main C code for the BACnet demo
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2007
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdbool.h>
#include <stdint.h>
#include "hardware.h"
#include "adc.h"
#include "nvdata.h"
#include "rs485.h"
#include "stack.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/npdu.h"
#include "bacnet/dcc.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/iam.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/av.h"
#include "bacnet/basic/object/bv.h"

/* From the WhoIs hander - performed by the DLMSTP module */
extern bool Send_I_Am_Flag;
/* main loop task timer */
static struct mstimer Task_Timer;
/* uptime */
static uint32_t Uptime_Seconds;

/* dummy function - so we can use default demo handlers */
bool dcc_communication_enabled(void)
{
    return true;
}

static void hardware_init(void)
{
    /* Initialize the Clock Prescaler for ATmega48/88/168 */
    /* The default CLKPSx bits are factory set to 0011 */
    /* Enbable the Clock Prescaler */
    CLKPR = _BV(CLKPCE);
    /* CLKPS3 CLKPS2 CLKPS1 CLKPS0 Clock Division Factor
       ------ ------ ------ ------ ---------------------
       0      0      0      0             1
       0      0      0      1             2
       0      0      1      0             4
       0      0      1      1             8
       0      1      0      0            16
       0      1      0      1            32
       0      1      1      0            64
       0      1      1      1           128
       1      0      0      0           256
       1      x      x      x      Reserved
     */
    /* Set the CLKPS3..0 bits to Prescaler of 1 */
    CLKPR = 0;
    /* Initialize I/O ports */
    /* For Port DDRx (Data Direction) Input=0, Output=1 */
    /* For Port PORTx (Bit Value) TriState=0, High=1 */
    DDRB = 0;
    PORTB = 0;
    DDRC = 0;
    PORTC = 0;
    DDRD = 0;
    PORTD = 0;

    /* Configure the watchdog timer - Disabled for testing */
    BIT_CLEAR(MCUSR, WDRF);
    WDTCSR = 0;

    /* Configure Specialized Hardware */
    RS485_Initialize();
    mstimer_init();
    adc_init();

    /* Enable global interrupts */
    __enable_interrupt();
}

/**
 * @brief process some values once per second
 */
static void one_second_task(void)
{
    BACNET_BINARY_PV value;
    float hours;

    /* LED toggling */
    value = Binary_Value_Present_Value(99);
    if (value == BINARY_ACTIVE) {
        value = BINARY_INACTIVE;
    } else {
        value = BINARY_ACTIVE;
    }
    Binary_Value_Present_Value_Set(99, value);
    /* uptime */
    Uptime_Seconds += 1;
    hours = (float)Uptime_Seconds / 3600.0f;
    Analog_Value_Present_Value_Set(99, hours, 0);
}

/**
 * Initialize the device's non-volatile data
 */
static void device_nvdata_init(void)
{
    uint8_t value8;
    uint16_t value16;
    uint32_t value32;
    uint8_t encoding;
    uint8_t name_len;
    char name[NV_EEPROM_NAME_SIZE + 1] = "";
    const char *default_name = "AVR Device";

    value16 = nvdata_unsigned16(NV_EEPROM_TYPE_0);
    if (value16 != NV_EEPROM_TYPE_ID) {
        /* set to defaults */
        nvdata_unsigned16_set(NV_EEPROM_TYPE_0, NV_EEPROM_TYPE_ID);
        nvdata_unsigned8_set(NV_EEPROM_VERSION, NV_EEPROM_VERSION_ID);
        nvdata_unsigned8_set(NV_EEPROM_MSTP_MAC, 123);
        nvdata_unsigned8_set(NV_EEPROM_MSTP_BAUD_K, 38);
        nvdata_unsigned8_set(NV_EEPROM_MSTP_MAX_MASTER, 127);
        nvdata_unsigned24_set(NV_EEPROM_DEVICE_0, 260123);
        nvdata_name_set(
            NV_EEPROM_DEVICE_NAME, CHARACTER_ANSI_X34, default_name,
            strlen(default_name));
    }
    value8 = nvdata_unsigned8(NV_EEPROM_MSTP_MAC);
    dlmstp_set_mac_address(value8);
    value8 = nvdata_unsigned8(NV_EEPROM_MSTP_BAUD_K);
    value32 = RS485_Baud_Rate_From_Kilo(value8);
    RS485_Set_Baud_Rate(value32);
    value8 = nvdata_unsigned8(NV_EEPROM_MSTP_MAX_MASTER);
    if (value8 > 127) {
        value8 = 127;
    }
    dlmstp_set_max_master(value8);
    dlmstp_set_max_info_frames(1);
    value32 = nvdata_unsigned24(NV_EEPROM_DEVICE_0);
    Device_Set_Object_Instance_Number(value32);
    name_len =
        nvdata_name(NV_EEPROM_DEVICE_NAME, &encoding, name, sizeof(name) - 1);
    if (name_len) {
        Device_Object_Name_ANSI_Init(name);
    } else {
        Device_Object_Name_ANSI_Init(default_name);
    }
    Send_I_Am_Flag = true;
}

/**
 * Static receive buffer,
 * initialized with zeros by the C Library Startup Code.
 * Note: Added a little safety margin to the buffer,
 * so that in the rare case, the message would be filled
 * up to MAX_MPDU and some decoding functions would overrun,
 * these decoding functions will just end up in a safe field
 * of static zeros. */
static uint8_t PDUBuffer[MAX_MPDU + 16];

/**
 * Main function
 * @return 0, should never reach this
 */
int main(void)
{
    uint16_t pdu_len = 0;
    BACNET_ADDRESS src; /* source address */

    hardware_init();
    Analog_Value_Init();
    Binary_Value_Init();
    device_nvdata_init();
    dlmstp_init(NULL);
    mstimer_set(&Task_Timer, 1000);
    for (;;) {
        /* process */
        if (mstimer_expired(&Task_Timer)) {
            mstimer_reset(&Task_Timer);
            one_second_task();
        }
        /* BACnet handling */
        pdu_len = dlmstp_receive(&src, &PDUBuffer[0], MAX_MPDU, 0);
        if (pdu_len) {
            npdu_handler(&src, &PDUBuffer[0], pdu_len);
        }
    }
}
