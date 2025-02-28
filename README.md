# NMC4 Desktop build

Add this to install into your local toolchain dir as sudo:
set (CMAKE_INSTALL_PREFIX "/opt/poky/rzn1/sysroots/x86_64-pokysdk-linux/usr/")

Add DEBUG_PRINT compile flag to compile_options to print out debug messages, e.g.:
add_compile_options(-DDEBUG_PRINT -DMAX_ANALOG_VALUES=32 -DMAX_ANALOG_INPUTS=32)

# BACnet Stack

BACnet open source protocol stack C library for embedded systems,
Linux, MacOS, BSD, and Windows
http://bacnet.sourceforge.net/

Welcome to the wonderful world of BACnet and true device interoperability!

Continuous Integration
----------------------

This library uses automated continuous integration services
to assist in automated compilation, validation, linting, security scanning,
and unit testing to produce robust C code and BACnet functionality.

[![Actions Status](https://github.com/bacnet-stack/bacnet-stack/workflows/CMake/badge.svg)](https://github.com/bacnet-stack/bacnet-stack/actions/workflows/main.yml) GitHub Workflow: CMake build of library and demo apps on Ubuntu, Windows and MacOS

[![Actions Status](https://github.com/bacnet-stack/bacnet-stack/workflows/GCC/badge.svg)](https://github.com/bacnet-stack/bacnet-stack/actions/workflows/gcc.yml) GitHub Workflow: Ubuntu Makefile GCC build of library, BACnet/IP demo apps with and without BBMD, BACnet/IPv6, BACnet Ethernet, and BACnet MSTP demo apps, gateway, router, router-ipv6, router-mstp, ARM ports (STM, Atmel), AVR ports, and BACnet/IP demo apps compiled with MinGW32.

[![Actions Status](https://github.com/bacnet-stack/bacnet-stack/workflows/Quality/badge.svg)](https://github.com/bacnet-stack/bacnet-stack/actions/workflows/lint.yml) GitHub Workflow: scan-build (LLVM Clang Tools), cppcheck, codespell, unit tests and code coverage.

[![Actions Status](https://github.com/bacnet-stack/bacnet-stack/workflows/CodeQL/badge.svg)](https://github.com/bacnet-stack/bacnet-stack/actions/workflows/codeql-analysis.yml) GitHub Workflow CodeQL Analysis

About this Project
------------------

This BACnet protocol stack library provides a BACnet application layer,
network layer and media access (MAC) layer communications services.
It is an open source, royalty-free library for a RTOS or bare metal
embedded system, or full OS such as Windows, Linux, MacOS, or BSD.

BACnet - A Data Communication Protocol for Building Automation and Control
Networks - see [bacnet.org](bacnet.org). BACnet is a standard data
communication protocol for Building Automation and Control Networks.
BACnet is an open protocol, which means anyone can contribute to the
standard, and anyone may use it. The only caveat is that the BACnet
standard document itself is copyrighted by ASHRAE, and they sell the
document to help defray costs of developing and maintaining
the standard (similar to IEEE or ANSI or ISO).

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
code becoming GPL. The text of the GPL exception included in each source
file is as follows:

     * SPDX-License-Identifier: GPL-2.0-or-later WITH GCC-exception-2.0

Note that some of the source files are designed as skeleton or example
or template files, and are not copyrighted as GPL. The text of the license
for these files is designated in each source file as follows:

     * SPDX-License-Identifier: MIT
     * SPDX-License-Identifier: Apache-2.0

A software bill-of-materials can be generated using grep:

    $ grep -nrw SPDX --include=*.[c,h]

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

The unit tests also use CMake and may be run with the command sequence:

    $ make test

The unit test framework uses a slightly modified ztest, and the tests are located
in the test/ folder.  The unit test builder uses CMake, and the test coverage
uses LCOV.  The HTML results of the unit testing coverage are available starting
in the test/build/lcoverage/index.html file.

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
180 Technology Parkway NW, Peachtree Corners, Georgia 30092 US.
