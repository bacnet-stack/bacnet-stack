@echo off
echo Build with MinGW <http://mingw.sourceforge.net/>
make BACNET_PORT=win32 OPTIMIZATION=-Os DEBUGGING= clean all
rem Build for MinGW debug
rem make BACNET_PORT=win32 clean all

rem On Linux, install mingw32 and use this:
rem make BACNET_PORT=win32 OPTIMIZATION=-Os DEBUGGING= CC=i586-mingw32msvc-gcc AR=i586-mingw32msvc-ar clean all 
