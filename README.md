# BACnet Stack 

BACnet open source protocol stack for embedded systems, Linux, and Windows
http://bacnet.sourceforge.net/

Welcome to the wonderful world of BACnet and true device interoperability!

Continuous Integration
----------------------

This library uses various automated continuous integration services 
to assist in automated compilation, validation, linting, and unit testing 
of robust C code and BACnet functionality.

[![Actions Status](https://github.com/bacnet-stack/bacnet-stack/workflows/CMake/badge.svg)](https://github.com/bacnet-stack/bacnet-stack/actions) GitHub Workflow

[![Build Status](https://travis-ci.com/bacnet-stack/bacnet-stack.svg?branch=master)](https://travis-ci.com/bacnet-stack/bacnet-stack) Travis CI

[![Build status](https://ci.appveyor.com/api/projects/status/5lq0d9a69g7ixskm/branch/master?svg=true)](https://ci.appveyor.com/project/skarg/bacnet-stack/branch/master) AppVeyor CI

About this Project
------------------

This BACnet library provides a BACnet application layer, network layer and
media access (MAC) layer communications services for an embedded system.

BACnet - A Data Communication Protocol for Building Automation and Control
Networks - see bacnet.org. BACnet is a standard data communication protocol for
Building Automation and Control Networks. BACnet is an open protocol, which
means anyone can contribute to the standard, and anyone may use it. The only
caveat is that the BACnet standard document itself is copyrighted by ASHRAE,
and they sell the document to help defray costs of developing and maintaining
the standard (just like IEEE or ANSI or ISO).

For software developers, the BACnet protocol is a standard way to send and
receive messages containing data that are understood by other BACnet
compliant devices. The BACnet standard defines a standard way to communicate
over various wires or radios, known as Data Link/Physical Layers: Ethernet, 
EIA-485, EIA-232, ARCNET, and LonTalk. The BACnet standard also defines a 
standard way to communicate using UDP, IP, HTTP (Web Services), and Websockets.

This BACnet protocol stack implementation is specifically designed for the
embedded BACnet appliance, using a GPL with exception license (like eCos),
which means that any changes to the core code that are distributed are shared, 
but the BACnet library can be linked to proprietary code without the proprietary 
code becoming GPL. Note that some of the source files are designed as 
skeleton or example or template files, and are not copyrighted as GPL.

The text of the GPL exception included in each source file is as follows: 

"As a special exception, if other files instantiate templates or use macros or
inline functions from this file, or you compile this file and link it with
other works to produce a work based on this file, this file does not by itself
cause the resulting work to be covered by the GNU General Public License.
However the source code for this file must still be made available in
accordance with section (3) of the GNU General Public License."

The code is written in C for portability, and includes unit tests (PC based
unit tests). Since the code is designed to be portable, it compiles with GCC as
well as other compilers, such as Clang or IAR.

The BACnet protocol is an ASHRAE/ANSI/ISO standard, so this library adheres to
that standard. BACnet has no royalties or licensing restrictions, and
registration for a BACnet vendor ID is free.

What the code does
------------------

For an overview of this library architecture and how to use it, see
https://sourceforge.net/p/bacnet/src/ci/master/tree/doc/README.developer

This stack includes unit tests that can be run using the Makefile in the
project root directory "make test".
The unit tests can also be run using individual make invocations. 
The unit tests run a PC and continue to do so with 
every commit within the Continuous Integration environment.

The BACnet stack was functionally tested using a variety of tools
as well as various controllers and workstations. It has been included
in many products that successfully completed BTL testing.

Using the Makefile in the project root directory, a dozen sample applications
are created that run under Windows or Linux. They use the BACnet/IPv4 datalink
layer for communication by default, but could be compiled to use BACnet IPv6, 
Ethernet, ARCNET, or MS/TP.

Linux/Unix/Cygwin

    $ make clean all

Windows MinGW Bash

    $ make win32

Windows Command Line

    c:\> build.bat

The BACnet stack can be compiled by a variety of compilers.  The most common
free compiler is GCC (or MinGW under Windows).  The makefiles use GCC by
default.

The library is also instrumented to use [CMake](https://cmake.org/) which can
generate a project or Makefiles for a variety of IDE or compiler. For example,
to generate a Code::Blocks project:

    $ mkdir build
    $ cd build/
    $ cmake .. -G"CodeBlocks - Unix Makefiles"
    
    c:\> mkdir build
    c:\> cd build/
    c:\> cmake .. -G"CodeBlocks - MinGW Makefiles"

The demo applications are all client applications that provide one main BACnet
service, except the one server application and one gateway application, 
a couple router applications, and a couple of MS/TP specific applications.
Each application will accept command line parameters, and prints the output to 
stdout or stderr.  The client applications are command line based and can 
be used in scripts or for troubleshooting.  
The demo applications make use of environment variables to 
setup the network options.  See each individual demo for the options.

There are also projects in the ports/ directory for ARM7, AVR, RTOS-32, PIC, 
and others.  Each of those projects has a demo application for specific hardware.
In the case of the ARM7 and AVR, their makefile works with GCC compilers and
there are project files for IAR Embedded Workbench and Rowley Crossworks for ARM.

Project Documentation
---------------------

The project documentation is in the doc/ directory.  Similar documents are
on the project website at <http://bacnet.sourceforge.net/>.

Project Mailing List and Help
-----------------------------

If you want to contribute to this project and have some C coding skills,
join us via https://github.com/bacnet-stack/bacnet-stack/
or via https://sourceforge.net/p/bacnet/src/ and create a
fork or branch, and eventually a pull request to have 
your code considered for inclusion.

If you find a bug in this project, please tell us about it at
https://sourceforge.net/p/bacnet/bugs/
or
https://github.com/bacnet-stack/bacnet-stack/issues

If you have a support request, you can post it at 
https://sourceforge.net/p/bacnet/support-requests/

If you have a feature request, you can post it at
https://sourceforge.net/p/bacnet/feature-requests/

If you have a problem getting this library to work for
your device, or have a BACnet question, join the developers mailing list at:
http://lists.sourceforge.net/mailman/listinfo/bacnet-developers
or post the question to the Open Discussion, Developers, or Help forums at
https://sourceforge.net/p/bacnet/discussion/

I hope that you get your BACnet Device working!

Steve Karg, Birmingham, Alabama USA
skarg@users.sourceforge.net

ASHRAE® and BACnet® are registered trademarks of the 
American Society of Heating, Refrigerating and Air-Conditioning Engineers, Inc.
1791 Tullie Circle NE, Atlanta, GA 30329.
