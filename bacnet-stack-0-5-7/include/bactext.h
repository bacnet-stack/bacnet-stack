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

/* tiny implementations have no need to print */
#if PRINT_ENABLED
#define BACTEXT_PRINT_ENABLED
#else
#ifdef TEST
#define BACTEXT_PRINT_ENABLED
#endif
#endif

#ifdef BACTEXT_PRINT_ENABLED

#include <stdbool.h>
#include <stdint.h>
#include "indtext.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    const char *bactext_confirmed_service_name(
        unsigned index);
    const char *bactext_unconfirmed_service_name(
        unsigned index);
    const char *bactext_application_tag_name(
        unsigned index);
    const char *bactext_object_type_name(
        unsigned index);
    bool bactext_object_type_index(
        const char *search_name,
        unsigned *found_index);
    const char *bactext_notify_type_name(
        unsigned index);
    const char *bactext_event_type_name(
        unsigned index);
    const char *bactext_property_name(
        unsigned index);
    const char *bactext_property_known_name(
        unsigned index);
    bool bactext_property_index(
        const char *search_name,
        unsigned *found_index);
    const char *bactext_engineering_unit_name(
        unsigned index);
    bool bactext_engineering_unit_index(
        const char *search_name,
        unsigned *found_index);
    const char *bactext_reject_reason_name(
        unsigned index);
    const char *bactext_abort_reason_name(
        unsigned index);
    const char *bactext_error_class_name(
        unsigned index);
    const char *bactext_error_code_name(
        unsigned index);
    unsigned bactext_property_id(
        const char *name);
    const char *bactext_month_name(
        unsigned index);
    const char *bactext_week_of_month_name(
        unsigned index);
    const char *bactext_day_of_week_name(
        unsigned index);
    const char *bactext_event_state_name(
        unsigned index);
    const char *bactext_binary_present_value_name(
        unsigned index);
    const char *bactext_binary_polarity_name(
        unsigned index);
    bool bactext_binary_present_value_index(
        const char *search_name,
        unsigned *found_index);
    const char *bactext_reliability_name(
        unsigned index);
    const char *bactext_device_status_name(
        unsigned index);
    const char *bactext_segmentation_name(
        unsigned index);
    const char *bactext_node_type_name(
        unsigned index);
    const char *bactext_character_string_encoding_name(
        unsigned index);
    const char *bactext_event_transition_name(
        unsigned index);
    bool bactext_event_transition_index(
        const char *search_name,
        unsigned *found_index);

    const char *bactext_days_of_week_name(
        unsigned index);
    bool bactext_days_of_week_index(
        const char *search_name,
        unsigned *found_index);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* BACTEXT_PRINT_ENABLED */
#endif
