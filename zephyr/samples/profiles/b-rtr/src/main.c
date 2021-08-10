/*
 * Copyright (C) 2020 Legrand North America, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <kernel.h>
#include <stdint.h>

#include "bacnet/config.h"
#include "bacnet/bacdef.h"
#include "bacnet/bacdcode.h"
#include "bacnet/apdu.h"
#include "bacnet/dcc.h"
#include "bacnet/iam.h"
#include "bacnet/npdu.h"
#include "bacnet/getevent.h"
#include "bacnet/version.h"
#include "bacnet/basic/services.h"
#include "bacnet/datalink/dlenv.h"
#include "bacnet/basic/sys/filename.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/binding/address.h"
/* include the device object */
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/lc.h"
#include "bacnet/basic/object/trendlog.h"
#if defined(INTRINSIC_REPORTING)
#include "bacnet/basic/object/nc.h"
#endif /* defined(INTRINSIC_REPORTING) */

/* Logging module registration is already done in ports/zephyr/main.c */
#include <logging/log.h>
LOG_MODULE_DECLARE(bacnet, CONFIG_BACNETSTACK_LOG_LEVEL);

/** Buffer used for receiving */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/** Initialize the handlers we will utilize.
 * @see Device_Init, apdu_set_unconfirmed_handler, apdu_set_confirmed_handler
 */
static void service_handlers_init(void)
{
	Device_Init(NULL);
	/* we need to handle who-is to support dynamic device binding */
	apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS,
				     handler_who_is);
	apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS,
				     handler_who_has);
	/* set the handler for all the services we don't implement */
	/* It is required to send the proper reject message... */
	apdu_set_unrecognized_service_handler_handler(
		handler_unrecognized_service);
	/* Set the handlers for any confirmed services that we support. */
	/* We must implement read property - it's required! */
	apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
				   handler_read_property);
	apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROP_MULTIPLE,
				   handler_read_property_multiple);
	apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROPERTY,
				   handler_write_property);
	apdu_set_confirmed_handler(SERVICE_CONFIRMED_WRITE_PROP_MULTIPLE,
				   handler_write_property_multiple);
	apdu_set_confirmed_handler(SERVICE_CONFIRMED_REINITIALIZE_DEVICE,
				   handler_reinitialize_device);
	/* handle communication so we can shutup when asked */
	apdu_set_confirmed_handler(
		SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
		handler_device_communication_control);
}

void main(void)
{
	LOG_INF("\n*** BACnet Profile B-SS Sample ***\n");
	LOG_INF("BACnet Stack Version " BACNET_VERSION_TEXT);
	LOG_INF("BACnet Device ID: %u", Device_Object_Instance_Number());
	LOG_INF("BACnet Device Max APDU: %d", MAX_APDU);

	service_handlers_init();
	datalink_init(NULL);

	/* configure the timeout values */
	int64_t last_ms = k_uptime_get();

	/* broadcast an I-Am on startup */
	Send_I_Am(&Handler_Transmit_Buffer[0]);

	int64_t address_binding_tmr = 0;
#if defined(INTRINSIC_REPORTING)
	int64_t recipient_scan_tmr = 0;
#endif
#if defined(BACNET_TIME_MASTER)
	BACNET_DATE_TIME bdatetime = { 0 };
#endif
	for (;;) {
		k_sleep(K_MSEC(1)); /* Allows debug prints */
		BACNET_ADDRESS src = { 0 }; /* address where message came from */
		const unsigned timeout_ms = 1;

		int64_t current_ms = k_uptime_get();

		/* returns 0 bytes on timeout */
		uint16_t const pdu_len = datalink_receive(&src, &Rx_Buf[0],
							  MAX_MPDU, timeout_ms);

		/* process */
		if (pdu_len > 0) {
			npdu_handler(&src, &Rx_Buf[0], pdu_len);
		}

		if (current_ms - last_ms > 1000) {
			const uint32_t elapsed_milliseconds =
				(uint32_t)(current_ms - last_ms);
			//TODO:     const uint32_t elapsed_seconds = elapsed_milliseconds / 1000UL;
			last_ms = current_ms;

//TODO:     dcc_timer_seconds(elapsed_seconds);
//TODO:     datalink_maintenance_timer(elapsed_seconds);
//TODO:     dlenv_maintenance_timer(elapsed_seconds);

//TODO:     Load_Control_State_Machine_handler():

//TODO:     handler_cov_timer_seconds(elapsed_seconds);
//TODO:     tsm_timer_milliseconds(elapsed_milliseconds);
//TODO:     trend_log_timer(elapsed_seconds);
#if defined(INTRINSIC_REPORTING)
//TODO:     Device_local_reporting();
#endif
#if defined(BACNET_TIME_MASTER)
//TODO:     Device_getCurrentDateTime(&bdatetime);
//TODO:     handler_Timesync_task(&bdatetime);
#endif
			address_binding_tmr += elapsed_milliseconds;
#if defined(INTRINSIC_REPORTING)
			recipient_scan_tmr += elapsed_milliseconds;
#endif
		}
		//TODO:     handler_cov_task();
		/* scan cache address */
		if (address_binding_tmr >= 60 * 1000) {
			//TODO:     address_cache_timer(address_binding_tmr / 1000);
			address_binding_tmr = 0;
		}
#if defined(INTRINSIC_REPORTING)
		/* try to find addresses of recipients */
		if (recipient_scan_tmr >= NC_RESCAN_RECIPIENTS_SECS * 1000) {
			//TODO:     Notification_Class_find_recipient();
			recipient_scan_tmr = 0;
		}
#endif
		/* output */

		/* blink LEDs, Turn on or off outputs, etc */
	}
}
