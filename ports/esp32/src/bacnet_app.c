/**
 * @file
 * @brief BACnet application glue for the PlatformIO ESP32 port
 * @author Kato Gangstad
 */

#include <stdbool.h>
#include <stdint.h>

#include "bacnet_app.h"
#include "mstimer_init.h"

#if defined(BACDL_MSTP)
#include "dlenv.h"
#include "bacnet/datalink/datalink.h"
#elif defined(BACDL_BIP)
#include "bip.h"
#endif

#include "bacnet/basic/binding/address.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/bi.h"
#include "bacnet/basic/object/bo.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/npdu.h"

#ifndef BACNET_IP_PORT
#define BACNET_IP_PORT 47808
#endif

#if defined(BACDL_BIP)
static uint8_t PDUBuffer[BIP_MPDU_MAX];
#else
static uint8_t PDUBuffer[MAX_MPDU + 16];
#endif

#define PLC_INPUT_COUNT 8
#define PLC_RELAY_COUNT 4
#define PLC_ANALOG_COUNT 2

#define PLC_AI_INSTANCE_TEMPERATURE 100
#define PLC_AI_INSTANCE_FREE_HEAP_KB 101

static uint32_t Plc_Input_Instances[PLC_INPUT_COUNT];
static uint32_t Plc_Relay_Instances[PLC_RELAY_COUNT];
static uint32_t Plc_Analog_Instances[PLC_ANALOG_COUNT];

static const char *const Plc_Input_Names[PLC_INPUT_COUNT] = {
    "PLC Input 1", "PLC Input 2", "PLC Input 3", "PLC Input 4",
    "PLC Input 5", "PLC Input 6", "PLC Input 7", "PLC Input 8",
};

static const char *const Plc_Input_Descriptions[PLC_INPUT_COUNT] = {
    "PLC Input 1", "PLC Input 2", "PLC Input 3", "PLC Input 4",
    "PLC Input 5", "PLC Input 6", "PLC Input 7", "PLC Input 8",
};

static const char *const Plc_Relay_Names[PLC_RELAY_COUNT] = {
    "PLC Relay 1",
    "PLC Relay 2",
    "PLC Relay 3",
    "PLC Relay 4",
};

static const char *const Plc_Relay_Descriptions[PLC_RELAY_COUNT] = {
    "PLC Relay 1",
    "PLC Relay 2",
    "PLC Relay 3",
    "PLC Relay 4",
};

static const char *const Plc_Analog_Names[PLC_ANALOG_COUNT] = {
    "PLC Temperature",
    "Free Heap KB",
};

static const char *const Plc_Analog_Descriptions[PLC_ANALOG_COUNT] = {
    "Internal PLC temperature in C",
    "Free heap memory in kilobytes",
};

static object_functions_t My_Object_Table[] = {
    { OBJECT_DEVICE,
      NULL,
      Device_Count,
      Device_Index_To_Instance,
      Device_Valid_Object_Instance_Number,
      Device_Object_Name,
      Device_Read_Property_Local,
      Device_Write_Property_Local,
      Device_Property_Lists,
      DeviceGetRRInfo,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      Device_Writable_Property_List },

    { OBJECT_BINARY_INPUT,
      Binary_Input_Init,
      Binary_Input_Count,
      Binary_Input_Index_To_Instance,
      Binary_Input_Valid_Instance,
      Binary_Input_Object_Name,
      Binary_Input_Read_Property,
      NULL,
      Binary_Input_Property_Lists,
      NULL,
      NULL,
      Binary_Input_Encode_Value_List,
      Binary_Input_Change_Of_Value,
      Binary_Input_Change_Of_Value_Clear,
      NULL,
      NULL,
      NULL,
      Binary_Input_Create,
      Binary_Input_Delete,
      NULL,
      Binary_Input_Writable_Property_List },

    { OBJECT_ANALOG_INPUT,
      Analog_Input_Init,
      Analog_Input_Count,
      Analog_Input_Index_To_Instance,
      Analog_Input_Valid_Instance,
      Analog_Input_Object_Name,
      Analog_Input_Read_Property,
      Analog_Input_Write_Property,
      Analog_Input_Property_Lists,
      NULL,
      NULL,
      Analog_Input_Encode_Value_List,
      Analog_Input_Change_Of_Value,
      Analog_Input_Change_Of_Value_Clear,
      Analog_Input_Intrinsic_Reporting,
      NULL,
      NULL,
      Analog_Input_Create,
      Analog_Input_Delete,
      NULL,
      Analog_Input_Writable_Property_List },

    { OBJECT_BINARY_OUTPUT,
      Binary_Output_Init,
      Binary_Output_Count,
      Binary_Output_Index_To_Instance,
      Binary_Output_Valid_Instance,
      Binary_Output_Object_Name,
      Binary_Output_Read_Property,
      Binary_Output_Write_Property,
      Binary_Output_Property_Lists,
      NULL,
      NULL,
      Binary_Output_Encode_Value_List,
      Binary_Output_Change_Of_Value,
      Binary_Output_Change_Of_Value_Clear,
      NULL,
      NULL,
      NULL,
      Binary_Output_Create,
      Binary_Output_Delete,
      NULL,
      Binary_Output_Writable_Property_List },

    { MAX_BACNET_OBJECT_TYPE,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL }
};

/**
 * @brief Create and initialize the BACnet objects exposed by this port
 */
static void init_server_objects(void)
{
    uint32_t instance;

    Device_Init(My_Object_Table);

    for (uint8_t i = 0; i < PLC_INPUT_COUNT; i++) {
        instance = Binary_Input_Create(i);
        Plc_Input_Instances[i] = instance;
        (void)Binary_Input_Name_Set(instance, Plc_Input_Names[i]);
        (void)Binary_Input_Description_Set(instance, Plc_Input_Descriptions[i]);
    }

    for (uint8_t i = 0; i < PLC_RELAY_COUNT; i++) {
        instance = Binary_Output_Create(i);
        Plc_Relay_Instances[i] = instance;
        (void)Binary_Output_Name_Set(instance, Plc_Relay_Names[i]);
        (void)Binary_Output_Description_Set(
            instance, Plc_Relay_Descriptions[i]);
        (void)Binary_Output_Present_Value_Set(
            instance, BINARY_INACTIVE, BACNET_MAX_PRIORITY);
    }

    Plc_Analog_Instances[0] = Analog_Input_Create(PLC_AI_INSTANCE_TEMPERATURE);
    Plc_Analog_Instances[1] = Analog_Input_Create(PLC_AI_INSTANCE_FREE_HEAP_KB);

    (void)Analog_Input_Name_Set(Plc_Analog_Instances[0], Plc_Analog_Names[0]);
    (void)Analog_Input_Description_Set(
        Plc_Analog_Instances[0], Plc_Analog_Descriptions[0]);
    (void)Analog_Input_Units_Set(
        Plc_Analog_Instances[0], UNITS_DEGREES_CELSIUS);

    (void)Analog_Input_Name_Set(Plc_Analog_Instances[1], Plc_Analog_Names[1]);
    (void)Analog_Input_Description_Set(
        Plc_Analog_Instances[1], Plc_Analog_Descriptions[1]);
    (void)Analog_Input_Units_Set(Plc_Analog_Instances[1], UNITS_NO_UNITS);
}

/**
 * @brief Initialize the BACnet application and selected datalink
 */
void bacnet_app_init(void)
{
    systimer_init();

    address_init();
    Device_Set_Object_Instance_Number(12345);
    init_server_objects();

#if defined(BACDL_MSTP)
    if (!m5_dlenv_init(2)) {
        for (;;) { }
    }
#elif defined(BACDL_BIP)
    if (!bip_init((uint16_t)BACNET_IP_PORT)) {
        for (;;) { }
    }
#endif

    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, handler_write_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);

    Send_I_Am(&Handler_Transmit_Buffer[0]);
}

/**
 * @brief Process one BACnet polling cycle and advance timers
 */
void bacnet_app_tick(void)
{
    BACNET_ADDRESS src = { 0 };
    uint16_t pdu_len = 0;

#if defined(BACDL_BIP)
    pdu_len = bip_receive(&src, &PDUBuffer[0], sizeof(PDUBuffer), 0);
#else
    pdu_len = datalink_receive(&src, &PDUBuffer[0], MAX_MPDU, 0);
#endif
    if (pdu_len) {
        npdu_handler(&src, &PDUBuffer[0], pdu_len);
    }

    tsm_timer_milliseconds(1);
}

/**
 * @brief Get the number of exposed PLC binary inputs
 * @return number of binary inputs
 */
uint8_t bacnet_app_input_count(void)
{
    return PLC_INPUT_COUNT;
}

/**
 * @brief Get the number of exposed PLC relay outputs
 * @return number of binary outputs
 */
uint8_t bacnet_app_relay_count(void)
{
    return PLC_RELAY_COUNT;
}

/**
 * @brief Update a BACnet binary input from PLC state
 * @param index zero-based PLC input index
 * @param active input state to publish
 */
void bacnet_app_input_set(uint8_t index, bool active)
{
    uint32_t instance;

    if (index >= PLC_INPUT_COUNT) {
        return;
    }

    instance = Plc_Input_Instances[index];

    (void)Binary_Input_Present_Value_Set(
        instance, active ? BINARY_ACTIVE : BINARY_INACTIVE);
}

/**
 * @brief Read the commanded state of a BACnet relay output
 * @param index zero-based relay index
 * @return true if the relay should be active
 */
bool bacnet_app_relay_get(uint8_t index)
{
    uint32_t instance;

    if (index >= PLC_RELAY_COUNT) {
        return false;
    }

    instance = Plc_Relay_Instances[index];

    return (Binary_Output_Present_Value(instance) == BINARY_ACTIVE);
}

/**
 * @brief Update the analog input that reports PLC temperature
 * @param value_celsius temperature in degrees Celsius
 */
void bacnet_app_temperature_set(float value_celsius)
{
    Analog_Input_Present_Value_Set(Plc_Analog_Instances[0], value_celsius);
}

/**
 * @brief Update the analog input that reports free heap memory
 * @param value_kb free heap in kilobytes
 */
void bacnet_app_free_heap_kb_set(float value_kb)
{
    Analog_Input_Present_Value_Set(Plc_Analog_Instances[1], value_kb);
}
