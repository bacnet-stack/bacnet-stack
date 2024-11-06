/**************************************************************************
*
* Copyright (C) 2009 Steve Karg <skarg@users.sourceforge.net>
*
* SPDX-License-Identifier: MIT
*
*********************************************************************/
#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void input_init(
        void);
    void input_task(
        void);
    uint8_t input_address(
        void);
    bool input_button_value(
        uint8_t index);
    uint8_t input_rotary_value(
        uint8_t index);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
