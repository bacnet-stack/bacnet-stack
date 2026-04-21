/**
 * @file
 * @brief BACnet Virtual Link Control implementation for the PlatformIO ESP32
 * port
 * @author Kato Gangstad
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "bip.h"
#include "bvlc.h"
#include "bacnet/bacint.h"

#define BVLC_RESULT 0
#define BVLC_WRITE_BROADCAST_DISTRIBUTION_TABLE 1
#define BVLC_READ_BROADCAST_DIST_TABLE 2
#define BVLC_READ_BROADCAST_DIST_TABLE_ACK 3
#define BVLC_FORWARDED_NPDU 4
#define BVLC_REGISTER_FOREIGN_DEVICE 5
#define BVLC_READ_FOREIGN_DEVICE_TABLE 6
#define BVLC_READ_FOREIGN_DEVICE_TABLE_ACK 7
#define BVLC_DELETE_FOREIGN_DEVICE_TABLE_ENTRY 8
#define BVLC_DISTRIBUTE_BROADCAST_TO_NETWORK 9
#define BVLC_ORIGINAL_UNICAST_NPDU 10
#define BVLC_ORIGINAL_BROADCAST_NPDU 11
#define BVLC_SECURE_BVLL 12

#define BVLC_RESULT_WRITE_BROADCAST_DISTRIBUTION_TABLE_NAK 0x0010
#define BVLC_RESULT_READ_BROADCAST_DISTRIBUTION_TABLE_NAK 0x0020
#define BVLC_RESULT_REGISTER_FOREIGN_DEVICE_NAK 0X0030
#define BVLC_RESULT_READ_FOREIGN_DEVICE_TABLE_NAK 0x0040
#define BVLC_RESULT_DELETE_FOREIGN_DEVICE_TABLE_ENTRY_NAK 0x0050
#define BVLC_RESULT_DISTRIBUTE_BROADCAST_TO_NETWORK_NAK 0x0060

static BACNET_BVLC_RESULT BVLC_Result_Code = BVLC_RESULT_SUCCESSFUL_COMPLETION;
static BACNET_BVLC_FUNCTION BVLC_Function_Code = BVLC_RESULT;

/**
 * @brief Encode a BVLC result message
 * @param pdu destination buffer, or NULL for length-only use
 * @param result_code BVLC result code to encode
 * @return encoded BVLC message length
 */
static int bvlc_encode_bvlc_result(uint8_t *pdu, BACNET_BVLC_RESULT result_code)
{
    if (pdu) {
        pdu[0] = BVLL_TYPE_BACNET_IP;
        pdu[1] = BVLC_RESULT;
        encode_unsigned16(&pdu[2], 6);
        encode_unsigned16(&pdu[4], (uint16_t)result_code);
    }

    return 6;
}

/**
 * @brief Send a raw BVLC frame to the destination host
 * @param dest_addr destination IPv4 address
 * @param dest_port destination UDP port
 * @param mtu raw BVLC frame buffer
 * @param mtu_len frame length in bytes
 * @return number of bytes sent, or -1 on error
 */
static int bvlc_send_mpdu(
    const uint8_t *dest_addr,
    const uint16_t *dest_port,
    uint8_t *mtu,
    uint16_t mtu_len)
{
    int bytes_sent = 0;

    if (!bip_valid()) {
        return -1;
    }

    bytes_sent = bip_socket_send(dest_addr, *dest_port, mtu, mtu_len);

    return bytes_sent;
}

/**
 * @brief Send a BVLC result response
 * @param dest_addr destination IPv4 address
 * @param dest_port destination UDP port
 * @param result_code BVLC result code to report
 */
static void bvlc_send_result(
    const uint8_t *dest_addr,
    const uint16_t *dest_port,
    BACNET_BVLC_RESULT result_code)
{
    uint8_t mtu[BIP_MPDU_MAX] = { 0 };
    uint16_t mtu_len = 0;

    mtu_len = (uint16_t)bvlc_encode_bvlc_result(&mtu[0], result_code);
    bvlc_send_mpdu(dest_addr, dest_port, mtu, mtu_len);
}

/**
 * @brief Handle non-BBMD BVLC functions for this port
 * @param addr source IPv4 address
 * @param port source UDP port
 * @param npdu received BVLC frame buffer
 * @param received_bytes frame length in bytes
 * @return non-zero when the frame was consumed by BVLC handling
 */
uint16_t bvlc_for_non_bbmd(
    uint8_t *addr, uint16_t *port, uint8_t *npdu, uint16_t received_bytes)
{
    uint16_t result_code = 0;

    if (received_bytes >= 1) {
        BVLC_Function_Code = npdu[1];
        switch (BVLC_Function_Code) {
            case BVLC_RESULT:
                if (received_bytes >= 6) {
                    (void)decode_unsigned16(&npdu[4], &result_code);
                    BVLC_Result_Code = (BACNET_BVLC_RESULT)result_code;
                    fprintf(stderr, "BVLC: Result Code=%d\n", BVLC_Result_Code);
                    result_code = 0;
                }
                break;
            case BVLC_WRITE_BROADCAST_DISTRIBUTION_TABLE:
                result_code =
                    BVLC_RESULT_WRITE_BROADCAST_DISTRIBUTION_TABLE_NAK;
                break;
            case BVLC_READ_BROADCAST_DIST_TABLE:
                result_code = BVLC_RESULT_READ_BROADCAST_DISTRIBUTION_TABLE_NAK;
                break;
            case BVLC_REGISTER_FOREIGN_DEVICE:
                result_code = BVLC_RESULT_REGISTER_FOREIGN_DEVICE_NAK;
                break;
            case BVLC_READ_FOREIGN_DEVICE_TABLE:
                result_code = BVLC_RESULT_READ_FOREIGN_DEVICE_TABLE_NAK;
                break;
            case BVLC_DELETE_FOREIGN_DEVICE_TABLE_ENTRY:
                result_code = BVLC_RESULT_DELETE_FOREIGN_DEVICE_TABLE_ENTRY_NAK;
                break;
            case BVLC_DISTRIBUTE_BROADCAST_TO_NETWORK:
                result_code = BVLC_RESULT_DISTRIBUTE_BROADCAST_TO_NETWORK_NAK;
                break;
            default:
                break;
        }
    }

    if (result_code > 0) {
        bvlc_send_result(addr, port, result_code);
        fprintf(stderr, "BVLC: NAK code=%d\n", result_code);
    }

    return result_code;
}

/**
 * @brief Get the last decoded BVLC function code
 * @return BVLC function code
 */
BACNET_BVLC_FUNCTION pico_bvlc_get_function_code(void)
{
    return BVLC_Function_Code;
}
