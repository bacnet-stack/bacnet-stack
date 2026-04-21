/**
 * @file
 * @brief BACnet router entry point bridging BACnet/IP and BACnet MS/TP
 * @author Kato Gangstad
 */

#include <Arduino.h>
#include <cstring>
#include <M5StamPLC.h>
#include <WiFi.h>

extern "C" {
#include "bacnet_app.h"
#include "bip.h"
#include "dlenv.h"
#include "bacnet/bacdcode.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/datalink/dlmstp.h"
#include "bacnet/basic/npdu/h_npdu.h"
#include "bacnet/basic/tsm/tsm.h"
}

#ifndef WIFI_SSID
#define WIFI_SSID ""
#endif

#ifndef WIFI_PASS
#define WIFI_PASS ""
#endif

#ifndef BACNET_IP_PORT
#define BACNET_IP_PORT 47808
#endif

#ifndef ROUTER_BIP_NET
#define ROUTER_BIP_NET 1
#endif

#ifndef ROUTER_MSTP_NET
#define ROUTER_MSTP_NET 200
#endif

#ifndef ROUTER_MSTP_MAC
#define ROUTER_MSTP_MAC 2
#endif

#ifndef PLC_INPUT_ACTIVE_LOW
#define PLC_INPUT_ACTIVE_LOW 0
#endif

#ifndef GATEWAY_MAX
#define GATEWAY_MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

static uint8_t BipRxBuffer[BIP_MPDU_MAX];
static uint8_t MstpRxBuffer[DLMSTP_MPDU_MAX];
static uint8_t TxBuffer[GATEWAY_MAX(BIP_MPDU_MAX, DLMSTP_MPDU_MAX)];

/**
 * @brief Connect the BACnet/IP side of the router to WiFi
 */
static void wifi_connect(void)
{
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
}

/**
 * @brief Send a BACnet router network message on the MS/TP segment
 * @param type network layer message type
 * @param dnet destination network number, or negative when omitted
 * @return number of bytes sent, or a negative value on error
 */
static int
send_router_message_on_mstp(BACNET_NETWORK_MESSAGE_TYPE type, int dnet)
{
    BACNET_NPDU_DATA npdu_data = { 0 };
    BACNET_ADDRESS dest = { 0 };
    int pdu_len = 0;

    dest.mac_len = 0;
    dest.net = BACNET_BROADCAST_NETWORK;
    dest.len = 0;

    npdu_encode_npdu_network(&npdu_data, type, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(TxBuffer, &dest, NULL, &npdu_data);
    if (dnet >= 0) {
        pdu_len += encode_unsigned16(&TxBuffer[pdu_len], (uint16_t)dnet);
    }

    return dlmstp_send_pdu(&dest, &npdu_data, TxBuffer, (unsigned)pdu_len);
}

/**
 * @brief Send a BACnet router network message on the BACnet/IP segment
 * @param type network layer message type
 * @param dnet destination network number, or negative when omitted
 * @return number of bytes sent, or a negative value on error
 */
static int
send_router_message_on_bip(BACNET_NETWORK_MESSAGE_TYPE type, int dnet)
{
    BACNET_NPDU_DATA npdu_data = { 0 };
    BACNET_ADDRESS dest = { 0 };
    int pdu_len = 0;

    bip_get_broadcast_address(&dest);

    npdu_encode_npdu_network(&npdu_data, type, false, MESSAGE_PRIORITY_NORMAL);
    pdu_len = npdu_encode_pdu(TxBuffer, &dest, NULL, &npdu_data);
    if (dnet >= 0) {
        pdu_len += encode_unsigned16(&TxBuffer[pdu_len], (uint16_t)dnet);
    }

    return bip_send_pdu(&dest, &npdu_data, TxBuffer, (unsigned)pdu_len);
}

/**
 * @brief Check whether a received NPDU should be processed by the local device
 * @param dest decoded destination address
 * @return true if the packet targets the local device or broadcast
 */
static bool should_process_locally(const BACNET_ADDRESS *dest)
{
    return (dest->net == 0U) || (dest->net == BACNET_BROADCAST_NETWORK);
}

/**
 * @brief Route an NPDU between BACnet/IP and MS/TP segments
 * @param from_bip true when the frame arrived from BACnet/IP
 * @param src decoded source address
 * @param pdu NPDU buffer to route
 * @param pdu_len NPDU length in bytes
 */
static void
forward_pdu(bool from_bip, BACNET_ADDRESS *src, uint8_t *pdu, uint16_t pdu_len)
{
    BACNET_ADDRESS dest = { 0 };
    BACNET_ADDRESS routed_src = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    int apdu_offset = bacnet_npdu_decode(pdu, pdu_len, &dest, src, &npdu_data);
    if (apdu_offset <= 0) {
        return;
    }

    const uint16_t src_net = from_bip ? ROUTER_BIP_NET : ROUTER_MSTP_NET;
    const uint16_t dst_net = from_bip ? ROUTER_MSTP_NET : ROUTER_BIP_NET;

    if (npdu_data.network_layer_message) {
        if (npdu_data.network_message_type ==
            NETWORK_MESSAGE_WHO_IS_ROUTER_TO_NETWORK) {
            uint16_t requested = 0;
            bool has_dnet = (pdu_len - apdu_offset) >= 2;
            if (has_dnet) {
                (void)decode_unsigned16(&pdu[apdu_offset], &requested);
            }
            if (!has_dnet || (requested == dst_net)) {
                if (from_bip) {
                    send_router_message_on_bip(
                        NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK, dst_net);
                } else {
                    send_router_message_on_mstp(
                        NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK, dst_net);
                }
            }
        }
        return;
    }

    if ((dest.net != 0U) && (dest.net != BACNET_BROADCAST_NETWORK) &&
        (dest.net != dst_net)) {
        return;
    }

    if (npdu_data.hop_count > 0) {
        npdu_data.hop_count--;
        if (npdu_data.hop_count == 0) {
            return;
        }
    }

    if (src->net != 0U) {
        routed_src = *src;
    } else {
        memset(&routed_src, 0, sizeof(routed_src));
        routed_src.net = src_net;
        routed_src.len = src->mac_len;
        for (uint8_t i = 0; i < src->mac_len && i < MAX_MAC_LEN; i++) {
            routed_src.adr[i] = src->mac[i];
        }
    }

    BACNET_ADDRESS out_dest = { 0 };
    if (dest.net == BACNET_BROADCAST_NETWORK ||
        ((dest.net == dst_net) && (dest.len == 0))) {
        out_dest.mac_len = 0;
        out_dest.net = BACNET_BROADCAST_NETWORK;
        out_dest.len = 0;
    } else if ((dest.net == dst_net) && (dest.len > 0)) {
        out_dest.mac_len = dest.len;
        for (uint8_t i = 0; i < dest.len && i < MAX_MAC_LEN; i++) {
            out_dest.mac[i] = dest.adr[i];
        }
        out_dest.net = 0;
        out_dest.len = 0;
    } else {
        out_dest = dest;
    }

    int out_len = npdu_encode_pdu(TxBuffer, &out_dest, &routed_src, &npdu_data);
    if (out_len <= 0) {
        return;
    }

    const uint16_t apdu_len = (uint16_t)(pdu_len - apdu_offset);
    if ((out_len + (int)apdu_len) > (int)sizeof(TxBuffer)) {
        return;
    }
    memcpy(&TxBuffer[out_len], &pdu[apdu_offset], apdu_len);
    out_len += apdu_len;

    if (from_bip) {
        (void)dlmstp_send_pdu(
            &out_dest, &npdu_data, TxBuffer, (unsigned)out_len);
    } else {
        (void)bip_send_pdu(&out_dest, &npdu_data, TxBuffer, (unsigned)out_len);
    }
}

/**
 * @brief Initialize the BACnet router and local device services
 */
void setup(void)
{
    Serial.begin(115200);
    delay(100);
    Serial.println("[BOOT] Mode: BACnet Gateway (Router + Local Device)");
    Serial.printf(
        "[GATEWAY] BIP NET=%u MSTP NET=%u MSTP MAC=%u\n",
        (unsigned)ROUTER_BIP_NET, (unsigned)ROUTER_MSTP_NET,
        (unsigned)ROUTER_MSTP_MAC);

    M5StamPLC.begin();
    wifi_connect();

    /* Local device + objects run on B/IP side (same logic as existing M5 app).
     */
    bacnet_app_init();

    if (!m5_dlenv_init((uint8_t)ROUTER_MSTP_MAC)) {
        Serial.println("[GATEWAY] ERROR: MSTP init failed");
        for (;;) {
            delay(1000);
        }
    }

    (void)send_router_message_on_bip(
        NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK, ROUTER_MSTP_NET);
    (void)send_router_message_on_mstp(
        NETWORK_MESSAGE_I_AM_ROUTER_TO_NETWORK, ROUTER_BIP_NET);

    Serial.println("[GATEWAY] Ready");
}

/**
 * @brief Run the BACnet router main loop
 */
void loop(void)
{
    BACNET_ADDRESS src = { 0 };
    BACNET_ADDRESS dest = { 0 };
    BACNET_NPDU_DATA npdu_data = { 0 };
    uint16_t pdu_len = 0;

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

    pdu_len = bip_receive(&src, BipRxBuffer, sizeof(BipRxBuffer), 0);
    if (pdu_len > 0) {
        int apdu_offset =
            bacnet_npdu_decode(BipRxBuffer, pdu_len, &dest, &src, &npdu_data);
        if ((apdu_offset > 0) && should_process_locally(&dest)) {
            npdu_handler(&src, BipRxBuffer, pdu_len);
        }
        forward_pdu(true, &src, BipRxBuffer, pdu_len);
    }

    pdu_len = dlmstp_receive(&src, MstpRxBuffer, sizeof(MstpRxBuffer), 0);
    if (pdu_len > 0) {
        forward_pdu(false, &src, MstpRxBuffer, pdu_len);
    }

    tsm_timer_milliseconds(1);

    delay(1);
}
