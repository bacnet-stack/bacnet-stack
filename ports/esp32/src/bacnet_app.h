/**
 * @file
 * @brief Public BACnet application hooks for the PlatformIO ESP32 port
 * @author Kato Gangstad
 */

#ifndef M5STAMPLC_BACNET_APP_H
#define M5STAMPLC_BACNET_APP_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the BACnet application and datalink bindings
 */
void bacnet_app_init(void);

/**
 * @brief Run one BACnet processing cycle
 */
void bacnet_app_tick(void);

/**
 * @brief Get the number of published PLC inputs
 * @return number of PLC inputs
 */
uint8_t bacnet_app_input_count(void);

/**
 * @brief Get the number of published PLC relays
 * @return number of PLC relays
 */
uint8_t bacnet_app_relay_count(void);

/**
 * @brief Push a PLC input state into the BACnet object model
 * @param index zero-based input index
 * @param active input state
 */
void bacnet_app_input_set(uint8_t index, bool active);

/**
 * @brief Read the requested state for a PLC relay
 * @param index zero-based relay index
 * @return true if the relay should be active
 */
bool bacnet_app_relay_get(uint8_t index);

/**
 * @brief Update the published PLC temperature value
 * @param value_celsius temperature in degrees Celsius
 */
void bacnet_app_temperature_set(float value_celsius);

/**
 * @brief Update the published free heap value
 * @param value_kb free heap in kilobytes
 */
void bacnet_app_free_heap_kb_set(float value_kb);

#ifdef __cplusplus
}
#endif

#endif
