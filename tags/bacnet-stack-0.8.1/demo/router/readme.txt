-----------------------
1. About
-----------------------

The Router connects two or more BACnet/IP and BACnet MS/TP networks.
Number of netwoks is limited only by available hardware communication devices (or ports for Ethernet).

-----------------------
2. License
-----------------------

Copyright (C) 2012  Andriy Sukhynyuk, Vasyl Tkhir, Andriy Ivasiv

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.


-----------------------
3. Build
-----------------------

1. Download, build and install libconfig C/C++ Configuration File Library from http://www.hyperrealm.com/libconfig
2. Set variable "BACNET_PORT" in library root diredory Makefile to linux
3. Run "make clean all" from library root directory

-----------------------
4. Router configuration
-----------------------

4.1. Configuration file format.

//single line comment

/*
	multiline comment
*/

ports =
(
	//route_1
	{
		device_type = "<value>";
		//route specific arguments, see below
	},

	//route_2
	{
		device_type = "<value>";
		//route specific arguments, see below
	},

	//.....

	//route_n
	{
		device_type = "<value>";
		//route specific arguments, see below
	}
);

Note:	- arguments are separeted with ';'
	- routes are separeted with ','
	- no ',' after the last route

4.2. Configuration file arguments.

Common arguments:
	device_type	- Describes a type of route, may be "bip" (Etherent) or "mstp" (Serial port). Use quotes.
	device		- Connection device, for example "eth0" or "/dev/ttyS0"; default values: for BIP:"eth0", for MSTP: "/dev/ttyS0". Use quotes.
	network		- Network number [1..65534]. Do not use network number 65535, it is broadcast number; default begins from 1 to routes count.

bip arguments:
	port 		- bip UDP port; default port is 47808 (0xBAC0).

mstp arguments:
	mac		- MSTP MAC; default value is 127.
	max_master	- MSTP max master; default value is 127.
	max_frames	- 1. Segmentation does not supported.
	baud		- one from the list: 0, 50, 75, 110, 134, 150, 200, 300, 600, 1200, 1800, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400; default baud is 9600
	parity		- one from the list (with quotes): "None", "Even", "Odd"; default parity "None". Use quotes.
	databits	- one from the list: 5, 6, 7, 8; default 8.
	stopbits	- 1 or 2; default 1.

4.3. Example of configuration file.

	ports = 
	(
		{
			device_type = "bip";
			device = "eth0";
			port = 47808;
			network = 1;
		},
		{
			device_type = "bip";
			device = "eth1";
			port = 47808;
			network = 2;
		},
		{
			device_type = "bip";
			device = "eth1";
			port = 47809;
			network = 3;
		},		
		{
			device_type = "mstp";
			device = "/dev/ttyS0";
			mac = 1;
			max_master = 127;
			max_frames = 1;
			baud = 38400;
			parity = "None";
			databits = 8;
			stopbits = 1;
			network = 4;
		}
	);


-----------------------
5. Start
-----------------------

1. Copy configuration file in the router executable directory
2. Start the router with "router -c init.cfg" command in terminal


