BACnet MS/TP Capture Tool

This tool captures BACnet MS/TP packets on an RS485 serial interface,
and saves the packets to a file in Wireshark PCAP format for
the BACnet MS/TP dissector to read.  The filename has a date and time
code in it, and will contain up to 65535 packets.  A new file
will be created at each 65535 packet interval.  The tool can
be stopped by using Control-C.

Here is a sample of the tool running (use CTRL-C to quit).
D:\code\bacnet-stack\bin>mstpcap COM3 38400
Adjusted interface name to \\.\COM3
mstpcap: Using \\.\COM3 for capture at 38400 bps.
mstpcap: saving capture to mstp_20090729123548.cap
1400 packets
MAC     MaxMstr Tokens  Retries Treply  Tusage  Trpfm   Tder    Tpostpd
0       0       525     0       32      0       0       0       0
1       127     525     0       16      79      0       0       0

The BACnet MS/TP capture tool also includes statistics which are
listed for any MAC addresses found passing a token,
or any MAC address replying to a DER message.
The statistics are emitted when Control-C is pressed, or when
65535 packets are captured and the new file is created.
The statistics are cleared when the new file is created.

MaxMstr = highest destination MAC address during PFM

Tokens = number of tokens transmitted by this MAC address.

Retries = number of second tokens sent to this MAC address.

Treply = maximum number of milliseconds it took to reply with
a token after receiving a token. Treply is required to be less
than 25ms (but the mstpcap tool may not have that good of
resolution on Windows).

Tusage = the maximum number of milliseconds the
device waits for a ReplyToPollForMaster or Token retry.
Tusage is required to be between 20ms and 100ms.

Trpfm = maximum number of milliseconds to respond to PFM with RPFM.  It is
required to be less than 25ms.

Tder = maximum number of milliseconds that a device takes to
respond to a DataExpectingReply request.  Tder is required to be less
than 250ms.

Tpostpd = maximum number of milliseconds to respond to
DataExpectingReply request with ReplyPostponed.  Tpostpd is
required to be less than 250ms.

Note that the mstpcap tool may not have that good of
resolution on Windows, so timing under 50ms may not be accurate.

