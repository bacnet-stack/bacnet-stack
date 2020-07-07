//
// Copyleft  F.Chaxel 2017
//

#include "bacnet/config.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/basic/services.h"

#include "bacnet/basic/services.h"
#include "bacnet/datalink/datalink.h"
#include "bacnet/dcc.h"
#include "bacnet/basic/tsm/tsm.h"
// conflict filename address.h with another file in default include paths
#include "../lib/stack/address.h"
#include "bacnet/datalink/bip.h"

#include "bacnet/basic/object/device.h"
#include "bacnet/basic/object/ai.h"
#include "bacnet/basic/object/bo.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"

#include "driver/gpio.h"

#include "lwip/sockets.h"
#include "lwip/netdb.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

// hidden function not in any .h files
extern uint8_t temprature_sens_read();
extern uint32_t hall_sens_read();

// Wifi params
wifi_config_t wifi_config = {
    .sta = {
        .ssid = "myWifi",
        .password = "myPass",
    },
};

// GPIO 5 has a Led on Sparkfun ESP32 board
#define BACNET_LED 5

uint8_t Handler_Transmit_Buffer[MAX_PDU] = { 0 };

/** Static receive buffer, initialized with zeros by the C Library Startup Code. */

uint8_t Rx_Buf[MAX_MPDU + 16 /* Add a little safety margin to the buffer,
                              * so that in the rare case, the message
                              * would be filled up to MAX_MPDU and some
                              * decoding functions would overrun, these
                              * decoding functions will just end up in
                              * a safe field of static zeros. */] = { 0 };

EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;

/* BACnet handler, stack init, IAm */
void StartBACnet()
{
    /* we need to handle who-is to support dynamic device binding */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_WHO_IS, handler_who_is);

    /* set the handler for all the services we don't implement */
    /* It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler(handler_unrecognized_service);
    /* Set the handlers for any confirmed services that we support. */
    /* We must implement read property - it's required! */
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROPERTY, handler_read_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_READ_PROP_MULTIPLE, handler_read_property_multiple);

    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_WRITE_PROPERTY, handler_write_property);
    apdu_set_confirmed_handler(
        SERVICE_CONFIRMED_SUBSCRIBE_COV, handler_cov_subscribe);

    address_init();
    bip_init(NULL);
    Send_I_Am(&Handler_Transmit_Buffer[0]);
}

/* wifi events handler : start & stop bacnet with an event  */
esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_CONNECTED:
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            if (xEventGroupGetBits(wifi_event_group) != CONNECTED_BIT) {
                xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
                StartBACnet();
            }
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            /* This is a workaround as ESP32 WiFi libs don't currently
               auto-reassociate. */
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            bip_cleanup();
            break;
        default:
            break;
    }
    return ESP_OK;
}

/* tcpip & wifi station start */

void wifi_init_station(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    esp_event_loop_init(wifi_event_handler, NULL);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_STA);

    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);

    esp_wifi_start();
}

/* setup gpio & nv flash, call wifi init code */
void setup()
{
    gpio_pad_select_gpio(BACNET_LED);
    gpio_set_direction(BACNET_LED, GPIO_MODE_OUTPUT);

    gpio_set_level(BACNET_LED, 0);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES) {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }
    wifi_init_station();
}

/* Bacnet Task */
void BACnetTask(void *pvParameters)
{
    uint16_t pdu_len = 0;
    BACNET_ADDRESS src = { 0 };
    unsigned timeout = 1;

    // Init Bacnet objets dictionnary
    Device_Init(NULL);
    Device_Set_Object_Instance_Number(12);

    setup();

    uint32_t tickcount = xTaskGetTickCount();

    for (;;) {
        vTaskDelay(
            10 / portTICK_PERIOD_MS); // could be remove to speed the code

        // do nothing if not connected to wifi
        xEventGroupWaitBits(
            wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
        {
            uint32_t newtick = xTaskGetTickCount();

            // one second elapse at least (maybe much more if Wifi was
            // deconnected for a long)
            if ((newtick < tickcount) ||
                ((newtick - tickcount) >= configTICK_RATE_HZ)) {
                tickcount = newtick;
                dcc_timer_seconds(1);
                bvlc_maintenance_timer(1);
                handler_cov_timer_seconds(1);
                tsm_timer_milliseconds(1000);

                // Read analog values from internal sensors
                Analog_Input_Present_Value_Set(0, temprature_sens_read());
                Analog_Input_Present_Value_Set(1, hall_sens_read());
            }

            pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout);
            if (pdu_len) {
                npdu_handler(&src, &Rx_Buf[0], pdu_len);

                if (Binary_Output_Present_Value(0) == BINARY_ACTIVE)
                    gpio_set_level(BACNET_LED, 1);
                else
                    gpio_set_level(BACNET_LED, 0);
            }

            handler_cov_task();
        }
    }
}
/* Entry point */
void app_main()
{
    // Cannot run BACnet code here, the default stack size is to small : 4096
    // byte
    xTaskCreate(BACnetTask, /* Function to implement the task */
        "BACnetTask", /* Name of the task */
        10000, /* Stack size in words */
        NULL, /* Task input parameter */
        20, /* Priority of the task */
        NULL); /* Task handle. */
}
