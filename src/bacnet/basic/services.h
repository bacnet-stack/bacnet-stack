/**
 * @file
 * @brief APIs for all the basic BACnet service handlers
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date 2005
 * @copyright SPDX-License-Identifier: MIT
 */
#ifndef BASIC_SERVICES_H
#define BASIC_SERVICES_H
/* BACnet Stack defines - first */
#include "bacnet/bacdef.h"

/* NPDU layer handlers */
#include "bacnet/basic/npdu/h_npdu.h"
#include "bacnet/basic/npdu/h_routed_npdu.h"
#include "bacnet/basic/npdu/s_router.h"

/* application layer binding handler */
#include "bacnet/basic/binding/address.h"

/* application layer device handler */
#include "bacnet/basic/service/h_device.h"

/* application layer service handler */
#include "bacnet/basic/service/h_alarm_ack.h"
#include "bacnet/basic/service/h_apdu.h"
#include "bacnet/basic/service/h_arf.h"
#include "bacnet/basic/service/h_arf_a.h"
#include "bacnet/basic/service/h_awf.h"
#include "bacnet/basic/service/h_ccov.h"
#include "bacnet/basic/service/h_cov.h"
#include "bacnet/basic/service/h_create_object.h"
#include "bacnet/basic/service/h_dcc.h"
#include "bacnet/basic/service/h_delete_object.h"
#include "bacnet/basic/service/h_gas_a.h"
#include "bacnet/basic/service/h_get_alarm_sum.h"
#include "bacnet/basic/service/h_getevent.h"
#include "bacnet/basic/service/h_getevent_a.h"
#include "bacnet/basic/service/h_iam.h"
#include "bacnet/basic/service/h_ihave.h"
#include "bacnet/basic/service/h_list_element.h"
#include "bacnet/basic/service/h_lso.h"
#include "bacnet/basic/service/h_noserv.h"
#include "bacnet/basic/service/h_rd.h"
#include "bacnet/basic/service/h_rp.h"
#include "bacnet/basic/service/h_rp_a.h"
#include "bacnet/basic/service/h_rpm.h"
#include "bacnet/basic/service/h_rpm_a.h"
#include "bacnet/basic/service/h_rr.h"
#include "bacnet/basic/service/h_rr_a.h"
#include "bacnet/basic/service/h_ts.h"
#include "bacnet/basic/service/h_ucov.h"
#include "bacnet/basic/service/h_upt.h"
#include "bacnet/basic/service/h_whohas.h"
#include "bacnet/basic/service/h_whois.h"
#include "bacnet/basic/service/h_wp.h"
#include "bacnet/basic/service/h_wpm.h"
#include "bacnet/basic/service/h_write_group.h"

/* application layer service send helpers */
#include "bacnet/basic/service/s_abort.h"
#include "bacnet/basic/service/s_ack_alarm.h"
#include "bacnet/basic/service/s_arfs.h"
#include "bacnet/basic/service/s_awfs.h"
#include "bacnet/basic/service/s_cevent.h"
#include "bacnet/basic/service/s_cov.h"
#include "bacnet/basic/service/s_create_object.h"
#include "bacnet/basic/service/s_dcc.h"
#include "bacnet/basic/service/s_delete_object.h"
#include "bacnet/basic/service/s_error.h"
#include "bacnet/basic/service/s_get_alarm_sum.h"
#include "bacnet/basic/service/s_get_event.h"
#include "bacnet/basic/service/s_getevent.h"
#include "bacnet/basic/service/s_iam.h"
#include "bacnet/basic/service/s_ihave.h"
#include "bacnet/basic/service/s_list_element.h"
#include "bacnet/basic/service/s_lso.h"
#include "bacnet/basic/service/s_rd.h"
#include "bacnet/basic/service/s_readrange.h"
#include "bacnet/basic/service/s_rp.h"
#include "bacnet/basic/service/s_rpm.h"
#include "bacnet/basic/service/s_ts.h"
#include "bacnet/basic/service/s_uevent.h"
#include "bacnet/basic/service/s_upt.h"
#include "bacnet/basic/service/s_whohas.h"
#include "bacnet/basic/service/s_whois.h"
#include "bacnet/basic/service/s_wp.h"
#include "bacnet/basic/service/s_wpm.h"
#include "bacnet/basic/service/s_write_group.h"

/** @defgroup MISCHNDLR Miscellaneous Service Handlers
 * Various utilities and functions to support the service handlers
 */
#endif
