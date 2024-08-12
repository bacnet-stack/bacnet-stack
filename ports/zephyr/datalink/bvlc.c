/**************************************************************************
 *
 * Copyright (C) 2020 Steve Karg
 *
 * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0
 *
 *********************************************************************/
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"
/* BACnet Stack API */
#include "bacnet/datalink/bvlc.h"

/**
 * @brief Encode the BVLC header
 *
 * @param pdu - buffer to store the encoding
 * @param pdu_size - size of the buffer to store encoding
 * @param message_type - BVLL Messages
 * @param length - number of bytes for this message type
 *
 * @return number of bytes encoded
 */
void bvlc_file_bdt_write(
    void *data,
    size_t len)
{
    /* TODO: Write BDT data blob into persistent storage */
}


size_t bvlc_file_bdt_read(
    void *data,
    size_t len)
{
    size_t sz = -1;

    /* TODO: Read BD data blob from persistent storage */

    return sz;
}
