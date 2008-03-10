@echo off
rem Build for Borland 5.5 tools
rem set BORLAND_DIR=c:\borland\bcc55
rem %BORLAND_DIR%\bin\make -f makefile.b32 clean
rem %BORLAND_DIR%\bin\make -f makefile.b32 all

rem Build for MinGW
make BACNET_PORT=win32 OPTIMIZATION=-Os DEBUGGING= clean all

