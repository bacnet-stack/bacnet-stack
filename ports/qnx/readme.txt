QNX Neutrino (QNX SDP 8.0) port

Datalinks: BACnet MS/TP (BACDL=mstp) and BACnet/IP (BACDL=bip).

MS/TP uses the devc-ser* serial drivers (/dev/ser1, /dev/serusb1, ...):
 - rs485.c: POSIX termios with numeric speeds (QNX speed_t is the baud
   rate, so 76800 works without BOTHER), readcond() for non-blocking
   receive, clock_nanosleep(TIMER_ABSTIME) for Tturnaround, and optional
   RS-485 transmitter-enable on RTS via devctl(DCMD_CHR_SERCTL) for
   transceivers without automatic direction control.
 - dlmstp.c: the MS/TP state machine thread runs SCHED_FIFO at priority 60
   by default (usable without root, since non-root processes are limited to
   priorities below 64); override with BACNET_MSTP_PRIORITY.
BACnet/IP reuses ports/bsd/bip-init.c, since io-sock is derived from FreeBSD.

Build (Linux host with QNX SDP 8.0 installed), x86_64 target:

source ~/qnx800/qnxsdp-env.sh
make BACNET_PORT=qnx BACDL=mstp CC="qcc -Vgcc_ntox86_64" AR=ntox86_64-ar \
    apps modbus-gateway-mstp

For aarch64le (e.g. Raspberry Pi 5) use -Vgcc_ntoaarch64le and
AR=ntoaarch64-ar. CMake builds select this port when CMAKE_SYSTEM_NAME is
QNX (e.g. with the QNX toolchain file from qnx-ports/build-files).

Run on the target (devc-ser8250 must be serving the port):

BACNET_IFACE=/dev/ser2 BACNET_MSTP_MAC=1 BACNET_MAX_MASTER=127 \
BACNET_MSTP_BAUD=38400 ./bacserv 1234

Environment variables specific to this port:
 BACNET_RS485_RTS=1      drive the transmitter enable from RTS
 BACNET_MSTP_PRIORITY=n  MS/TP thread priority (default 60)
 BACNET_RS485_STATS=1    print frame counts and the Tturnaround target vs
                         achieved delay (microseconds) at exit, or on
                         SIGUSR1 (slay -s SIGUSR1 <process>)

The MS/TP files also build on a POSIX host (non-QNX branches), which allows
host-only testing over a pseudo-terminal pair:
make BACNET_PORT=linux BACNET_PORT_DIRNAME=qnx BACDL=mstp apps
