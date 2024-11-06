# Win32 ports for BACnet

This directory contains a demo program that compiles with a Win32 compiler.
It was tested with the free MinGW32 MSYS2 GCC compiler
and the (free) Microsoft Visual Studio Community Edition

## Building with Makefile under MinGW32

Note: Building for 64-bit with MinGW64 requires extra runtimes,
whereas building for 32-bit as Win32 doesn't require any other
files for the EXE to work in any version of Windows.

* MSYS2 installation is (from powershell):

    c:\> winget install --id=MSYS2.MSYS2  -e

* Start MSYS shell, and install MinGW32.

    $ pacman -Syu mingw-w64-i686-toolchain

* Edit ~/.bashrc file and add:

    alias make=mingw32-make.exe

* Exit MSYS.

* Start MSYS profile for MinGW32.

* Verify MSYS profile:

    $ uname

    MINGW32_NT-10.0-19045

* Verify GCC is targeting i686:

    $ gcc -dumpmachine

    i686-w64-mingw32

* Verify make is built for Windows32:

     make --version

     Built for Windows32

## Building and running with Microsoft Visual Studio

* Obtain the (free) Microsoft Visual Studio Community Edition, or use your professional version. Currently tested to MSVC 2022
* Open solution file ports/win32/Microsoft Visual Studio/bacnet-stack.sln
* Set startup project to be the desired application project, e.g. "server"
* In the active project properties, debugging, set command line to desired DeviceID
* In active project properties, debugging, set environment variables as appropriate, e.g. BACNET_IFACE=10.59.2.1 BACNET_IP_PORT=53004
* Compile & run
* Questions? edward@bac-test.com
