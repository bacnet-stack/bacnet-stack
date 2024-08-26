/**************************************************************************
*
* Copyright (C) 2011 Steve Karg <skarg@users.sourceforge.net>
*
* SPDX-License-Identifier: MIT
*
*********************************************************************/
#ifndef HARDWARE_H
#define HARDWARE_H

/* IO Port RPDL function definitions */
#include "r_pdl_io_port.h"
/* CMT RPDL function definitions */
#include "r_pdl_cmt.h"
/* General RPDL function definitions */
#include "r_pdl_definitions.h"
/*  Evaluation Board Definitions */
#include "YRDKRX62N.h"
/* Ethernet driver */
#include "r_ether.h"
/* LCD Driver */
#include "lcd.h"

/* there are 4..15 LEDs on the board */
#define MAX_LEDS 16
/* use the LEDS as binary outputs */
#define MAX_BINARY_OUTPUTS 16

#endif
