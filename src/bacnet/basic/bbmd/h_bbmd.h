/**
 * @file
 * @author Steve Karg
 * @date February 2020
 * @brief Header file for a basic BBMD for BVLC IPv4 handler
 *
 * @section LICENSE
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef BVLC_HANDLER_H
#define BVLC_HANDLER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdint.h>
#include "bacnet/bacnet_stack_exports.h"
#include "bacnet/bacdef.h"
#include "bacnet/datalink/bvlc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* user application function prototypes */
BACNET_STACK_EXPORT
int bvlc_handler(BACNET_IP_ADDRESS *addr,
    BACNET_ADDRESS *src,
    uint8_t *npdu,
    uint16_t npdu_len);

BACNET_STACK_EXPORT
int bvlc_broadcast_handler(BACNET_IP_ADDRESS *addr,
    BACNET_ADDRESS *src,
    uint8_t *npdu,
    uint16_t npdu_len);

BACNET_STACK_EXPORT
int bvlc_bbmd_enabled_handler(BACNET_IP_ADDRESS *addr,
    BACNET_ADDRESS *src,
    uint8_t *mtu,
    uint16_t mtu_len);

BACNET_STACK_EXPORT
int bvlc_bbmd_disabled_handler(BACNET_IP_ADDRESS *addr,
    BACNET_ADDRESS *src,
    uint8_t *mtu,
    uint16_t mtu_len);


BACNET_STACK_EXPORT
int bvlc_send_pdu(BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len);

BACNET_STACK_EXPORT
uint16_t bvlc_get_last_result(void);
BACNET_STACK_EXPORT
void bvlc_set_last_result(uint16_t result_code);

BACNET_STACK_EXPORT
uint8_t bvlc_get_function_code(void);
BACNET_STACK_EXPORT
void bvlc_set_function_code(uint8_t function_code);

BACNET_STACK_EXPORT
void bvlc_maintenance_timer(uint16_t seconds);

BACNET_STACK_EXPORT
void bvlc_init(void);

BACNET_STACK_EXPORT
void bvlc_debug_enable(void);

BACNET_STACK_EXPORT
void bvlc_debug_disable(void);

/* send a Read BDT request */
BACNET_STACK_EXPORT
int bvlc_bbmd_read_bdt(BACNET_IP_ADDRESS *bbmd_addr);
BACNET_STACK_EXPORT
int bvlc_bbmd_write_bdt(BACNET_IP_ADDRESS *bbmd_addr,
    BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bdt_list);
/* send a Read FDT request */
BACNET_STACK_EXPORT
int bvlc_bbmd_read_fdt(BACNET_IP_ADDRESS *bbmd_addr);

/* registers with a bbmd as a foreign device */
BACNET_STACK_EXPORT
int bvlc_register_with_bbmd(
    BACNET_IP_ADDRESS *address, uint16_t time_to_live_seconds);
BACNET_STACK_EXPORT
void bvlc_remote_bbmd_address(
    BACNET_IP_ADDRESS *address);
BACNET_STACK_EXPORT
uint16_t bvlc_remote_bbmd_lifetime(
    void);

/* Local interface to manage BBMD.
 * The interface user needs to handle mutual exclusion if needed i.e.
 * BACnet packet is not being handled when the BBMD table is modified.
 */

/* Get broadcast distribution table list */
BACNET_STACK_EXPORT
BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bvlc_bdt_list(void);

/* Invalidate all entries in the broadcast distribution table */
BACNET_STACK_EXPORT
void bvlc_bdt_list_clear(void);

/* Get foreign device table list */
BACNET_IP_FOREIGN_DEVICE_TABLE_ENTRY *bvlc_fdt_list(void);

/* Backup broadcast distribution table to a file.
 * Filename is the BBMD_BACKUP_FILE constant
 */
BACNET_STACK_EXPORT
void bvlc_bdt_backup_local(void);

/* Restore broadcast distribution from a file.
 * Filename is the BBMD_BACKUP_FILE constant
 */
BACNET_STACK_EXPORT
void bvlc_bdt_restore_local(void);

/* Set global IP address of a NAT enabled router which is used in forwarded
 * messages. Enables NAT handling.
 */
BACNET_STACK_EXPORT
void bvlc_set_global_address_for_nat(const BACNET_IP_ADDRESS *addr);

/* Disable NAT handling of BBMD.
 */
BACNET_STACK_EXPORT
void bvlc_disable_nat(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
