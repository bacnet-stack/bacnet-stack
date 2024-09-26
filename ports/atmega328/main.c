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
static float Uptime_Seconds;

/* dummy function - so we can use default demo handlers */
bool dcc_communication_enabled(void)
{
    return true;
}

/**
 * Configure the RS485 transceiver board LED
 */
static void led_init(void)
{
    /* Configure the LED pin as an output */
    BIT_CLEAR(PORTB, PB5);
    BIT_SET(DDRB, PB5);
    /* Turn off the LED */
    BIT_CLEAR(PORTB, PB5);
}

/**
 * Configure some Arduino digital pins as outputs
 */
static void digital_output_init(void)
{
    /* Configure the D11 pin as an output */
    BIT_CLEAR(PORTB, PB3);
    BIT_SET(DDRB, PB3);
    /* Turn off the D11 */
    BIT_CLEAR(PORTB, PB3);

    /* Configure the D12 pin as an output */
    BIT_CLEAR(PORTB, PB4);
    BIT_SET(DDRB, PB4);
    /* Turn off the D12 */
    BIT_CLEAR(PORTB, PB4);
}

/**
 * Control the Arduino board digital outputs
 */
static void digital_output_set(uint8_t index, bool state)
{
    switch (index) {
        case 11:
            if (state) {
                BIT_SET(PORTB, PB3);
            } else {
                BIT_CLEAR(PORTB, PB3);
            }
            break;
        case 12:
            if (state) {
                BIT_SET(PORTB, PB4);
            } else {
                BIT_CLEAR(PORTB, PB4);
            }
            break;
        default:
            break;
    }
}

/**
 * Configure some Arduino digital pins as inputs
 */
static void digital_input_init(void)
{
    /* Configure the D3 pin as an input */
    BIT_CLEAR(DDRD, DDD3);
}

/**
 * Read the MAC address from the DIP switch.
 */
static bool digital_input_value(uint8_t index)
{
    bool value = false;

    switch (index) {
        case 3:
            value = BIT_CHECK(PIND, PIND3);
            break;
        default:
            break;
    }

    return value;
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
    led_init();
    digital_output_init();
    digital_input_init();
    adc_init();

    /* Enable global interrupts */
    __enable_interrupt();
}

/**
 * Control the RS485 transceiveer board LED
 */
static void led_set(bool state)
{
    if (state) {
        BIT_SET(PORTB, PB5);
    } else {
        BIT_CLEAR(PORTB, PB5);
    }
}

/**
 * @brief process the outputs once per second
 */
static void binary_value_process(void)
{
    BACNET_BINARY_PV value;

    /* LED toggling */
    value = Binary_Value_Present_Value(0);
    if (value == BINARY_ACTIVE) {
        value = BINARY_INACTIVE;
    } else {
        value = BINARY_ACTIVE;
    }
    Binary_Value_Present_Value_Set(0, value);
}

/**
 * @brief read the Arduino Digital inputs
 */
static void digital_input_read(void)
{
    BACNET_BINARY_PV value;

    if (digital_input_value(3)) {
        value = BINARY_ACTIVE;
    } else {
        value = BINARY_INACTIVE;
    }
    Binary_Value_Present_Value_Set(3, value);
}

/**
 * Write to outputs from the Present_Value of the Binary Value.
 */
static void binary_value_write(void)
{
    BACNET_BINARY_PV value;

    value = Binary_Value_Present_Value(0);
    if (value == BINARY_ACTIVE) {
        led_set(true);
    } else {
        led_set(false);
    }
    value = Binary_Value_Present_Value(1);
    if (value == BINARY_ACTIVE) {
        digital_output_set(11, true);
    } else {
        digital_output_set(11, false);
    }
    value = Binary_Value_Present_Value(2);
    if (value == BINARY_ACTIVE) {
        digital_output_set(12, true);
    } else {
        digital_output_set(12, false);
    }
}

/**
 * Read ADC and update the Present_Value of the Analog Value.
 */
static void analog_values_read(void)
{
    static unsigned process_counter = 0;
    float value;

    switch (process_counter) {
        case 0:
            /* initializing */
            adc_enable(0);
            Analog_Value_Name_Set(0, "ADC0");
            Analog_Value_Units_Set(0, UNITS_MILLIVOLTS);
            adc_enable(1);
            Analog_Value_Name_Set(1, "ADC1");
            Analog_Value_Units_Set(1, UNITS_MILLIVOLTS);
            adc_enable(2);
            Analog_Value_Name_Set(2, "ADC2");
            Analog_Value_Units_Set(2, UNITS_MILLIVOLTS);
            adc_enable(3);
            Analog_Value_Name_Set(3, "ADC3");
            Analog_Value_Units_Set(3, UNITS_MILLIVOLTS);
            Analog_Value_Name_Set(4, "CStack Size");
            Analog_Value_Units_Set(4, UNITS_PERCENT);
            Analog_Value_Name_Set(5, "CStack Unused");
            Analog_Value_Units_Set(5, UNITS_PERCENT);
            break;
        case 1:
            value = adc_millivolts(0);
            Analog_Value_Present_Value_Set(0, value, 0);
            process_counter++;
            break;
        case 2:
            value = adc_millivolts(1);
            Analog_Value_Present_Value_Set(1, value, 0);
            process_counter++;
            break;
        case 3:
            value = adc_millivolts(2);
            Analog_Value_Present_Value_Set(2, value, 0);
            process_counter++;
            break;
        case 4:
            value = adc_millivolts(3);
            Analog_Value_Present_Value_Set(3, value, 0);
            process_counter++;
            break;
        case 5:
            value = stack_size();
            Analog_Value_Present_Value_Set(4, value, 0);
            process_counter++;
            break;
        case 6:
            value = stack_unused();
            Analog_Value_Present_Value_Set(5, value, 0);
            process_counter++;
            break;
        default:
            process_counter = 1;
            break;
    }
    value = process_counter;
    Analog_Value_Present_Value_Set(9, value, 0);
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
    const char *default_description = "Uno R3 device with ATmega328";
    const char *default_location = "Location Unknown";

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
        nvdata_name_set(
            NV_EEPROM_DEVICE_DESCRIPTION, CHARACTER_ANSI_X34,
            default_description, strlen(default_description));
        nvdata_name_set(
            NV_EEPROM_DEVICE_LOCATION, CHARACTER_ANSI_X34, default_location,
            strlen(default_location));
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
    name_len = nvdata_name(
        NV_EEPROM_DEVICE_DESCRIPTION, &encoding, name, sizeof(name) - 1);
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
    device_nvdata_init();
    dlmstp_init(NULL);
    Analog_Value_Name_Set(6, "Uptime Seconds");
    Analog_Value_Units_Set(6, UNITS_SECONDS);
    Analog_Value_Name_Set(7, "MCU Frequency");
    Analog_Value_Units_Set(7, UNITS_HERTZ);
    Analog_Value_Present_Value_Set(7, F_CPU, 0);
    mstimer_set(&Task_Timer, 1000);
    for (;;) {
        /* input */
        analog_values_read();
        digital_input_read();
        /* process */
        if (mstimer_expired(&Task_Timer)) {
            mstimer_reset(&Task_Timer);
            Uptime_Seconds += 1.0;
            Analog_Value_Present_Value_Set(6, Uptime_Seconds, 0);
            binary_value_process();
        }
        /* output */
        binary_value_write();
        /* BACnet handling */
        pdu_len = dlmstp_receive(&src, &PDUBuffer[0], MAX_MPDU, 0);
        if (pdu_len) {
            npdu_handler(&src, &PDUBuffer[0], pdu_len);
        }
    }
}
