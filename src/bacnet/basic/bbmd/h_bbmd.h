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
#include "bacnet/bacdef.h"
#include "bacnet/datalink/bvlc.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* user application function prototypes */
int bvlc_handler(BACNET_IP_ADDRESS *addr,
    BACNET_ADDRESS *src,
    uint8_t *npdu,
    uint16_t npdu_len);

int bvlc_send_pdu(BACNET_ADDRESS *dest,
    BACNET_NPDU_DATA *npdu_data,
    uint8_t *pdu,
    unsigned pdu_len);

uint16_t bvlc_get_last_result(void);

uint8_t bvlc_get_function_code(void);

void bvlc_maintenance_timer(uint16_t seconds);

void bvlc_init(void);

void bvlc_debug_enable(void);

/* send a Read BDT request */
int bvlc_bbmd_read_bdt(BACNET_IP_ADDRESS *bbmd_addr);
/* send a Read FDT request */
int bvlc_bbmd_read_fdt(BACNET_IP_ADDRESS *bbmd_addr);

/* registers with a bbmd as a foreign device */
int bvlc_register_with_bbmd(
    BACNET_IP_ADDRESS *address, uint16_t time_to_live_seconds);

/* Local interface to manage BBMD.
 * The interface user needs to handle mutual exclusion if needed i.e.
 * BACnet packet is not being handled when the BBMD table is modified.
 */

/* Get broadcast distribution table list */
BACNET_IP_BROADCAST_DISTRIBUTION_TABLE_ENTRY *bvlc_bdt_list(void);

/* Invalidate all entries in the broadcast distribution table */
void bvlc_bdt_list_clear(void);

/* Backup broadcast distribution table to a file.
 * Filename is the BBMD_BACKUP_FILE constant
 */
void bvlc_bdt_backup_local(void);

/* Restore broadcast distribution from a file.
 * Filename is the BBMD_BACKUP_FILE constant
 */
void bvlc_bdt_restore_local(void);

/* Set global IP address of a NAT enabled router which is used in forwarded
 * messages. Enables NAT handling.
 */
void bvlc_set_global_address_for_nat(const BACNET_IP_ADDRESS *addr);

/* Disable NAT handling of BBMD.
 */
void bvlc_disable_nat(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
