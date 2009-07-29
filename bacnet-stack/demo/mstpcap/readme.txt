MSTP Capture Tool

This tool captures MS/TP packets on an RS485 serial interface,
and saves the packets to a file in Wireshark PCAP format for 
the MS/TP dissector to read.  The filename has a date and time
code in it, and will contain up to 65535 packets.  A new file
will be created at each 65535 packet interval.

Here is a sample of the tool running (use CTRL-C to quit).
D:\code\bacnet-stack\bin>mstpcap COM3 38400
Adjusted interface name to \\.\COM3
mstpcap: Using \\.\COM3 for capture at 38400 bps.
mstpcap: saving capture to mstp_20090729123548.cap
1400 packets
MAC     MaxMstr Tokens  Retries Treply  Tusage  Trpfm   Tder    Tpostpd
0       0       525     0       32      0       0       0       0
1       127     525     0       16      79      0       0       0

MS/TP capture tool also includes statistics which are listed for
any MAC addresses found passing a token, 
or any MAC address replying to a DER message. 
The statistics are emitted when CTRL-C is pressed, or when
65535 packets are captured and the new file is created.
The statistics are cleared when the new file is created. 

MaxMstr = highest destination MAC during PFM
Tokens = number of tokens transmitted
Retries = number of second tokens sent to this MAC
Treply = max milliseconds it took to reply with a token after receiving a token
Tusage = max Tusage_delay in milliseconds based on PFM and subsequent token
Trpfm = max milliseconds to respond to PFM with RPFM.
Tder = max milliseconds to respond to DER request with DNER
Tpostpd = max milliseconds to respond to DER request with Reply Postponed.
