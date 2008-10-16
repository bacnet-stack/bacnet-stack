@echo off
echo Build with MinGW mingw.sourceforge.net
echo Note: copy mingw32-make.exe to make.exe to build for Win32
make BACNET_PORT=win32 OPTIMIZATION=-Os DEBUGGING= clean all
rem Build for MinGW debug
rem make BACNET_PORT=win32 clean all

rem On Linux, install mingw32 and use this:
rem make BACNET_PORT=win32 OPTIMIZATION=-Os DEBUGGING= CC=i586-mingw32msvc-gcc AR=i586-mingw32msvc-ar clean all 
