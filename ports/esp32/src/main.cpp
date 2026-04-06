/**
 * @file
 * @brief BACnet MS/TP application entry point for M5StamPLC
 * @author Kato Gangstad
 */

#include <Arduino.h>
#include <M5StamPLC.h>

#ifndef PLC_INPUT_ACTIVE_LOW
#define PLC_INPUT_ACTIVE_LOW 0
#endif

extern "C" {
#include "bacnet_app.h"
}

/**
 * @brief Initialize the MS/TP application and PLC hardware
 */
void setup(void)
{
    Serial.begin(115200);
    delay(100);
    Serial.println("[BOOT] Mode: BACnet MS/TP");
    Serial.println("[BOOT] Init M5StamPLC...");
    M5StamPLC.begin();
    Serial.println("[BOOT] Init BACnet app...");
    bacnet_app_init();
    Serial.println("[BOOT] BACnet MS/TP ready");
}

/**
 * @brief Run the main MS/TP application loop
 */
void loop(void)
{
    M5StamPLC.update();

    for (uint8_t i = 0; i < bacnet_app_input_count(); i++) {
        bool state = M5StamPLC.readPlcInput(i);
#if PLC_INPUT_ACTIVE_LOW
        state = !state;
#endif
        bacnet_app_input_set(i, state);
    }

    for (uint8_t i = 0; i < bacnet_app_relay_count(); i++) {
        M5StamPLC.writePlcRelay(i, bacnet_app_relay_get(i));
    }

    bacnet_app_temperature_set(M5StamPLC.getTemp());
    bacnet_app_free_heap_kb_set((float)ESP.getFreeHeap() / 1024.0f);

    bacnet_app_tick();
    delay(1);
}
