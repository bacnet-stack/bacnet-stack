/**************************************************************************
*
* Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/

#ifndef SERVER_H_
#define SERVER_H_

/** @file server/server.h  Header for example server (ie, BACnet Device) 
 *        using the BACnet Stack */

/** @defgroup Demos Demos of Servers and Clients
 * Most of the folders under the /demo folder contain demonstration (ie, sample) 
 * code that implements the name functionality.
 * 
 * The exceptions to this general rule, /handler and /object folders, are 
 * described in the various BIBBs and Object Framework modules. 
 */

/** @defgroup ServerDemo Demo of a BACnet Server (Device)
 * @ingroup Demos
 * This is a basic demonstration of a simple BACnet Device consisting of
 * the services and properties shown in its PICS (output provided by
 * the demo/epics/epics program):
 * @verbatim
List of Objects in test device:                                                                                                     
{                                                                                                                                   
    object-identifier: (Device, 1234)                                                                                               
    object-name: "SimpleServer"                                                                                                     
    object-type: Device                                                                                                             
    system-status: operational                                                                                                      
    vendor-name: "BACnet Stack at SourceForge"
    vendor-identifier: 260
    model-name: "GNU"
    firmware-version: "0.5.5"
    application-software-version: "1.0"
    protocol-version: 1
    protocol-revision: 5
    protocol-services-supported: {                                             
        false,false,false,false,   # ,,,,
        false, true, true, true,   # , Subscribe-COV, Atomic-Read-File, Atomic-Write-File,
        false,false,false,false,   # ,,,,
         true,false, true, true,   #  Read-Property,, Read-Property-Multiple, Write-Property,
        false, true,false,false,   # , Device-Communication-Control,,,
         true,false,false,false,   #  Reinitialize-Device,,,,
        false,false,false,false,   # ,,,,
         true,false,false,false,   #  COV-Notification,,,,
         true, true, true,false,   #  Time-Synchronization, Who-Has, Who-Is,,
         true,false,false,false    #  UTC-Time-Synchronization,,,,
        }
    protocol-object-types-supported: {
         true, true, true, true,   #  Analog Input, Analog Output, Analog Value, Binary Input,
         true, true,false,false,   #  Binary Output, Binary Value,,,
         true,false, true,false,   #  Device,, File,,
        false, true, true,false,   # , Multi-State Input, Multi-State Output,,
        false,false,false,false,   # ,,,,
         true, true,false,false,   #  Trendlog, Life Safety Point,,,
        false,false,false,false,   # ,,,,
         true,false,false,false,   #  Load-Control,,,,
        false,false,false,false,   # ,,,,
        false,false    # ,,
        }
    object-list: {(Device, 1234),(Analog Input, 0),(Analog Input, 1),
        (Analog Input, 2),(Analog Input, 3),(Analog Output, 0),(Analog Output, 1),
        (Analog Output, 2),(Analog Output, 3),(Analog Value, 0),(Analog Value, 1),
        (Analog Value, 2),(Analog Value, 3),(Binary Input, 0),(Binary Input, 1),
        (Binary Input, 2),(Binary Input, 3),(Binary Input, 4),(Binary Output, 0),
        (Binary Output, 1),(Binary Output, 2),(Binary Output, 3),(Binary Value, 0),
        (Binary Value, 1),(Binary Value, 2),(Binary Value, 3),(Binary Value, 4),
        (Binary Value, 5),(Binary Value, 6),(Binary Value, 7),(Binary Value, 8),
        (Binary Value, 9),(Life Safety Point, 0),(Life Safety Point, 1),(Life Safety Point, 2),
        (Life Safety Point, 3),(Life Safety Point, 4),(Life Safety Point, 5),(Life Safety Point, 6),
        (Load-Control, 0),(Load-Control, 1),(Load-Control, 2),(Load-Control, 3),
        (Multi-State Output, 0),(Multi-State Output, 1),(Multi-State Output, 2),(Multi-State Output, 3),
        (Multi-State Input, 0),(Trendlog, 0),(Trendlog, 1),(Trendlog, 2),
        (Trendlog, 3),(Trendlog, 4),(Trendlog, 5),(Trendlog, 6),
        (Trendlog, 7),(File, 0),(File, 1),(File, 2)}
    max-apdu-length-accepted: 1476
    segmentation-supported: no-segmentation
    apdu-timeout: 3000
    number-of-APDU-retries: 3
    device-address-binding: Null
    database-revision: 1
}   @endverbatim
 */

#endif /* SERVER_H_ */
