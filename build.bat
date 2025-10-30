@echo off
echo Build with MinGW32 and MSYS: mingw.sourceforge.net
set PATH=C:\MinGW\msys\1.0\bin;C:\MinGW\bin;%PATH%
rem assumes rm, cp, size are already in path
set CC=mingw32-gcc.exe
set AR=mingw32-gcc-ar.exe
set NM=mingw32-gcc-nm.exe
set OBJCOPY=objcopy.exe
set SIZE=size.exe
set MAKE=mingw32-make.exe

doskey make = mingw32-make.exe $*

make BACNET_PORT=win32 clean
make BACNET_PORT=win32 BUILD=release clean all

rem Build for MinGW debug
rem make BACNET_PORT=win32 BUILD=debug clean all

rem Build for MinGW MS/TP
rem make BACNET_PORT=win32 BACDL_DEFINE=-DBACDL_MSTP=1 clean all
rem make BACNET_PORT=win32 BACDL_DEFINE=-DBACDL_BIP6=1 clean all

rem On Linux, install mingw32 and use this:
rem make BACNET_PORT=win32 CC=i586-mingw32msvc-gcc AR=i586-mingw32msvc-ar clean all
