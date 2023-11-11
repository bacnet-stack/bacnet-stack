/**
 * @file
 * @author Steve Karg <skarg@users.sourceforge.net>
 * @date February 2023
 * @brief BACnet initialization and tasks
 *
 * SPDX-License-Identifier: MIT
 *
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
/* hardware layer includes */
#include "FreeRTOS.h"
#include "dlmstp-init.h"
#include "rs485.h"
/* BACnet Stack includes */
#include "bacnet/npdu.h"
#include "bacnet/dcc.h"
#include "bacnet/iam.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/sys/mstimer.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/sys/ringbuf.h"
/* BACnet objects */
#include "bacnet/basic/object/device.h"
/* me */
#include "bacnet.h"

/* FreeRTOS file data */
static TaskHandle_t BACnet_Task_Handle;
static volatile unsigned long BACnet_Task_High_Water_Mark;
static TaskHandle_t BACnet_MSTP_Task_Handle;
static volatile unsigned long BACnet_MSTP_Task_High_Water_Mark;
static SemaphoreHandle_t BACnet_PDU_Available;
static unsigned BACnet_Task_Counter;
/* Device ID to track changes */
static uint32_t Device_ID = 0xFFFFFFFF;
/* timer for device communications control */
static struct mstimer DCC_Timer;
#define DCC_CYCLE_SECONDS 1
/* timer for TSM */
static struct mstimer TSM_Timer;
#define TSM_CYCLE_SECONDS 1
/* timer for Reinit */
static struct mstimer Reinit_Timer;
/* timer for write-property task */
static struct mstimer WriteProperty_Timer;
#define WRITE_CYCLE_SECONDS 60

#define BACNET_MSTP_TASK_MILLISECONDS 1
#define BACNET_TASK_MILLISECONDS 10

/* count must be a power of 2 for ringbuf library */
#ifndef MSTP_PDU_PACKET_COUNT
#define MSTP_PDU_PACKET_COUNT 2
#endif

/* MS/TP port */
static struct dlmstp_packet
    Receive_PDU_Buffer[NEXT_POWER_OF_2(MSTP_PDU_PACKET_COUNT)];
static RING_BUFFER Receive_PDU_Queue;
/* buffer for incoming packet */
static uint8_t Receive_Buffer[MAX_MPDU];

/**
 * @brief handles reinitializing the device after a few seconds
 * @note gives the device enough time to acknowledge the RD request
 */
static void reinit_task(void)
{
    BACNET_REINITIALIZED_STATE state = BACNET_REINIT_IDLE;

    state = Device_Reinitialized_State();
    if (state == BACNET_REINIT_IDLE) {
        /* set timer to never expire */
        mstimer_set(&Reinit_Timer, 0);
    } else if (mstimer_interval(&Reinit_Timer) != 0) {
        if (mstimer_expired(&Reinit_Timer)) {
            /* FIXME: system_reset(); */
        }
    } else {
        mstimer_set(&Reinit_Timer, 3000);
    }
}

/**
 * @brief Initializes the BACnet Device - just one of many ways to do it
 * @param mac - MSTP MAC address
 */
static void device_id_init(uint8_t mac)
{
    uint32_t device_id = BACNET_MAX_INSTANCE;

    /* Get the device ID from the eeprom */
    if (device_id < BACNET_MAX_INSTANCE) {
        Device_Set_Object_Instance_Number(device_id);
    } else {
        Device_Set_Object_Instance_Number(mac);
    }
}

/**
 * @brief handles recurring strictly timed task
 * @brief timeout - number of milliseconds for datalink to wait for packet
 * @note called by ISR or RTOS every timeout milliseconds
 */
static void bacnet_dlmstp_task(unsigned int timeout)
{
    struct dlmstp_packet *pkt = NULL;
    uint16_t pdu_len = 0;
    BACNET_ADDRESS src = { 0 };

    pdu_len = dlmstp_receive(
        &src, &Receive_Buffer[0], sizeof(Receive_Buffer), timeout);
    if (pdu_len) {
        pkt = (void *)Ringbuf_Data_Peek(&Receive_PDU_Queue);
        if (pkt) {
            memcpy(pkt->pdu, Receive_Buffer, MAX_MPDU);
            bacnet_address_copy(&pkt->address, &src);
            pkt->pdu_len = pdu_len;
            if (Ringbuf_Data_Put(&Receive_PDU_Queue, (volatile uint8_t *)pkt)) {
                xSemaphoreGive(BACnet_PDU_Available);
            }
        }
    }
}

/**
 * @brief handles recurring background task
 */
void bacnet_task(void)
{
    bool hello_world = false;
    struct dlmstp_packet pkt = { 0 };
    bool pdu_available = false;

    BACnet_Task_Counter++;
    /* hello, World! */
    if (Device_ID != Device_Object_Instance_Number()) {
        Device_ID = Device_Object_Instance_Number();
        /* update EEPROM? */
        hello_world = true;
    }
    if (hello_world) {
        Send_I_Am(&Handler_Transmit_Buffer[0]);
    }
    /* handle the timers */
    if (mstimer_expired(&DCC_Timer)) {
        mstimer_reset(&DCC_Timer);
        dcc_timer_seconds(DCC_CYCLE_SECONDS);
    }
    reinit_task();
    /* handle the messaging */
    if (!dlmstp_send_pdu_queue_full()) {
        pdu_available = Ringbuf_Pop(&Receive_PDU_Queue, (uint8_t *)&pkt);
    }
    if (pdu_available) {
        npdu_handler(&pkt.address, &pkt.pdu[0], pkt.pdu_len);
    }
    if (mstimer_expired(&TSM_Timer)) {
        mstimer_reset(&TSM_Timer);
        tsm_timer_milliseconds(mstimer_interval(&TSM_Timer));
    }
}

/**
 * @brief FreeRTOS recurring time critical task
 */
static void BACnet_MSTP_Task(void *pvParameters)
{
    const TickType_t xFrequency =
        BACNET_MSTP_TASK_MILLISECONDS / portTICK_PERIOD_MS;

    for (;;) {
        vTaskDelay(xFrequency);
        bacnet_dlmstp_task(BACNET_MSTP_TASK_MILLISECONDS);
#if (INCLUDE_uxTaskGetStackHighWaterMark2 == 1)
        BACnet_MSTP_Task_High_Water_Mark = uxTaskGetStackHighWaterMark(NULL);
#endif
    }
    /* Should never go there */
    vTaskDelete(NULL);
}

/**
 * @brief FreeRTOS main loop recurring task
 */
static void BACnet_Task(void *pvParameters)
{
    const TickType_t xBlockTime = BACNET_TASK_MILLISECONDS / portTICK_PERIOD_MS;

    for (;;) {
        /* block to wait for a notification from either MS/TP task */
        xSemaphoreTake(BACnet_PDU_Available, xBlockTime);
        bacnet_task();
#if (INCLUDE_uxTaskGetStackHighWaterMark2 == 1)
        BACnet_Task_High_Water_Mark = uxTaskGetStackHighWaterMark(NULL);
#endif
    }
    /* Should never go there */
    vTaskDelete(NULL);
}

/**
 * @brief initializes the BACnet library
 */
void bacnet_init(void)
{
    const signed char *const BACnet_Task_Name =
        (const signed char *const)"BACnet Task";
    const signed char *const BACnet_MSTP_Task_Name =
        (const signed char *const)"BACnet MSTP Task";
    uint8_t mstp_mac = 123;

    /* configure application layer */
    Device_Init(NULL);
    device_id_init(mstp_mac);
    /* set up our confirmed service unrecognized service handler - required! */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(
        SERVICE_UNCONFIRMED_WHO_IS, handler_who_is_unicast);
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_HAS, handler_who_has);
    /* Set the handlers for any confirmed services that we support. */
    /* We must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_REINITIALIZE_DEVICE, handler_reinitialize_device);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, handler_write_property);
    /* handle communication so we can shut up when asked */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_DEVICE_COMMUNICATION_CONTROL,
        handler_device_communication_control);
    /* start the cyclic 1 second timer for DCC */
    mstimer_set(&DCC_Timer, DCC_CYCLE_SECONDS * 1000);
    /* start the cyclic 1 second timer for TSM */
    mstimer_set(&TSM_Timer, TSM_CYCLE_SECONDS * 1000);
    /* start the cyclic 1 minute timer for WriteProperty */
    mstimer_set(&WriteProperty_Timer, WRITE_CYCLE_SECONDS * 1000);
    /* add counting semaphores to signal PDU handling */
    BACnet_PDU_Available =
        xSemaphoreCreateCounting(MSTP_PDU_PACKET_COUNT * 2, 0);
    /* create the tasks */
    xTaskCreate(BACnet_MSTP_Task, "BACnet MSTP",
        configMINIMAL_STACK_SIZE + (MAX_APDU * 1), NULL,
        configMAX_PRIORITIES - 2, &BACnet_MSTP_Task_Handle);
    xTaskCreate(BACnet_Task, "BACnet",
        configMINIMAL_STACK_SIZE + (MAX_APDU * 3), NULL,
        configMAX_PRIORITIES - 4, &BACnet_Task_Handle);
    dlmstp_freertos_init();
}
