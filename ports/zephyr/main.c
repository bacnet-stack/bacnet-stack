/*
 * Copyright (C) 2020 Legrand North America, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

/* Standard includes */
#include <stdint.h>

/* Zephyr includes */
#include <init.h>
#include <kernel.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <zephyr.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(bacnet);

/* To do:  init()

static int init(struct device *unused)
{
    ARG_UNUSED(unused);

    return 0;
}

SYS_INIT(init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

*/
