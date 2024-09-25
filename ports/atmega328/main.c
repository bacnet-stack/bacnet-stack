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
/* MS/TP MAC Address */
static uint8_t Address_Switch;
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
 * Configure the RS485 transceiveer board LED
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
 * Initialize the DIP switch for the MAC address
 *
 * MS/TP MAC Address DIP Switch 0-127
 *
 *      Port  Address
 *      ------ -------
 *      PC0       1
 *      PC1       2
 *      PC2       4
 *      PC3       8
 *      PB0      16
 *      PB1      32
 *      PB2      64
 */
static void input_switch_init(void)
{
    /* Configure the DIP switch pins as inputs */
    BIT_CLEAR(DDRD, DDD4);
    BIT_CLEAR(DDRD, DDD5);
    BIT_CLEAR(DDRD, DDD6);
    BIT_CLEAR(DDRD, DDD7);
    BIT_CLEAR(DDRB, DDB0);
    BIT_CLEAR(DDRB, DDB1);
    BIT_CLEAR(DDRB, DDB2);
    /* activate the internal pull up resistors */
    BIT_SET(PORTD, PORTD0);
    BIT_SET(PORTD, PORTD1);
    BIT_SET(PORTD, PORTD2);
    BIT_SET(PORTD, PORTD3);
    BIT_SET(PORTB, PORTB0);
    BIT_SET(PORTB, PORTB1);
    BIT_SET(PORTB, PORTB2);
}

/**
 * Read the MAC address from the DIP switch.
 */
static uint8_t input_swtich_value(void)
{
    uint8_t value;

    value = BITMASK_CHECK(PIND, 0xF0) >> 4;
    value |= (BITMASK_CHECK(PINB, 0x07) << 4);

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
    input_switch_init();
    adc_init();

    /* Enable global interrupts */
    __enable_interrupt();
}

/**
 * Read the MAC address from the DIP switch.
 */
static void input_switch_read(void)
{
    uint8_t value;
    static uint8_t old_value = 0;

    value = input_swtich_value();
    if (value != old_value) {
        /* simplistic two-cycle debounce */
        old_value = value;
    } else {
        if (old_value != Address_Switch) {
            Address_Switch = old_value;
            dlmstp_set_mac_address(Address_Switch);
            /* optional: set the Device Instance offset using MAC,
               or we could store Device ID in EEPROM */
            Device_Set_Object_Instance_Number(260000 + Address_Switch);
            Send_I_Am_Flag = true;
        }
    }
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
 * @brief process the outputs
 */
static void binary_value_process(void)
{
    BACNET_BINARY_PV value;

    value = Binary_Value_Present_Value(0);
    if (value == BINARY_ACTIVE) {
        value = BINARY_INACTIVE;
    } else {
        value = BINARY_ACTIVE;
    }
    Binary_Value_Present_Value_Set(0, value);
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
    RS485_Set_Baud_Rate(38400UL);
    dlmstp_set_max_master(127);
    dlmstp_set_max_info_frames(1);
    dlmstp_init(NULL);
    Analog_Value_Name_Set(6, "Uptime Seconds");
    Analog_Value_Units_Set(6, UNITS_SECONDS);
    Analog_Value_Name_Set(7, "MCU Frequency");
    Analog_Value_Units_Set(7, UNITS_HERTZ);
    Analog_Value_Present_Value_Set(7, F_CPU, 0);
    mstimer_set(&Task_Timer, 1000);
    for (;;) {
        /* input */
        input_switch_read();
        analog_values_read();
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
