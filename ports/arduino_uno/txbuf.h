/**************************************************************************
 *
 * Copyright (C) 2005 Steve Karg <skarg@users.sourceforge.net>
 *
 * SPDX-License-Identifier: MIT
 *
 *********************************************************************/
#ifndef TXBUF_H
#define TXBUF_H

#include <stddef.h>
#include <stdint.h>
#include "bacnet/config.h"
#include "bacnet/datalink/datalink.h"

extern uint8_t Handler_Transmit_Buffer[MAX_PDU];

#endif
