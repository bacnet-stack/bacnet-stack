# BACnet Stack Port for Raspberry Pi Pico

This directory contains the Raspberry Pi Pico port of the BACnet stack, supporting both BACnet/IP and BACnet MS/TP.

## Overview

This port provides:
- **BACnet/IP** support with abstracted network layer (Developer must provide Ethernet implementation)
- **BACnet MS/TP** support using UART (RS485)

## Using This Port

### Implement Network Functions

Developer must implement the following functions declared in `bip.h`:

```c
bool bip_socket_init(uint16_t port);

int bip_socket_send(
    const uint8_t *dest_addr,
    uint16_t dest_port,
    const uint8_t *mtu,
    uint16_t mtu_len);

int bip_socket_receive(
    uint8_t *buf,
    uint16_t buf_len,
    uint8_t *src_addr,
    uint16_t *src_port);

void bip_socket_cleanup(void);

bool bip_get_local_network_info(uint8_t *local_addr, uint8_t *netmask);
```

### Example Usage

```c
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "mstimer_init.h"
#include "bip.h"
/* BACnet Stack Includes */
#include "bacnet/datalink/datalink.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/dcc.h"

static struct mstimer DCC_Timer;

int main() {
    // Initialize Pico
    stdio_init_all();
    systimer_init();

    // Initialize BIP datalink
    Device_Set_Object_Instance_Number(12345);
    Device_Init(NULL);
    address_init();
    
    // Initialize BACnet/IP on port 0xBAC0 (47808)
    if (!bip_init(0xBAC0)) {
        return -1;
    }

    /* set up confirmed service unrecognized service handler - required! */
    Send_I_Am(&Handler_Transmit_Buffer[0]);

    mstimer_set(&DCC_Timer, 1000); // 1 second timer
    printf("BACnet BIP Initialized");
    
    BACNET_ADDRESS src = {0};
    uint8_t rx_buf[480];
    const uint16_t max_pdu = sizeof(rx_buf);

    while (1) 
    {
        if (mstimer_expired(&DCC_Timer)) 
        {
            mstimer_reset(&DCC_Timer);
            dcc_timer_seconds(1);
        }
        
        uint16_t pdu_len = bip_receive(&src, rx_buf, max_pdu, 0);
        if (pdu_len > 0) 
        {
            npdu_handler(&src, rx_buf, pdu_len);
            printf("%u bytes received from MAC %u\r\n", pdu_len, src.mac[0]);
            for (int i=0;i<pdu_len;i++) printf("%02X ", rx_buf[i]);
            printf("\n");
        }
    }
    
    return 0;
}
```

## BACnet MS/TP (RS485)

The RS485 driver is ready to use without modification. It uses the Pico SDK UART functions.

### RS485 Pin Configuration (rs485.h)

```c
#define RS485_UART_ID       uart1
#define RS485_BAUD_RATE     38400
#define RS485_TX_PIN        8
#define RS485_RX_PIN        9
#define RS485_DE_PIN        10      // Driver Enable (DE/RE pin)
```

### Using MS/TP

```c
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/uart.h"
#include "rs485.h"
#include "mstimer_init.h"
#include "dlenv.h"

/* BACnet Stack Includes */
#include "bacnet/datalink/datalink.h"
#include "bacnet/datalink/dlmstp.h"
#include "bacnet/datalink/mstp.h"
#include "bacnet/basic/services.h"
#include "bacnet/basic/object/device.h"
#include "bacnet/basic/tsm/tsm.h"
#include "bacnet/whois.h"
#include "bacnet/dcc.h"

static struct mstimer DCC_Timer;

int main() {
    // Initialize Pico
    stdio_init_all();
    systimer_init();

    // Initialize MS/TP datalink
    address_init();
    pico_dlenv_init(2);
    Device_Set_Object_Instance_Number(12345);
    Device_Init(NULL);
    /* set up confirmed service unrecognized service handler - required! */
    Send_I_Am(&Handler_Transmit_Buffer[0]);

    mstimer_set(&DCC_Timer, 1000); // 1 second timer
    printf("BACnet MS/TP Initialized.");

    BACNET_ADDRESS src = {0};
    uint8_t rx_buf[480];
    const uint16_t max_pdu = sizeof(rx_buf);
    
    // MS/TP application code
    while (1) 
    {
        if (mstimer_expired(&DCC_Timer)) 
        {
            mstimer_reset(&DCC_Timer);
            dcc_timer_seconds(1);
        }
        
        uint16_t pdu_len = datalink_receive(&src, rx_buf, max_pdu, 1);
        if (pdu_len) 
        {
          npdu_handler(&src, rx_buf, pdu_len);
          printf("%u bytes received from MAC %u\r\n", pdu_len, src.mac[0]);
          for (int i=0;i<pdu_len;i++) printf("%02X ", rx_buf[i]);
          printf("\n");
        }
    }
    
    return 0;
}
```
