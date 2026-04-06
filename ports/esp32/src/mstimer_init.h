/**
 * @file
 * @brief BACnet millisecond timer hook declarations for the PlatformIO ESP32
 * port
 * @author Kato Gangstad
 */

#ifndef M5STAMPLC_MSTIMER_INIT_H
#define M5STAMPLC_MSTIMER_INIT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the system timer services used by BACnet
 */
void systimer_init(void);

/**
 * @brief Get the current millisecond tick count
 * @return milliseconds since startup
 */
unsigned long mstimer_now(void);

#ifdef __cplusplus
}
#endif

#endif
