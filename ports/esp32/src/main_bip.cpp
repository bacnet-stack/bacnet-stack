/**
 * @file
 * @brief BACnet/IP application entry point for ESP32 PlatformIO environments
 * @author Kato Gangstad
 */

#include <Arduino.h>
#include <cstring>
#if !defined(USE_M5STAMPLC_IO) || (USE_M5STAMPLC_IO)
#include <M5StamPLC.h>
#endif
#include <WiFi.h>
#if defined(USE_ETH_INTERFACE) && (USE_ETH_INTERFACE)
#include <ETH.h>
#endif

#ifndef PLC_INPUT_ACTIVE_LOW
#define PLC_INPUT_ACTIVE_LOW 0
#endif

#ifndef USE_M5STAMPLC_IO
#define USE_M5STAMPLC_IO 1
#endif

#ifndef USE_ETH_INTERFACE
#define USE_ETH_INTERFACE 0
#endif

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASS
#define WIFI_PASS ""
#endif

#ifndef BACNET_IP_PORT
#define BACNET_IP_PORT 47808
#endif

extern "C" {
#include "bacnet_app.h"
}

/**
 * @brief Bring up the selected WiFi or Ethernet interface
 */
static void wifi_connect(void)
{
#if USE_ETH_INTERFACE
    Serial.println("[ETH] Starting Ethernet...");
    if (!ETH.begin()) {
        Serial.println("[ETH] ETH.begin() failed");
        return;
    }

    unsigned long start = millis();
    while (!ETH.linkUp() && ((millis() - start) < 15000UL)) {
        delay(200);
        Serial.print('.');
    }
    Serial.println();

    if (ETH.linkUp()) {
        Serial.printf(
            "[ETH] Link up. IP: %s\n", ETH.localIP().toString().c_str());
    } else {
        Serial.println("[ETH] Link timeout");
    }
#else
    if (strlen(WIFI_SSID) == 0) {
        Serial.println("[WIFI] SSID not configured; skipping WiFi connect");
        return;
    }

    Serial.printf("[WIFI] Connecting to SSID: %s\n", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    unsigned long start = millis();
    while ((WiFi.status() != WL_CONNECTED) && ((millis() - start) < 15000UL)) {
        delay(200);
        Serial.print('.');
    }

    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf(
            "[WIFI] Connected. IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.printf(
            "[WIFI] Connect timeout. status=%d\n", (int)WiFi.status());
    }
#endif
}

/**
 * @brief Initialize the BACnet/IP application and selected board profile
 */
void setup(void)
{
    Serial.begin(115200);
    delay(100);
    Serial.println("[BOOT] Mode: BACnet/IP");
    Serial.printf("[BOOT] BACnet UDP Port: %u\n", (unsigned)BACNET_IP_PORT);
#if USE_ETH_INTERFACE
    Serial.println("[BOOT] Network: Ethernet");
#else
    Serial.println("[BOOT] Network: WiFi");
#endif
#if USE_M5STAMPLC_IO
    Serial.println("[BOOT] Board profile: M5StamPLC I/O enabled");
    M5StamPLC.begin();
#else
    Serial.println("[BOOT] Board profile: Generic/ESP32-PoE (M5 I/O disabled)");
#endif
    wifi_connect();
    Serial.println("[BOOT] Init BACnet app...");
    bacnet_app_init();
    Serial.println("[BOOT] BACnet/IP ready");
}

/**
 * @brief Run the main BACnet/IP application loop
 */
void loop(void)
{
#if USE_M5STAMPLC_IO
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
#else
    bacnet_app_temperature_set(0.0f);
    bacnet_app_free_heap_kb_set((float)ESP.getFreeHeap() / 1024.0f);
#endif

    bacnet_app_tick();
    delay(1);
}
