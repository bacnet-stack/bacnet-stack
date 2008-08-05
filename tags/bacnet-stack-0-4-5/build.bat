@echo off
rem Build for Borland 5.5 tools
rem set BORLAND_DIR=c:\borland\bcc55
rem %BORLAND_DIR%\bin\make -f makefile.b32 clean
rem %BORLAND_DIR%\bin\make -f makefile.b32 all

rem Build for MinGW
make BACNET_PORT=win32 OPTIMIZATION=-Os DEBUGGING= clean all
rem Build for MinGW debug
rem make BACNET_PORT=win32 clean all

rem On Linux, install mingw32 and use this:
rem make BACNET_PORT=win32 OPTIMIZATION=-Os DEBUGGING= CC=i586-mingw32msvc-gcc AR=i586-mingw32msvc-ar clean all 
