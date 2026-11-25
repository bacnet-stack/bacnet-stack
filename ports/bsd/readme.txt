This is a port to MAC OS X or FreeBSD for testing.

Install on FreeBSD 15
sudo pkg install cmake git libdispatch
git clone https://github.com/bacnet-stack/bacnet-stack.git
cd bacnet-stack
mkdir build
cd build
cmake ..
make

QNX 8.0 (QNX SDP 8.0) now has its own port in ports/qnx (MS/TP and BACnet/IP);
ports/qnx/bip-init.c reuses the BACnet/IP code from this port.
Cross-compile from a Linux host with QNX SDP 8.0 installed, for an aarch64le
target:

source ~/qnx800/qnxsdp-env.sh
make BACNET_PORT=qnx BACDL=bip CC="qcc -Vgcc_ntoaarch64le" AR=ntoaarch64-ar apps

For an x86_64 target use -Vgcc_ntox86_64 and AR=ntox86_64-ar. Run the
applications with BACNET_IFACE set to the QNX interface, e.g.
BACNET_IFACE=bcm0 ./bacwi
