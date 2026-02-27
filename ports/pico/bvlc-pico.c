/**
 * @file
 * @authors Miguel Fernandes <miguelandre.fernandes@gmail.com> Testimony Adams <adamstestimony@gmail.com>
 * @date 6 de Jun de 2013
 * @brief BACnet Virtual Link Control for Pico
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "bvlc-pico.h"
#include "bip.h"
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
#define BVLC_INVALID 255

#define BVLC_RESULT_SUCCESSFUL_COMPLETION 0x0000
#define BVLC_RESULT_WRITE_BROADCAST_DISTRIBUTION_TABLE_NAK 0x0010
#define BVLC_RESULT_READ_BROADCAST_DISTRIBUTION_TABLE_NAK 0x0020
#define BVLC_RESULT_REGISTER_FOREIGN_DEVICE_NAK 0X0030
#define BVLC_RESULT_READ_FOREIGN_DEVICE_TABLE_NAK 0x0040
#define BVLC_RESULT_DELETE_FOREIGN_DEVICE_TABLE_ENTRY_NAK 0x0050
#define BVLC_RESULT_DISTRIBUTE_BROADCAST_TO_NETWORK_NAK 0x0060
#define BVLC_RESULT_INVALID 0xFFFF

#define BVLL_TYPE_BACNET_IP (0x81)

/** result from a client request */
BACNET_BVLC_RESULT BVLC_Result_Code = BVLC_RESULT_SUCCESSFUL_COMPLETION;
/** The current BVLC Function Code being handled. */
BACNET_BVLC_FUNCTION BVLC_Function_Code = BVLC_RESULT; /* A safe default */

/** Encode the BVLC Result message
 *
 * @param pdu - buffer to store the encoding
 * @param result_code - BVLC result code
 *
 * @return number of bytes encoded
 */
static int bvlc_encode_bvlc_result(uint8_t *pdu, BACNET_BVLC_RESULT result_code)
{
    if (pdu) {
        pdu[0] = BVLL_TYPE_BACNET_IP;
        pdu[1] = BVLC_RESULT;
        /* The 2-octet BVLC Length field is the length, in octets,
           of the entire BVLL message, including the two octets of the
           length field itself, most significant octet first. */
        encode_unsigned16(&pdu[2], 6);
        encode_unsigned16(&pdu[4], (uint16_t)result_code);
    }

    return 6;
}

/**
 * The common send function for bvlc functions, using b/ip.
 *
 * @param dest_addr - Points to an address containing the
 *  destination address (4 bytes, network byte order)
 * @param dest_port - Destination port number
 * @param mtu - the bytes of data to send
 * @param mtu_len - the number of bytes of data to send
 * @return Upon successful completion, returns the number of bytes sent.
 *  Otherwise, -1 shall be returned to indicate the error.
 */
static int bvlc_send_mpdu(
    const uint8_t *dest_addr,
    const uint16_t *dest_port,
    uint8_t *mtu,
    uint16_t mtu_len)
{
    int bytes_sent = 0;
    
    /* assumes that the driver has already been initialized */
    if (!bip_valid()) {
        return -1;
    }

    /* Send using platform-specific socket function */
    bytes_sent = bip_socket_send(dest_addr, *dest_port, mtu, mtu_len);

    return bytes_sent;
}

/** Sends a BVLC Result
 *
 * @param dest_addr - destination address
 * @param dest_port - destination port
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

    return;
}

/** Note any BVLC_RESULT code, or NAK the BVLL message in the unsupported cases.
 * Use this handler when you are not a BBMD.
 * Sets the BVLC_Function_Code in case it is needed later.
 *
 * @param addr  [in] Socket address to send any NAK back to.
 * @param port  [in] Socket port
 * @param npdu  [in] The received buffer.
 * @param received_bytes [in] How many bytes in npdu[].
 * @return Non-zero BVLC_RESULT_ code if we sent a response (NAK) to this
 *         BVLC message. If zero, may need further processing.
 */
uint16_t bvlc_for_non_bbmd(
    uint8_t *addr, uint16_t *port, uint8_t *npdu, uint16_t received_bytes)
{
    uint16_t result_code = 0; /* aka, BVLC_RESULT_SUCCESSFUL_COMPLETION */

    /* To check the BVLC-function code, the buffer of received
     * bytes has to be at least one byte long. */
    if (received_bytes >= 1) {
        BVLC_Function_Code = npdu[1]; /* The BVLC function */
        switch (BVLC_Function_Code) {
            case BVLC_RESULT:
                if (received_bytes >= 6) {
                    /* This is the result of our foreign device registration */
                    (void)decode_unsigned16(&npdu[4], &result_code);
                    BVLC_Result_Code = (BACNET_BVLC_RESULT)result_code;
                    fprintf(stderr, "BVLC: Result Code=%d\n", BVLC_Result_Code);
                    /* But don't send any response */
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

/** Returns the current BVLL Function Code we are processing.
 * We have to store this higher layer code for when the lower layers
 * need to know what it is, especially to differentiate between
 * BVLC_ORIGINAL_UNICAST_NPDU and BVLC_ORIGINAL_BROADCAST_NPDU.
 *
 * @return A BVLC_ function code, such as BVLC_ORIGINAL_UNICAST_NPDU.
 */
BACNET_BVLC_FUNCTION pico_bvlc_get_function_code(void)
{
    return BVLC_Function_Code;
}
