/**
 * @file
 * @brief BACnet millisecond timer hooks for the PlatformIO ESP32 port
 * @author Kato Gangstad
 */

#include "mstimer_init.h"

#include "esp_timer.h"

/**
 * @brief Initialize the system timer services used by BACnet
 */
void systimer_init(void)
{
    /* esp_timer is available after runtime startup on ESP32. */
}

/**
 * @brief Get the current millisecond tick count
 * @return milliseconds since startup
 */
unsigned long mstimer_now(void)
{
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}
