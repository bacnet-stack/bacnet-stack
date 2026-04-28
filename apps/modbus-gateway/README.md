# Modbus RTU ↔ BACnet Gateway

A generic gateway that bridges **Modbus RTU** devices to a **BACnet** network.
It reads Modbus registers/coils from one or more slaves and exposes them as BACnet
Analog Input (AI), Analog Output (AO), Binary Input (BI), or Binary Output (BO)
objects — all configured via a single JSON file at runtime.

The BACnet datalink type is selected at runtime via `gateway_config.json`
(`"mstp"`, `"bip"`, `"bip6"`, `"ethernet"`), so no recompilation is needed
when switching between datalinks.

Tested on Raspberry Pi (Raspberry Pi OS), but portable to any Linux/POSIX platform.

---

## Hardware Setup

```
  [Modbus RTU devices]──RS-485──[RS-485↔USB #1  /dev/ttyUSB1]──┐
                                                                ├── Host (e.g. Raspberry Pi)
  [BACnet MS/TP bus  ]──RS-485──[RS-485↔USB #0  /dev/ttyUSB0]──┘
```

> When using **BACnet/IP**, only one serial port (for Modbus) is needed.
> The BACnet side communicates over the existing Ethernet/Wi-Fi interface.

| Role                    | Default                  | Default Baud |
|-------------------------|--------------------------|--------------|
| BACnet MS/TP (master)   | `/dev/ttyUSB0`           | 38400 bps    |
| BACnet/IP               | network interface (eth0) | —            |
| Modbus RTU   (master)   | `/dev/ttyUSB1`           | 9600 bps     |

---

## Build

### Prerequisites (Debian / Raspberry Pi OS)

```bash
sudo apt update
sudo apt install -y build-essential
```

No external libraries are required. The gateway uses the bundled `cJSON` parser and
the bacnet-stack data-link layer that is already part of this repository.

### Compile

```bash
# From the repository root – default datalink (BACnet/IP)
make modbus-gateway-clean
make modbus-gateway

# MS/TP specific build
make modbus-gateway-mstp-clean
make modbus-gateway-mstp
```

Output binary: `bin/modbusgw, apps/modbus-gateway/modbusgw` (or `bin/modbusgw-mstp, apps/modbus-gateway/modbusgw-mstp` for the MS/TP specific build)

---

## Configuration — `gateway_config.json`

All runtime settings are read from `gateway_config.json` (in the working directory
by default). Pass a different path as the first CLI argument if needed.

```json
{
  "bacnet": {
    "device_instance": 50001,
    "device_name":     "Modbus-Gateway",
    "vendor_id":       260,

    "datalink_type":   "mstp",

    "iface":           "/dev/ttyUSB0",

    "mstp_baud":       38400,
    "mstp_mac":        1,
    "mstp_max_master": 5,
    "mstp_net":        2,

    "bip_port":        47808
  },

  "modbus": {
    "port":             "/dev/ttyUSB1",
    "baud":             9600,
    "poll_interval_sec": 5
  },

  "points": [
    {
      "name":             "Temperature",
      "description":      "Room Temperature",
      "enabled":          true,
      "bacnet_type":      "AI",
      "bacnet_instance":  0,
      "units":            "DEGREES_CELSIUS",
      "modbus_slave":     1,
      "modbus_func":      "HOLDING_REGISTER",
      "modbus_address":   1,
      "scale":            0.1,
      "offset":           0.0,
      "range_min":       -40.0,
      "range_max":        125.0
    },
    {
      "name":             "Humidity",
      "description":      "Relative Humidity",
      "enabled":          true,
      "bacnet_type":      "AI",
      "bacnet_instance":  1,
      "units":            "PERCENT_RELATIVE_HUMIDITY",
      "modbus_slave":     1,
      "modbus_func":      "HOLDING_REGISTER",
      "modbus_address":   0,
      "scale":            0.1,
      "offset":           0.0,
      "range_min":        0.0,
      "range_max":        100.0
    }
  ]
}
```

### `bacnet` section

| Key               | Type    | Description                                                      |
|-------------------|---------|------------------------------------------------------------------|
| `device_instance` | uint32  | BACnet Device Object Instance (1 – 4194302)                      |
| `device_name`     | string  | BACnet Device Object Name                                        |
| `vendor_id`       | uint16  | BACnet Vendor Identifier                                         |
| `datalink_type`   | string  | Datalink type: `"mstp"`, `"bip"`, `"bip6"`, `"ethernet"`        |
| `iface`           | string  | Serial port for MS/TP (e.g. `/dev/ttyUSB0`) or network interface for BACnet/IP (e.g. `eth0`) |
| `mstp_baud`       | uint32  | MS/TP baud rate (e.g. 9600, 19200, 38400, 76800)                 |
| `mstp_mac`        | uint8   | MS/TP MAC address of this node (0 – 127)                         |
| `mstp_max_master` | uint8   | Highest MAC address polled in the token ring                     |
| `mstp_net`        | uint16  | BACnet network number (0 = local)                                |
| `bip_port`        | uint16  | UDP port for BACnet/IP (default: 47808)                          |

### `modbus` section

| Key                | Type   | Description                                      |
|--------------------|--------|--------------------------------------------------|
| `port`             | string | Serial port for Modbus RTU                       |
| `baud`             | uint32 | Baud rate                                        |
| `poll_interval_sec`| int    | Seconds between Modbus polling cycles            |

### `points` array — per-point fields

| Key               | Type    | Description                                                         |
|-------------------|---------|---------------------------------------------------------------------|
| `name`            | string  | BACnet Object Name (must be unique)                                 |
| `description`     | string  | BACnet Description property                                         |
| `enabled`         | bool    | `false` skips this point entirely                                   |
| `bacnet_type`     | string  | `"AI"`, `"AO"`, `"BI"`, or `"BO"`                                  |
| `bacnet_instance` | uint32  | BACnet Object Instance number                                       |
| `units`           | string  | BACnet Engineering Units name (e.g. `"DEGREES_CELSIUS"`)           |
| `modbus_slave`    | uint8   | Modbus slave ID (1 – 247)                                           |
| `modbus_func`     | string  | `"COIL"`, `"DISCRETE_INPUT"`, `"HOLDING_REGISTER"`, `"INPUT_REGISTER"` |
| `modbus_address`  | uint16  | Modbus register / coil address (0-based)                            |
| `scale`           | float   | Multiplier applied to the raw value: `eng = raw × scale + offset`  |
| `offset`          | float   | Offset added after scaling                                          |
| `range_min/max`   | float   | Out-of-range check; sets Out-of-Service if violated                 |

> **Compile-time defaults** (used when `gateway_config.json` is absent or a key is
> missing) are defined in `gateway_config.h`.

---

## Running

```bash
# Use gateway_config.json in the current directory
sudo ./modbusgw

# Specify a different config file
sudo ./modbusgw /etc/modbus-gateway/my_config.json

# Override Device Instance at the command line
sudo ./modbusgw 12345

# Both config file and instance override
sudo ./modbusgw /etc/modbus-gateway/my_config.json 12345
```

> To run without `sudo`, add your user to the `dialout` group:
> ```bash
> sudo usermod -aG dialout $USER
> # Log out and back in for the change to take effect.
> ```

---

## Operation Flow

```
[Startup]
  │
  ├─ Parse CLI arguments (config path, device instance override)
  ├─ Load gateway_config.json  →  BACnet & Modbus settings + point table
  ├─ Initialize BACnet datalink  (mstp / bip / bip6 / ethernet via dlenv_init)
  ├─ Create BACnet objects from point table  (AI / AO / BI / BO)
  ├─ Initialize Modbus RTU port
  ├─ Broadcast I-Am
  ├─ Initial Modbus poll
  │
  └─ [Main loop] ────────────────────────────────────────────────┐
       │                                                         │
       ├─ datalink_receive()                                     │
       │     Receive BACnet frames → npdu_handler()             │
       │                                                         │
       ├─ 1-second BACnet timers                                 │
       │     dcc_timer / tsm_timer / Device_Timer                │
       │                                                         │
       └─ Modbus poll  (every poll_interval_sec seconds)         │
             Read registers/coils for each enabled point         │
             → Update BACnet Present_Value                       │
             → On failure: set Out-of-Service = true ────────────┘
```

---

## File Overview

| File                   | Description                                                      |
|------------------------|------------------------------------------------------------------|
| `gateway_config.json`  | Runtime configuration (BACnet, Modbus, point table)             |
| `gateway_config.h`     | Compile-time defaults (fallback when JSON key is absent)        |
| `point_table.h/c`      | JSON loader and Modbus-to-BACnet point mapping engine           |
| `modbus_rtu.h/c`       | Modbus RTU master driver (CRC-16, FC01/02/03/04)                |
| `main.c`               | Gateway main loop                                               |
| `../../src/bacnet/basic/sys/cJSON.h/c`            | Bundled cJSON library for JSON parsing                          |
| `Makefile`             | Build script                                                    |
