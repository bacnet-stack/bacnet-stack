/**
 * @file
 * @brief Handles Acknowledgment of Atomic Read  File response.
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: MIT
 */
#include <stddef.h>
#include <stdint.h>
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/bacdcode.h"
#include "bacnet/npdu.h"
#include "bacnet/apdu.h"
#include "bacnet/arf.h"
#include "bacnet/basic/object/bacfile.h"
/* basic objects, services, TSM, and datalink */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/services.h"

/* We performed an AtomicReadFile Request, */
/* and here is the data from the server */
/* Note: it does not have to be the same file=instance */
/* that someone can read from us.  It is common to */
/* use the description as the file name. */
void handler_atomic_read_file_ack(
    uint8_t *service_request,
    uint16_t service_len,
    BACNET_ADDRESS *src,
    BACNET_CONFIRMED_SERVICE_ACK_DATA *service_data)
{
    int len = 0;
    BACNET_ATOMIC_READ_FILE_DATA data;
    uint32_t instance = 0;

    (void)src;
    /* get the file instance from the tsm data before freeing it */
    instance = bacfile_instance_from_tsm(service_data->invoke_id);
    len = arf_ack_decode_service_request(service_request, service_len, &data);
#if PRINT_ENABLED
    fprintf(stderr, "Received Read-File Ack!\n");
#endif
    if ((len > 0) && (instance <= BACNET_MAX_INSTANCE)) {
        /* write the data received to the file specified */
        if (data.access == FILE_STREAM_ACCESS) {
            bacfile_read_ack_stream_data(instance, &data);
        } else if (data.access == FILE_RECORD_ACCESS) {
            bacfile_read_ack_record_data(instance, &data);
        }
    }
}
