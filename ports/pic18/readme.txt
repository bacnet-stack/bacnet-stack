BACnet Stack - SourceForge.net
Build for MPLAB IDE

These are some settings that are important when building 
the BACnet Stack using MPLAB IDE and MCC18 Compiler,

1. Add the files to the project that you need:
abort.c, apdu.c, bacapp.c, bacdcode.c, bacerror.c,
bacstr.c, bigend.c, crc.c, datalink.c, dcc.c, dlmstp.c,
ima.c, mstp.c, npdu.c, rd.c, reject.c, reject.c,
ringbuf.c, rp.c, whois.c, wp.c

From demo/object/: device.c or dev_tiny.c, ai.c, ao.c, etc.

From demo/handler/: h_dcc.c, h_rd.c, h_rp.c, h_wp.c

2. Project->Options->Project

General Tab: Include Path:
C:\code\bacnet-stack\;C:\code\bacnet-stack\demo\handler\;C:\code\bacnet-stack\demo\object\;C:\code\bacnet-stack\ports\pic18\

MPLAB C18 Tab: Memory Model:
Code: Large Code Model
Data: Large Data Model
Stack: Multi-bank Model

MPLAB C18 Tab: General: Macro Definitions:
PRINT_ENABLED=0
BACDL_MSTP=1
BIG_ENDIAN=0
