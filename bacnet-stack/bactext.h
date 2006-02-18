/*####COPYRIGHTBEGIN####
 -------------------------------------------
 Copyright (C) 2005 Steve Karg

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to
 The Free Software Foundation, Inc.
 59 Temple Place - Suite 330
 Boston, MA  02111-1307, USA.

 As a special exception, if other files instantiate templates or
 use macros or inline functions from this file, or you compile
 this file and link it with other works to produce a work based
 on this file, this file does not by itself cause the resulting
 work to be covered by the GNU General Public License. However
 the source code for this file must still be made available in
 accordance with section (3) of the GNU General Public License.

 This exception does not invalidate any other reasons why a work
 based on this file might be covered by the GNU General Public
 License.
 -------------------------------------------
####COPYRIGHTEND####*/
#ifndef BACTEXT_H
#define BACTEXT_H

#include <stdbool.h>
#include <stdint.h>
#include "indtext.h"

#ifdef __cplusplus
extern "C" {
#endif                          /* __cplusplus */

    const char *bactext_confirmed_service_name(int index);
    const char *bactext_unconfirmed_service_name(int index);
    const char *bactext_application_tag_name(int index);
    const char *bactext_object_type_name(int index);
    const char *bactext_property_name(int index);
    const char *bactext_engineering_unit_name(int index);
    const char *bactext_reject_reason_name(int index);
    const char *bactext_abort_reason_name(int index);
    const char *bactext_error_class_name(int index);
    const char *bactext_error_code_name(int index);
    unsigned bactext_property_id(const char *name);
    const char *bactext_month_name(int index);
    const char *bactext_week_of_month_name(int index);
    const char *bactext_day_of_week_name(int index);
    const char *bactext_event_state_name(int index);
    const char *bactext_binary_present_value_name(int index);
    const char *bactext_reliability_name(int index);
    const char *bactext_device_status_name(int index);
    const char *bactext_segmentation_name(int index);


#ifdef __cplusplus
}
#endif                          /* __cplusplus */
#endif
