/*
 * Copyright (c) 2020 Legrand North America, LLC.
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr.h>
#include <sys/printk.h>

void main(void)
{
    printk("Hello BACnet-Stack! %s\n", CONFIG_BOARD);
}
