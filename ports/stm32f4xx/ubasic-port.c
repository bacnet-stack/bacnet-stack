/**
 * @file
 * @brief Example uBASIC-Plus porting layer for the uBASIC-Plus interpreter
 *  for STM32F4xx NUCLIO board
 * @author Steve Karg
 * @date 2025
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <ctype.h>
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/ao.h"
#include "bacnet/basic/object/av.h"
#include "bacnet/basic/object/bi.h"
#include "bacnet/basic/object/bo.h"
#include "bacnet/basic/object/bv.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/ms-input.h"
#include "bacnet/basic/object/mso.h"
#include "bacnet/basic/object/msv.h"
#include "bacnet/basic/program/ubasic/ubasic.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/wp.h"
#include "led.h"

#if defined(UBASIC_SCRIPT_PRINT_TO_SERIAL)
/**
 * @brief Write a buffer to the serial port
 * @param msg Pointer to the buffer to write
 * @param n Number of bytes to write
 */
static void serial_write(const char *msg, uint16_t n)
{
    (void)msg;
    (void)n;
}

/**
 * @brief Gather key presses until new-line is recieved or buffer is full
 * @return return the next byte from the input stream, or EOF(-1) if no byte is
 *  available
 */
static int serial_getc(void)
{
    int ch = -1;

    return ch;
}
#endif

#if defined(UBASIC_SCRIPT_HAVE_HARDWARE_EVENTS)
static uint32_t Event_Mask;

/**
 * @brief Hardware event status bit
 * @param bit Event bit
 * @return 1 if the event is set, 0 otherwise
 */
static int8_t hw_event(uint8_t bit)
{
    if (bit < 32) {
        if (Event_Mask & (1UL << bit)) {
            return 1; // Event is set
        }
    }

    return 0; // Event is not set
}

/**
 * @brief Clear a hardware event state bit
 * @param bit Event bit
 */
static void hw_event_clear(uint8_t bit)
{
    if (bit < 32) {
        Event_Mask &= ~(1UL << bit);
    }
}
#endif

#if defined(UBASIC_SCRIPT_HAVE_STORE_VARS_IN_FLASH)
static uint8_t EEPROM_Buffer[256];

/**
 * @brief Read some data from the EEPROM
 * @param start_address EEPROM starting memory address
 * @param buffer data to store
 * @param length number of bytes of data to read
 */
static size_t
eepromRead(uint16_t start_address, uint8_t *buffer, uint16_t length)
{
    size_t bytes_read = 0;
    uint16_t i = 0;

    for (i = 0; i < length; i++) {
        if (start_address + i < sizeof(EEPROM_Buffer)) {
            buffer[i] = EEPROM_Buffer[start_address + i];
            bytes_read++;
        } else {
            break;
        }
    }

    return bytes_read;
}

/**
 * @brief Write some data to the EEPROM
 * @param start_address EEPROM starting memory address
 * @param buffer data to send
 * @param length number of bytes of data
 */
static size_t
eepromWrite(uint16_t start_address, uint8_t *buffer, uint16_t length)
{
    size_t bytes_written = 0;
    uint16_t i = 0;

    for (i = 0; i < length; i++) {
        if (start_address + i < sizeof(EEPROM_Buffer)) {
            EEPROM_Buffer[start_address + i] = buffer[i];
            bytes_written++;
        } else {
            break;
        }
    }

    return bytes_written;
}

#define UBASIC_FLASH_PAGE_SIZE 256

/**
 * @brief Write a variable to the EEPROM
 * @param Name Variable name
 * @param Vartype Variable type
 * @param datalen_bytes Data length in bytes
 * @param dataptr Pointer to the data
 */
static void variable_write(
    uint8_t Name, uint8_t vartype, uint8_t datalen_bytes, uint8_t *dataptr)
{
    // Calculate the starting address based on variable name
    uint16_t start_address = Name * UBASIC_FLASH_PAGE_SIZE;
    uint8_t buffer[UBASIC_FLASH_PAGE_SIZE];

    // Prepare the buffer with the variable type and data length
    buffer[0] = vartype; // First byte is the variable type
    buffer[1] = datalen_bytes; // Second byte is the data length
    for (uint8_t i = 0; i < datalen_bytes; i++) {
        buffer[i + 2] = dataptr[i]; // Copy the actual data into the buffer
    }

    // Write the buffer to EEPROM
    eepromWrite(start_address, buffer, datalen_bytes + 2);
}

/**
 * @brief Read a variable from the EEPROM
 * @param Name Variable name
 * @param Vartype Variable type
 * @param dataptr Pointer to store the data
 * @param datalen Pointer to store the data length
 */
static void
variable_read(uint8_t Name, uint8_t vartype, uint8_t *dataptr, uint8_t *datalen)
{
    // Calculate the starting address based on variable name
    uint8_t buffer[UBASIC_FLASH_PAGE_SIZE] = { 0 };
    uint16_t start_address = Name * UBASIC_FLASH_PAGE_SIZE;

    // Read the data from EEPROM
    eepromRead(start_address, buffer, UBASIC_FLASH_PAGE_SIZE);
    // Check if the variable type matches
    if (buffer[0] == vartype) {
        *datalen = buffer[1]; // Get the data length
        for (uint8_t i = 0; i < *datalen; i++) {
            dataptr[i] =
                buffer[i + 2]; // Copy the actual data into the provided pointer
        }
    } else {
        *datalen = 0; // If type does not match, set length to 0
    }
}
#endif

#if (                                              \
    defined(UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS) || \
    defined(UBASIC_SCRIPT_HAVE_SLEEP) ||           \
    defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL))
/* defined in mstimer.h */
#endif

#if defined(UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR)
/**
 * @brief Generate a random number
 * @param size Size of the random number in bits
 * @return Random number size-bits wide
 */
static uint32_t random_uint32(uint8_t size)
{
    uint32_t value = 0, grains = 0;
    uint8_t k, i;
    static bool initialized = false;

    if (!initialized) {
        srand(0);
        initialized = true;
    }
    for (k = 0; k < 4; k++) {
        grains = 0;
        for (i = 0; i < (size >> 1); i++) {
            /* Two LS bits are most likely most random */
            grains |= (rand() & 0x00000003) << (2 * i);
        }
        value ^= grains;
    }

    return value;
}
#endif

#if defined(UBASIC_SCRIPT_HAVE_PWM_CHANNELS)
static int32_t dutycycle_pwm_ch[UBASIC_SCRIPT_HAVE_PWM_CHANNELS];

/**
 * @brief Configure the PWM
 * @param psc Prescaler
 * @param per Period
 */
static void pwm_config(uint16_t psc, uint16_t per)
{
    (void)psc;
    (void)per;
}

/**
 * @brief Write a value to the PWM
 * @param ch Channel
 * @param dutycycle Duty cycle
 */
static void pwm_write(uint8_t ch, int32_t dutycycle)
{
    if (ch < UBASIC_SCRIPT_HAVE_PWM_CHANNELS) {
        dutycycle_pwm_ch[ch] = dutycycle;
    }
}

/**
 * @brief Read a value from the PWM
 * @param ch Channel
 * @return Duty cycle
 */
static int32_t pwm_read(uint8_t ch)
{
    if (ch < UBASIC_SCRIPT_HAVE_PWM_CHANNELS) {
        return dutycycle_pwm_ch[ch];
    }
    return 0;
}
#endif

#if defined(UBASIC_SCRIPT_HAVE_ANALOG_READ)
/**
 * @brief Configure the ADC
 * @param sampletime Sample time
 * @param nreads Number of reads
 */
static void adc_config(uint8_t sampletime, uint8_t nreads)
{
    (void)sampletime;
    (void)nreads;
}

/**
 * @brief Read a value from the ADC
 * @param channel Channel
 * @return ADC value
 */
static int32_t adc_read(uint8_t channel)
{
    return (int32_t)random_uint32(12);
}
#endif

#if defined(UBASIC_SCRIPT_HAVE_GPIO_CHANNELS)
/**
 * @brief Configure the GPIO
 * @param ch Channel
 * @param mode Mode
 * @param freq Frequency
 */
static void gpio_config(uint8_t ch, int8_t mode, uint8_t freq)
{
    (void)ch;
    (void)mode;
    (void)freq;
}

/**
 * @brief Write a value to the GPIO
 * @param ch Channel
 * @param pin_state Pin state
 */
static void gpio_write(uint8_t ch, uint8_t pin_state)
{
    switch (ch) {
        case 1:
            if (pin_state) {
                led_on(LED_LD1);
            } else {
                led_off(LED_LD1);
            }
            break;
        case 2:
            if (pin_state) {
                led_on(LED_LD2);
            } else {
                led_off(LED_LD2);
            }
            break;
        default:
            break;
    }
}

/**
 * @brief Read a value from the GPIO
 * @param ch Channel
 * @return GPIO value
 */
static int32_t gpio_read(uint8_t ch)
{
    return 0;
}
#endif

#if defined(UBASIC_SCRIPT_HAVE_BACNET)
/**
 * @brief Create a BACnet object
 * @param object_type Object type
 * @param instance Object instance
 * @param object_name Object name
 */
static void
bacnet_create_object(uint16_t object_type, uint32_t instance, char *object_name)
{
    uint32_t r;

    switch (object_type) {
        case OBJECT_ANALOG_INPUT:
            if (!Analog_Input_Valid_Instance(instance)) {
                r = Analog_Input_Create(instance);
                if (r == instance) {
                    Analog_Input_Name_Set(instance, strdup(object_name));
                }
            }
            break;
        case OBJECT_ANALOG_OUTPUT:
            if (!Analog_Output_Valid_Instance(instance)) {
                r = Analog_Output_Create(instance);
                if (r == instance) {
                    Analog_Output_Name_Set(instance, strdup(object_name));
                }
            }
            break;
        case OBJECT_ANALOG_VALUE:
            if (!Analog_Value_Valid_Instance(instance)) {
                r = Analog_Value_Create(instance);
                if (r == instance) {
                    Analog_Value_Name_Set(instance, strdup(object_name));
                }
            }
            break;
        case OBJECT_BINARY_INPUT:
            if (!Binary_Input_Valid_Instance(instance)) {
                r = Binary_Input_Create(instance);
                if (r == instance) {
                    Binary_Input_Name_Set(instance, strdup(object_name));
                }
            }
            break;
        case OBJECT_BINARY_OUTPUT:
            if (!Binary_Output_Valid_Instance(instance)) {
                r = Binary_Output_Create(instance);
                if (r == instance) {
                    Binary_Output_Name_Set(instance, strdup(object_name));
                }
            }
            break;
        case OBJECT_BINARY_VALUE:
            if (!Binary_Value_Valid_Instance(instance)) {
                r = Binary_Value_Create(instance);
                if (r == instance) {
                    Binary_Value_Name_Set(instance, strdup(object_name));
                }
            }
            break;
        case OBJECT_MULTI_STATE_INPUT:
            if (!Multistate_Input_Valid_Instance(instance)) {
                r = Multistate_Input_Create(instance);
                if (r == instance) {
                    Multistate_Input_Name_Set(instance, strdup(object_name));
                }
            }
            break;
        case OBJECT_MULTI_STATE_OUTPUT:
            if (!Multistate_Output_Valid_Instance(instance)) {
                r = Multistate_Output_Create(instance);
                if (r == instance) {
                    Multistate_Output_Name_Set(instance, strdup(object_name));
                }
            }
            break;
        case OBJECT_MULTI_STATE_VALUE:
            if (!Multistate_Value_Valid_Instance(instance)) {
                r = Multistate_Value_Create(instance);
                if (r == instance) {
                    Multistate_Value_Name_Set(instance, strdup(object_name));
                }
            }
            break;
        default:
            break;
    }
}

/**
 * @brief Write a property to a BACnet object
 * @param object_type Object type
 * @param instance Object instance
 * @param property_id Property ID
 * @param value Property value
 */
static void bacnet_write_property(
    uint16_t object_type,
    uint32_t instance,
    uint32_t property_id,
    VARIABLE_TYPE value)
{
    BACNET_BINARY_PV value_binary = BINARY_INACTIVE;

    switch (object_type) {
        case OBJECT_ANALOG_INPUT:
            if (property_id == PROP_PRESENT_VALUE) {
                Analog_Input_Present_Value_Set(
                    instance, fixedpt_tofloat(value));
            }
            break;
        case OBJECT_ANALOG_OUTPUT:
            if (property_id == PROP_PRESENT_VALUE) {
                Analog_Output_Present_Value_Set(
                    instance, fixedpt_tofloat(value), BACNET_MAX_PRIORITY);
            }
            break;
        case OBJECT_ANALOG_VALUE:
            if (property_id == PROP_PRESENT_VALUE) {
                Analog_Value_Present_Value_Set(
                    instance, fixedpt_tofloat(value), BACNET_MAX_PRIORITY);
            }
            break;
        case OBJECT_BINARY_INPUT:
            if (property_id == PROP_PRESENT_VALUE) {
                if (fixedpt_toint(value) != 0) {
                    value_binary = BINARY_ACTIVE;
                }
                Binary_Input_Present_Value_Set(instance, value_binary);
            }
            break;
        case OBJECT_BINARY_OUTPUT:
            if (property_id == PROP_PRESENT_VALUE) {
                if (fixedpt_toint(value) != 0) {
                    value_binary = BINARY_ACTIVE;
                }
                Binary_Output_Present_Value_Set(
                    instance, value_binary, BACNET_MAX_PRIORITY);
            }
            break;
        case OBJECT_BINARY_VALUE:
            if (property_id == PROP_PRESENT_VALUE) {
                if (fixedpt_toint(value) != 0) {
                    value_binary = BINARY_ACTIVE;
                }
                Binary_Value_Present_Value_Set(instance, value_binary);
            }
            break;
        case OBJECT_MULTI_STATE_INPUT:
            if (property_id == PROP_PRESENT_VALUE) {
                Multistate_Input_Present_Value_Set(
                    instance, fixedpt_toint(value));
            }
            break;
        case OBJECT_MULTI_STATE_OUTPUT:
            if (property_id == PROP_PRESENT_VALUE) {
                Multistate_Output_Present_Value_Set(
                    instance, fixedpt_toint(value), BACNET_MAX_PRIORITY);
            }
            break;
        case OBJECT_MULTI_STATE_VALUE:
            if (property_id == PROP_PRESENT_VALUE) {
                Multistate_Value_Present_Value_Set(
                    instance, fixedpt_toint(value));
            }
            break;
        default:
            break;
    }
}

/**
 * @brief Read a property from a BACnet object
 * @param object_type Object type
 * @param instance Object instance
 * @param property_id Property ID
 * @return Property value
 */
static VARIABLE_TYPE bacnet_read_property(
    uint16_t object_type, uint32_t instance, uint32_t property_id)
{
    VARIABLE_TYPE value = 0;
    float value_float = 0.0;
    BACNET_BINARY_PV value_binary = BINARY_INACTIVE;

    switch (object_type) {
        case OBJECT_ANALOG_INPUT:
            if (property_id == PROP_PRESENT_VALUE) {
                value_float = Analog_Input_Present_Value(instance);
                value = fixedpt_fromfloat(value_float);
            }
            break;
        case OBJECT_ANALOG_OUTPUT:
            if (property_id == PROP_PRESENT_VALUE) {
                value_float = Analog_Output_Present_Value(instance);
                value = fixedpt_fromfloat(value_float);
            }
            break;
        case OBJECT_ANALOG_VALUE:
            if (property_id == PROP_PRESENT_VALUE) {
                value_float = Analog_Value_Present_Value(instance);
                value = fixedpt_fromfloat(value_float);
            }
            break;
        case OBJECT_BINARY_INPUT:
            if (property_id == PROP_PRESENT_VALUE) {
                value_binary = Binary_Input_Present_Value(instance);
                value =
                    fixedpt_fromint((value_binary == BINARY_ACTIVE) ? 1 : 0);
            }
            break;
        case OBJECT_BINARY_OUTPUT:
            if (property_id == PROP_PRESENT_VALUE) {
                value_binary = Binary_Output_Present_Value(instance);
                value =
                    fixedpt_fromint((value_binary == BINARY_ACTIVE) ? 1 : 0);
            }
            break;
        case OBJECT_BINARY_VALUE:
            if (property_id == PROP_PRESENT_VALUE) {
                value_binary = Binary_Value_Present_Value(instance);
                value =
                    fixedpt_fromint((value_binary == BINARY_ACTIVE) ? 1 : 0);
            }
            break;
        case OBJECT_MULTI_STATE_INPUT:
            if (property_id == PROP_PRESENT_VALUE) {
                value =
                    fixedpt_fromint(Multistate_Input_Present_Value(instance));
            }
            break;
        case OBJECT_MULTI_STATE_OUTPUT:
            if (property_id == PROP_PRESENT_VALUE) {
                value =
                    fixedpt_fromint(Multistate_Output_Present_Value(instance));
            }
            break;
        case OBJECT_MULTI_STATE_VALUE:
            if (property_id == PROP_PRESENT_VALUE) {
                value =
                    fixedpt_fromint(Multistate_Value_Present_Value(instance));
            }
            break;
        default:
            break;
    }

    return value;
}
#endif

/**
 * @brief Initialize the hardware drivers
 * @param data Pointer to the ubasic data structure
 */
void ubasic_port_init(struct ubasic_data *data)
{
#if (                                              \
    defined(UBASIC_SCRIPT_HAVE_TICTOC_CHANNELS) || \
    defined(UBASIC_SCRIPT_HAVE_SLEEP) ||           \
    defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL))
    data->mstimer_now = mstimer_now;
#endif
    data->variable_write = variable_write;
    data->variable_read = variable_read;
#if defined(UBASIC_SCRIPT_HAVE_HARDWARE_EVENTS)
    data->hw_event = hw_event;
    data->hw_event_clear = hw_event_clear;
#endif
#if defined(UBASIC_SCRIPT_HAVE_PWM_CHANNELS)
    data->pwm_config = pwm_config;
    data->pwm_write = pwm_write;
    data->pwm_read = pwm_read;
#endif
#if defined(UBASIC_SCRIPT_HAVE_PWM_CHANNELS)
    data->adc_config = adc_config;
    data->adc_read = adc_read;
#endif
#if defined(UBASIC_SCRIPT_HAVE_GPIO_CHANNELS)
    data->gpio_config = gpio_config;
    data->gpio_write = gpio_write;
    data->gpio_read = gpio_read;
#endif
#if defined(UBASIC_SCRIPT_HAVE_RANDOM_NUMBER_GENERATOR)
    data->random_uint32 = random_uint32;
#endif
#if defined(UBASIC_SCRIPT_PRINT_TO_SERIAL)
    data->serial_write = serial_write;
#endif
#if defined(UBASIC_SCRIPT_HAVE_INPUT_FROM_SERIAL)
    data->ubasic_getc = serial_getc;
#endif
#if defined(UBASIC_SCRIPT_HAVE_BACNET)
    data->bacnet_create_object = bacnet_create_object;
    data->bacnet_write_property = bacnet_write_property;
    data->bacnet_read_property = bacnet_read_property;
#endif
}
