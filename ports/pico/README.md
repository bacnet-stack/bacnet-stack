# BACnet Stack Port for Raspberry Pi Pico

This directory contains the Raspberry Pi Pico port of the BACnet stack, supporting both BACnet/IP and BACnet MS/TP.

## Overview

This port provides:
- **BACnet/IP** support with abstracted network layer (Developer must provide Ethernet implementation)
- **BACnet MS/TP** support using UART (RS485)

## Hardware Used

The port is written against the Raspberry Pi Pico SDK. The same code is intended to build for boards that the Pico SDK exposes with compatible UART/GPIO support, such as:

- `pico`
- `pico_w`
- `pico2`
- `pico2_w`

The current `make pico-cmake` flow defaults to the Pico SDK board `pico2` for this port unless `-DPICO_BOARD=...` is passed to CMake.

For the MS/TP side, the code currently assumes:

- UART1 on GPIO 8 (`TX`) and GPIO 9 (`RX`)
- GPIO 10 as the RS485 transceiver `DE`/`RE` control pin
- An external 3.3 V RS485 transceiver module, for example a MAX3485- or SN65HVD-based module

Typical wiring is:

- Pico/Pico 2 GPIO 8 -> transceiver `DI`
- Pico/Pico 2 GPIO 9 -> transceiver `RO`
- Pico/Pico 2 GPIO 10 -> transceiver `DE` and `RE`
- Pico/Pico 2 `3V3` -> transceiver `VCC`
- Pico/Pico 2 `GND` -> transceiver `GND`
- Transceiver `A/B` -> BACnet MS/TP trunk

No external hardware is needed for the automated CI build because the CI job only verifies that the Pico MS/TP target compiles.

## Software Dependencies

The external dependency required for this port is cloned by `ports/pico/configure.sh` into `ports/pico/external/`:

- Raspberry Pi Pico SDK 2.2.0
  URL: <https://github.com/raspberrypi/pico-sdk>

The script checks out that SDK tag and initializes the SDK submodules needed by the build.

## Build

From the repository root:

```sh
make pico-cmake
```

That target runs `ports/pico/configure.sh`, configures the CMake build in `ports/pico/build`, and builds the Pico port for `pico2`.

To select another supported board, configure manually and pass `PICO_BOARD`:

```sh
cd ports/pico
./configure.sh
cmake -S . -B build -DPICO_SDK_PATH="$PWD/external/pico-sdk" -DPICO_BOARD=pico2
cmake --build build -- -j"$(nproc)"
```

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

These BACnet/IP hooks are examples of the interface you need to provide. The repository does not yet include a complete in-tree Pico Ethernet or Wi-Fi transport implementation, so BACnet/IP still requires board-specific network code behind `bip.h`.

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

For a minimal MS/TP example for this port, see `main.c`.
The RS485 driver is ready to use without modification. It uses the Pico SDK UART functions.
